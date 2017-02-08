
#include "asf.h"
#include "project_utils.h"
#include "project_tasks.h"

/* Final decimal variables */
volatile int short temp_room = 0;
volatile int short temp_target = 0;
volatile int short power = 0;

/* Tasks IDs */
volatile xTaskHandle *id_vTaskReadADCPotentiometer;
volatile xTaskHandle *id_vTaskReadADCSensor;
volatile xTaskHandle *id_vTaskComputePower;
volatile xTaskHandle *id_vTaskRefreshLCD;
volatile xTaskHandle *id_vTaskPowerLEDs;

void vTaskReadADCPotentiometer(void *pvParameters) {

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
		}
	
		vTaskDelay(100);

		vTaskPrioritySet(*id_vTaskReadADCPotentiometer, READ_ADC_POTENTIOMETER_PRIORITY - 2);
	}
}

void vTaskReadADCSensor(void *pvParameters) {

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
		}
	
		vTaskDelay(100);

		vTaskPrioritySet(*id_vTaskReadADCSensor, READ_ADC_SENSOR_PRIORITY - 2);
	}
}

void vTaskComputePower(void *pvParameters) {

	int power_percent = 0;

	for (;;) {
		/* Compute power */
		power_percent = compute_power();

		/* See if we can obtain the semaphore */
		if (xSemaphoreTake(SEMAPHORE_POWER, portMAX_DELAY) == 1) {
			power = power_percent;
			xSemaphoreGive(SEMAPHORE_POWER);
		}
		
		vTaskDelay(100);

		vTaskPrioritySet(*id_vTaskRefreshLCD, COMPUTE_POWER_PRIORITY + 1);
	}
}

void vTaskRefreshLCD(void *pvParameters) {

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

void vTaskPowerLEDs(void *pvParameters) {

	for (;;) {
		power_LEDs();
		reset_priorities();
	}
}

