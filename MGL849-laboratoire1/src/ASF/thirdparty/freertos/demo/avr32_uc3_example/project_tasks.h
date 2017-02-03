#ifndef PROJECT_TASKS_H
#define PROJECT_TASKS_H

/* Final decimal variables */
volatile int short temp_room = 0;
volatile int short temp_target = 0;
volatile int short power = 0;

/* Tasks priorities */
#define READ_ADC_POTENTIOMETER_PRIORITY 6
#define READ_ADC_SENSOR_PRIORITY 6
#define COMPUTE_POWER_PRIORITY 5
#define REFRESH_LCD_PRIORITY 3
#define POWER_LEDS_PRIORITY 2

/* Tasks IDs */
volatile xTaskHandle *id_vTaskReadADCPotentiometer;
volatile xTaskHandle *id_vTaskReadADCSensor;
volatile xTaskHandle *id_vTaskComputePower;
volatile xTaskHandle *id_vTaskRefreshLCD;
volatile xTaskHandle *id_vTaskPowerLEDs;

/* Tasks */
static void vTaskReadADCPotentiometer(void *);

static void vTaskReadADCSensor(void *);

static void vTaskComputePower(void *);

static void vTaskRefreshLCD(void *);

static void vTaskPowerLEDs(void *);

/* Methods */
static void clear_LCD(void);

static void reset_priorities(void);

static void convert_sensor_hex_to_celsius(int *);

static void convert_potentiometer_hex_to_celsius(int *);

static int compute_power(void);

static void power_LEDs(void);

/* Semaphore */
static xSemaphoreHandle SEMAPHORE_TEMP_ROOM = NULL;
static xSemaphoreHandle SEMAPHORE_TEMP_TARGET = NULL;
static xSemaphoreHandle SEMAPHORE_POWER = NULL;

const unsigned short temperature_code[] = {
        0x3B4, 0x3B0, 0x3AB, 0x3A6, 0x3A0, 0x39A, 0x394, 0x38E, 0x388, 0x381, 0x37A, 0x373,
        0x36B, 0x363, 0x35B, 0x353, 0x34A, 0x341, 0x338, 0x32F, 0x325, 0x31B, 0x311, 0x307,
        0x2FC, 0x2F1, 0x2E6, 0x2DB, 0x2D0, 0x2C4, 0x2B8, 0x2AC, 0x2A0, 0x294, 0x288, 0x27C,
        0x26F, 0x263, 0x256, 0x24A, 0x23D, 0x231, 0x225, 0x218, 0x20C, 0x200, 0x1F3, 0x1E7,
        0x1DB, 0x1CF, 0x1C4, 0x1B8, 0x1AC, 0x1A1, 0x196, 0x18B, 0x180, 0x176, 0x16B, 0x161,
        0x157, 0x14D, 0x144, 0x13A, 0x131, 0x128, 0x11F, 0x117, 0x10F, 0x106, 0xFE, 0xF7,
        0xEF, 0xE8, 0xE1, 0xDA, 0xD3, 0xCD, 0xC7, 0xC0, 0xBA, 0xB5, 0xAF, 0xAA, 0xA4, 0x9F,
        0x9A, 0x96, 0x91, 0x8C, 0x88, 0x84, 0x80, 0x7C, 0x78, 0x74, 0x71, 0x6D, 0x6A, 0x67,
        0x64, 0x61, 0x5E, 0x5B, 0x58, 0x55, 0x53, 0x50, 0x4E, 0x4C, 0x49, 0x47, 0x45, 0x43,
        0x41, 0x3F, 0x3D, 0x3C, 0x3A, 0x38};

/************************************************************************************/

static void vTaskReadADCPotentiometer(void *pvParameters) {

    int temp_target_celsius = 0;

    for (;;) {
        /* Start conversions on all enabled channels */
        adc_start(&AVR32_ADC);

        /* Get value for the potentiometer adc channel */
        adc_value_pot = adc_get_value(&AVR32_ADC, ADC_POTENTIOMETER_CHANNEL);

        /* Convert hex value of potentiometer to Celsius ----*/
		convert_potentiometer_hex_to_celsius(&temp_target_celsius);

        /* See if we can obtain the semaphore */
        if (xSemaphoreTake(SEMAPHORE_TEMP_TARGET, portMAX_DELAY) == 1) {
            temp_target = temp_target_celsius;
            xSemaphoreGive(SEMAPHORE_TEMP_TARGET);
            vTaskDelay(100);
        }

        vTaskPrioritySet(*id_vTaskReadADCPotentiometer, READ_ADC_POTENTIOMETER_PRIORITY - 2);
    }
}

