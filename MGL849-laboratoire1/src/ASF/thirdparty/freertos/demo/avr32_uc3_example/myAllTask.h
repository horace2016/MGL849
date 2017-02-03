#ifndef MYALLTASK
#define MYALLTASK

volatile unsigned short compteur = 0;
volatile int short power = 0;


/* Priority of task */
#define READ_ADC_PRIORITY   4 //(tskIDLE_PRIORITY + 1 ) //(tskIDLE_PRIORITY + 1 ) //( 4)
#define LCD_REFRESH_PRIORITY 3 //(tskIDLE_PRIORITY + 1 ) //tskIDLE_PRIORITY + 1  //(4)
#define TRAITEMENT_PRIORITY 2 // (tskIDLE_PRIORITY + 1 ) //skIDLE_PRIORITY + 1  //(4)

/* Identifiant des taches */
volatile xTaskHandle *id_vTaskReadADC;
volatile xTaskHandle *id_vTaskRefreshLCD;
volatile xTaskHandle *id_vTaskLedPOWER;

static void vTaskReadADC(void *pvParameters);

static void vTaskRefreshLCD(void *pvParameters);

static void vTaskLedPOWER(void *pvParameters);

static void clearLCD(void);

static int calculateRequiredPower(void);

static void powerLed(void);


/* semaphore  */
static xSemaphoreHandle SYNCHRO_SEMAPHORE = NULL;

/* semaphore  */
volatile int currentTemperature = 0;
volatile int orderTemperature = 0;


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

/*!
*
*/
static void vTaskReadADC(void *pvParameters) {

    int temps_cel = 0;
    int temps_pot = 0;
    int power_percent = 0;
    int index = 0;

    for (;;) {
        /* Start conversions on all enabled channels */
        adc_start(&AVR32_ADC);

        /* Get value for the temperature adc channel */
        adc_value_temp = adc_get_value(&AVR32_ADC, ADC_TEMPERATURE_CHANNEL);

        /* Get value for the potentiometer adc channel */
        adc_value_pot = adc_get_value(&AVR32_ADC, ADC_POTENTIOMETER_CHANNEL);

        /*--- convert hex value curent temp to cellsius ----*/
        index = 0;
        if (adc_value_temp > temperature_code[0]) {
            temps_cel = -20;
        } else {
            while (temperature_code[index++] > adc_value_temp);
            temps_cel = index - 1 - 20;
        }

        /*--- convert hex value potentiometer to cellsius ----*/
        temps_pot = 5 + adc_value_pot * 26 / 1024;

        /* Get power */
        power_percent = calculateRequiredPower();
        // See if we can obtain the semaphore.

        if (xSemaphoreTake(SYNCHRO_SEMAPHORE, portMAX_DELAY) == 1) {
            currentTemperature = temps_cel;
            orderTemperature = temps_pot;
            power = power_percent;
            xSemaphoreGive(SYNCHRO_SEMAPHORE);
            vTaskDelay(100);
        }
        vTaskPrioritySet(*id_vTaskRefreshLCD, READ_ADC_PRIORITY + 1);
    }
}

static void clearLCD() {
    dip204_set_cursor_position(14, 1);
    dip204_write_string("      ");
    dip204_set_cursor_position(12, 2);
    dip204_write_string("        ");
    dip204_set_cursor_position(7, 3);
    dip204_write_string("             ");
}

/*!
*
*/
static void vTaskRefreshLCD(void *pvParameters) {

    for (;;) {
        clearLCD();
        dip204_set_cursor_position(16, 1);
        dip204_printf_string("%d", orderTemperature);
        dip204_set_cursor_position(16, 2);
        dip204_printf_string("%d", currentTemperature);
        dip204_set_cursor_position(16, 3);
        dip204_printf_string("%d", power);
        vTaskDelay(100);
        vTaskPrioritySet(*id_vTaskLedPOWER, LCD_REFRESH_PRIORITY + 1);
    }
}

/*!
*
*/
static void vTaskLedPOWER(void *pvParameters) {
    for (;;) {
        powerLed();
        vTaskPrioritySet(*id_vTaskReadADC, READ_ADC_PRIORITY);
        vTaskPrioritySet(*id_vTaskRefreshLCD, LCD_REFRESH_PRIORITY);
        vTaskPrioritySet(*id_vTaskLedPOWER, TRAITEMENT_PRIORITY);
    }
}

/************************************************/
static void powerLed(void) {
    int i;
    int step = 100 / LED_COUNT;
    uint32_t value = 0; // One bit per led (8 leds)
    for (i = 0; i < LED_COUNT; i++) {
        if (power > i * step)
            value |= 1 << i; // Set led's bit to '1' if power exceeds led's "value"
    }

    LED_Display(value);
}

/*           Calcul de la puissance */
static int calculateRequiredPower(void) {
    int power = (int) (((double) (orderTemperature - currentTemperature) / 6) * 100);
    return power >= 0 ? power : 0;
}

#endif /* MYALLTASK */
