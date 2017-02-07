
#include "asf.h"
#include "project_utils.h"
#include "project_tasks.h"

volatile signed short adc_value_pot = -1;
volatile signed short adc_value_temp = -1;

void initializeLCD(void) {

    static const gpio_map_t DIP204_SPI_GPIO_MAP =
            {
                    {DIP204_SPI_SCK_PIN,  DIP204_SPI_SCK_FUNCTION},  // SPI Clock.
                    {DIP204_SPI_MISO_PIN, DIP204_SPI_MISO_FUNCTION},  // MISO.
                    {DIP204_SPI_MOSI_PIN, DIP204_SPI_MOSI_FUNCTION},  // MOSI.
                    {DIP204_SPI_NPCS_PIN, DIP204_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
            };

    // Disable all interrupts
    Disable_global_interrupt();

    Enable_global_interrupt();

    // Add the spi options driver structure for the LCD DIP204
    spi_options_t spiOptions =
            {
                    .reg    = DIP204_SPI_NPCS,
                    .baudrate    = 1000000,
                    .bits        = 8,
                    .spck_delay    = 0,
                    .stay_act    = 1,
                    .trans_delay = 8,
                    .spi_mode    = 0,
                    .modfdis    = 1
            };
    // Assign I/Os to SPI
    gpio_enable_module(DIP204_SPI_GPIO_MAP, sizeof(DIP204_SPI_GPIO_MAP) / sizeof(DIP204_SPI_GPIO_MAP[0]));

    // Initialize as master
    spi_initMaster(DIP204_SPI, &spiOptions);

    // Set selection mode: variable_ps, pcs_decode, delay
    spi_selectionMode(DIP204_SPI, 0, 0, 0);

    // Enable SPI
    spi_enable(DIP204_SPI);

    // Setup chip registers
    spi_setupChipReg(DIP204_SPI, &spiOptions, FOSC0);

    // Initialize delay driver
    delay_init(FOSC0);

    // Initialize LCD
    dip204_init(backlight_PWM, true);

    dip204_clear_display();
    dip204_set_cursor_position(1, 1);
    dip204_write_string("Temp. target: ");
    dip204_set_cursor_position(20, 1);
    dip204_write_string("C");
    dip204_set_cursor_position(1, 2);
    dip204_write_string("Temp. room: ");
    dip204_set_cursor_position(20, 2);
    dip204_write_string("C");
    dip204_set_cursor_position(1, 3);
    dip204_write_string("Power: ");
    dip204_set_cursor_position(20, 3);
    dip204_write_string("%");
    dip204_hide_cursor();

}

void initializeADC(void) {

    /** GPIO pin/adc-function map. */
    const gpio_map_t ADC_GPIO_MAP = {
            {ADC_TEMPERATURE_PIN,   ADC_TEMPERATURE_FUNCTION},
            {ADC_POTENTIOMETER_PIN, ADC_POTENTIOMETER_FUNCTION}
    };

    /* Init system clocks */
    sysclk_init();

    /* Assign and enable GPIO pins to the ADC function. */
    gpio_enable_module(ADC_GPIO_MAP, sizeof(ADC_GPIO_MAP) /
                                     sizeof(ADC_GPIO_MAP[0]));

    /* Configure the ADC peripheral module.
     * Lower the ADC clock to match the ADC characteristics (because we
     * configured the CPU clock to 12MHz, and the ADC clock characteristics are
     *  usually lower; cf. the ADC Characteristic section in the datasheet). */
    AVR32_ADC.mr |= 0x1 << AVR32_ADC_MR_PRESCAL_OFFSET;
    adc_configure(&AVR32_ADC);

    /* Enable the ADC channels. */
    adc_enable(&AVR32_ADC, ADC_TEMPERATURE_CHANNEL);
    adc_enable(&AVR32_ADC, ADC_POTENTIOMETER_CHANNEL);
}

void clear_LCD() {
	dip204_set_cursor_position(14, 1);
	dip204_write_string("      ");
	dip204_set_cursor_position(12, 2);
	dip204_write_string("        ");
	dip204_set_cursor_position(7, 3);
	dip204_write_string("             ");
}

void reset_priorities() {
	vTaskPrioritySet(*id_vTaskReadADCPotentiometer, READ_ADC_POTENTIOMETER_PRIORITY);
	vTaskPrioritySet(*id_vTaskReadADCSensor, READ_ADC_SENSOR_PRIORITY);
	vTaskPrioritySet(*id_vTaskComputePower, COMPUTE_POWER_PRIORITY);
	vTaskPrioritySet(*id_vTaskRefreshLCD, REFRESH_LCD_PRIORITY);
	vTaskPrioritySet(*id_vTaskPowerLEDs, POWER_LEDS_PRIORITY);
}

void power_LEDs(void) {
	int i;
	int step = 100 / LED_COUNT;
	uint32_t value = 0; // One bit per LED (8 leds)
	for (i = 0; i < LED_COUNT; i++) {
		if (power > i * step)
		value |= 1 << i; // Set LED's bit to '1' if power exceeds LED's "value"
	}

	LED_Display(value);
}

int compute_power(void) {
	int power = (int) (((double) (temp_target - temp_room) / 6) * 100);
	return power >= 0 ? (power <= 100 ? power : 100) : 0;
}

void convert_sensor_hex_to_celsius(int *temp_room_celsius) {
	int index = 0;
	if (adc_value_temp > temperature_code[0]) {
		*temp_room_celsius = -20;
		} else {
		while (temperature_code[index++] > adc_value_temp);
		*temp_room_celsius = index - 1 - 20;
	}
}

void convert_potentiometer_hex_to_celsius(int *temp_target_celsius) {
	*temp_target_celsius = 5 + adc_value_pot * 26 / 1024;
}