static void vTaskReadADCSensor(void *pvParameters) {

	int temp_room_celsius = 0;

	for (;;) {
		/* Start conversions on all enabled channels */
		adc_start(&AVR32_ADC);

		/* Get value for the temperature adc channel */
		adc_value_temp = adc_get_value(&AVR32_ADC, ADC_TEMPERATURE_CHANNEL);

		/* Convert hex value of current temperature to Celsius */
		convert_sensor_hex_to_celsius(&temp_room_celsius);

		/* See if we can obtain the semaphore */
		if (xSemaphoreTake(SEMAPHORE_TEMP_ROOM, portMAX_DELAY) == 1) {
			temp_room = temp_room_celsius;
			xSemaphoreGive(SEMAPHORE_TEMP_ROOM);
			vTaskDelay(100);
		}

        vTaskPrioritySet(*id_vTaskReadADCSensor, READ_ADC_SENSOR_PRIORITY - 2);
	}
}

static void vTaskComputePower(void *pvParameters) {

	int power_percent = 0;

	for (;;) {
		/* Compute power */
		power_percent = compute_power();

		/* See if we can obtain the semaphore */
		if (xSemaphoreTake(SEMAPHORE_POWER, portMAX_DELAY) == 1) {
			power = power_percent;
			xSemaphoreGive(SEMAPHORE_POWER);
			vTaskDelay(100);
		}

		vTaskPrioritySet(*id_vTaskRefreshLCD, COMPUTE_POWER_PRIORITY + 1);
	}
}

static void vTaskRefreshLCD(void *pvParameters) {

    for (;;) {
        clear_LCD();
        dip204_set_cursor_position(16, 1);
		if (xSemaphoreTake(SEMAPHORE_TEMP_TARGET, portMAX_DELAY) == 1) {
	        dip204_printf_string("%d", temp_target);
			xSemaphoreGive(SEMAPHORE_TEMP_TARGET);
		}
        dip204_set_cursor_position(16, 2);
		if (xSemaphoreTake(SEMAPHORE_TEMP_ROOM, portMAX_DELAY) == 1) {
	        dip204_printf_string("%d", temp_room);
			xSemaphoreGive(SEMAPHORE_TEMP_ROOM);
		}
        dip204_set_cursor_position(16, 3);
		if (xSemaphoreTake(SEMAPHORE_POWER, portMAX_DELAY) == 1) {
	        dip204_printf_string("%d", power);
			xSemaphoreGive(SEMAPHORE_POWER);
		}
        vTaskDelay(100);
        vTaskPrioritySet(*id_vTaskPowerLEDs, REFRESH_LCD_PRIORITY + 1);
    }
}

static void vTaskPowerLEDs(void *pvParameters) {

    for (;;) {
        power_LEDs();
		reset_priorities();
    }
}

/************************************************/

static void clear_LCD() {
	dip204_set_cursor_position(14, 1);
	dip204_write_string("      ");
	dip204_set_cursor_position(12, 2);
	dip204_write_string("        ");
	dip204_set_cursor_position(7, 3);
	dip204_write_string("             ");
}

static void reset_priorities() {
    vTaskPrioritySet(*id_vTaskReadADCPotentiometer, READ_ADC_POTENTIOMETER_PRIORITY);
    vTaskPrioritySet(*id_vTaskReadADCSensor, READ_ADC_SENSOR_PRIORITY);
    vTaskPrioritySet(*id_vTaskComputePower, COMPUTE_POWER_PRIORITY);
    vTaskPrioritySet(*id_vTaskRefreshLCD, REFRESH_LCD_PRIORITY);
    vTaskPrioritySet(*id_vTaskPowerLEDs, POWER_LEDS_PRIORITY);
}

static void power_LEDs(void) {
    int i;
    int step = 100 / LED_COUNT;
    uint32_t value = 0; // One bit per LED (8 leds)
    for (i = 0; i < LED_COUNT; i++) {
        if (power > i * step)
            value |= 1 << i; // Set LED's bit to '1' if power exceeds LED's "value"
    }

    LED_Display(value);
}

static int compute_power(void) {
    int power = (int) (((double) (temp_target - temp_room) / 6) * 100);
    return power >= 0 ? power : 0;
}

static void convert_sensor_hex_to_celsius(int *temp_room_celsius) {
        int index = 0;
        if (adc_value_temp > temperature_code[0]) {
	        *temp_room_celsius = -20;
	        } else {
	        while (temperature_code[index++] > adc_value_temp);
	        *temp_room_celsius = index - 1 - 20;
        }
}

static void convert_potentiometer_hex_to_celsius(int *temp_target_celsius) {
        *temp_target_celsius = 5 + adc_value_pot * 26 / 1024;
}

#endif /* PROJECT_TASKS_H */
