/************************************************************************/
/* MGL849 Laboratoire 1 squelette                                       */
/* Noms et Code permanent des étudiants:                                */
/*  Mon Nom - codepermanent                                             */
/************************************************************************/

/* Includes standard libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Includes from example projects */
#include "asf.h"
#include "conf_adc.h"
#include "dip204.h"

/* Environment header files */
#include "power_clocks_lib.h"

/* Scheduler header files */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/*! \name Priority definitions for most of the tasks in the demo application.
* Some tasks just use the idle priority.
*/
//! @{
#define mainFLASH_LEDS_PRIORITY ( tskIDLE_PRIORITY + 1 )
//! @}

/** GPIO pin/adc-function map. */
const gpio_map_t ADC_GPIO_MAP = {
	#if defined(EXAMPLE_ADC_TEMPERATURE_CHANNEL)
	{EXAMPLE_ADC_TEMPERATURE_PIN, EXAMPLE_ADC_TEMPERATURE_FUNCTION},
	#endif
	#if defined(EXAMPLE_ADC_POTENTIOMETER_CHANNEL)
	{EXAMPLE_ADC_POTENTIOMETER_PIN, EXAMPLE_ADC_POTENTIOMETER_FUNCTION}
	#endif
};

static const gpio_map_t DIP204_SPI_GPIO_MAP = {
	{DIP204_SPI_SCK_PIN,  DIP204_SPI_SCK_FUNCTION },  // SPI Clock.
	{DIP204_SPI_MISO_PIN, DIP204_SPI_MISO_FUNCTION},  // MISO.
	{DIP204_SPI_MOSI_PIN, DIP204_SPI_MOSI_FUNCTION},  // MOSI.
	{DIP204_SPI_NPCS_PIN, DIP204_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
};

//! ASCII representation of hexadecimal digits.
static const char HEX_DIGITS[16] = "0123456789ABCDEF";

/* Temporary hex variables */
#if defined(EXAMPLE_ADC_TEMPERATURE_CHANNEL)
volatile signed short adc_value_temp = -1;
#endif
#if defined(EXAMPLE_ADC_POTENTIOMETER_CHANNEL)
volatile signed short adc_value_pot = -1;
#endif

/* Final decimal variables */
volatile int temp_room = 0;
volatile int temp_target = 0;
volatile int power = 0;

const unsigned short temperature_code[] = {
	0x3B4,0x3B0,0x3AB,0x3A6,0x3A0,0x39A,0x394,0x38E,0x388,0x381,0x37A,0x373,
	0x36B,0x363,0x35B,0x353,0x34A,0x341,0x338,0x32F,0x325,0x31B,0x311,0x307,
	0x2FC,0x2F1,0x2E6,0x2DB,0x2D0,0x2C4,0x2B8,0x2AC,0x2A0,0x294,0x288,0x27C,
	0x26F,0x263,0x256,0x24A,0x23D,0x231,0x225,0x218,0x20C,0x200,0x1F3,0x1E7,
	0x1DB,0x1CF,0x1C4,0x1B8,0x1AC,0x1A1,0x196,0x18B,0x180,0x176,0x16B,0x161,
	0x157,0x14D,0x144,0x13A,0x131,0x128,0x11F,0x117,0x10F,0x106,0xFE,0xF7,
	0xEF,0xE8,0xE1,0xDA,0xD3,0xCD,0xC7,0xC0,0xBA,0xB5,0xAF,0xAA,0xA4,0x9F,
	0x9A,0x96,0x91,0x8C,0x88,0x84,0x80,0x7C,0x78,0x74,0x71,0x6D,0x6A,0x67,
	0x64,0x61,0x5E,0x5B,0x58,0x55,0x53,0x50,0x4E,0x4C,0x49,0x47,0x45,0x43,
	0x41,0x3F,0x3D,0x3C,0x3A,0x38
};


/*-----------------------------------------------------------*/

/* Tasks */
static void read_values( void *pvParameters );

static void convert_values( void *pvParameters );

static void calculate_required_power();

static void print_values( void *pvParameters );

/* Methods */
static void init_freeRTOS();

static void init_microcontroller();

static void init_screen();

static void create_tasks();

static void read_temperature();

static void read_potentiometer();

static void convert_temperature();

static void convert_potentiometer();

/* Debug */
// static void write_hexadecimal_string(unsigned long n);

/* Semaphore */
static xSemaphoreHandle SYNCHRO_SEMAPHORE = NULL;

/*-----------------------------------------------------------*/

int main( void )
{

	init_microcontroller();
	init_freeRTOS();
	
	/* Will only get here if there was insufficient memory to create the idle task. */

	return 0;
}

/*-----------------------------------------------------------*/

/* Tasks */
static void read_values( void *pvParameters )
{
	while(1){
		printf("%s\n", "Reading values\n");

		read_temperature();
		read_potentiometer();

		vTaskDelay(1000);
	}
}

static void convert_values( void *pvParameters )
{
	while(1){
		printf("%s\n", "Converting values\n");

		convert_temperature();
		convert_potentiometer();
		
		vTaskDelay(1000);
	}
}

static void convert_temperature()
{
		/* Convert hex value to celsius */
	int temp_cel = 0;
	int index = 0;

	index = 0;
	if(adc_value_temp > temperature_code[0])
	{
		temp_cel = -20;
	}
	else
	{
		while(temperature_code[index++] > adc_value_temp);
		temp_cel = index - 1 - 20;
	}

	if( xSemaphoreTake( SYNCHRO_SEMAPHORE,  portMAX_DELAY ) == 1 )
	{
		temp_room = temp_cel;
		xSemaphoreGive( SYNCHRO_SEMAPHORE );
	}

}

static void convert_potentiometer()
{
		/* Convert hex value to decimal */
	int pot_dec = 0;
	pot_dec = pot_dec * 30 / 1024;

	if( xSemaphoreTake( SYNCHRO_SEMAPHORE,  portMAX_DELAY ) == 1 )
	{
		temp_target = pot_dec;
		xSemaphoreGive( SYNCHRO_SEMAPHORE );
	}

}

static void print_values( void *pvParameters )
{
	while(1){
		printf("%s\n", "Printing values\n");

		printf("Temp. target : %d\n", temp_target);
		printf("Temp. room : %d\n", temp_room);
		printf("Power : %d\n", power);

		/* Display target temperature to user */
		/* go to first column of 1st line */
		dip204_set_cursor_position(1,1);
		dip204_write_string("Temp. target : ");
		dip204_write_data(temp_target);

		/* Display room temperature to user */
		/* go to first column of 2nd line */
		dip204_set_cursor_position(1,2);
		dip204_write_string("Temp. room : ");
		dip204_write_data(temp_room);

		/* Display power to user */
		/* go to first column of 2nd line */
		dip204_set_cursor_position(1,3);
		dip204_write_string("Power : ");
		dip204_write_data(power);

		vTaskDelay(1000);
	}
}

/*-----------------------------------------------------------*/

/* Methods */
static void init_freeRTOS()
{
	// Initialize your semaphore
	SYNCHRO_SEMAPHORE = xSemaphoreCreateCounting(1,1);

	create_tasks();

	/* Start the scheduler. */
	vTaskStartScheduler();	
}

static void init_microcontroller()
{

	// Configure Osc0 in crystal mode (i.e. use of an external crystal source, with
	// frequency FOSC0 (12Mhz) ) with an appropriate startup time then switch the main clock
	// source to Osc0.
	pcl_switch_to_osc(PCL_OSC0, FOSC0, OSC0_STARTUP);

	/* Init screen */
	init_screen();

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
#if defined(EXAMPLE_ADC_TEMPERATURE_CHANNEL)
	adc_enable(&AVR32_ADC, EXAMPLE_ADC_TEMPERATURE_CHANNEL);
#endif
#if defined(EXAMPLE_ADC_POTENTIOMETER_CHANNEL)
	adc_enable(&AVR32_ADC, EXAMPLE_ADC_POTENTIOMETER_CHANNEL);
#endif

	dip204_hide_cursor();

}

static void init_screen() {

  // Switch the CPU main clock to oscillator 0
	pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  // Disable all interrupts.
	Disable_global_interrupt();

  // init the interrupts
	INTC_init_interrupts();

  // Enable all interrupts.
	Enable_global_interrupt();

  // add the spi options driver structure for the LCD DIP204
	spi_options_t spiOptions =
	{
		.reg          = DIP204_SPI_NPCS,
		.baudrate     = 1000000,
		.bits         = 8,
		.spck_delay   = 0,
		.trans_delay  = 8,
		.stay_act     = 1,
		.spi_mode     = 0,
		.modfdis      = 1
	};

  // Assign I/Os to SPI
	gpio_enable_module(DIP204_SPI_GPIO_MAP,	sizeof(DIP204_SPI_GPIO_MAP) / sizeof(DIP204_SPI_GPIO_MAP[0]));

  // Initialize as master
	spi_initMaster(DIP204_SPI, &spiOptions);

  // Set selection mode: variable_ps, pcs_decode, delay
	spi_selectionMode(DIP204_SPI, 0, 0, 0);

  // Enable SPI
	spi_enable(DIP204_SPI);

  // setup chip registers
	spi_setupChipReg(DIP204_SPI, &spiOptions, FOSC0);

  // initialize LCD
	dip204_init(backlight_PWM, true);

}

static void create_tasks()
{
	/* Start the tasks defined within this file. */
	xTaskCreate(
		read_values
		,  (const signed portCHAR *)"ReadValues"
		,  configMINIMAL_STACK_SIZE
		,  NULL
		,  mainFLASH_LEDS_PRIORITY
		,  NULL );

	xTaskCreate(
		convert_values
		,  (const signed portCHAR *)"ConvertValues"
		,  configMINIMAL_STACK_SIZE
		,  NULL
		,  mainFLASH_LEDS_PRIORITY
		,  NULL );

	xTaskCreate(
		calculate_required_power
		,  (const signed portCHAR *)"CalculatePower"
		,  configMINIMAL_STACK_SIZE
		,  NULL
		,  mainFLASH_LEDS_PRIORITY
		,  NULL );

	xTaskCreate(
		print_values
		,  (const signed portCHAR *)"PrintValues"
		,  configMINIMAL_STACK_SIZE
		,  NULL
		,  mainFLASH_LEDS_PRIORITY
		,  NULL );

}

static void read_temperature()
{

	printf("%s\n", "Reading temperature\n");

		/* Start conversions on all enabled channels */
	adc_start(&AVR32_ADC);

		/* See if we can obtain the semaphore */
	if( xSemaphoreTake( SYNCHRO_SEMAPHORE,  portMAX_DELAY ) == 1 )
	{
			/* Get value for the temperature adc channel */
		adc_value_temp = adc_get_value(&AVR32_ADC,EXAMPLE_ADC_TEMPERATURE_CHANNEL);
		xSemaphoreGive( SYNCHRO_SEMAPHORE );
	}

}

static void read_potentiometer()
{

	printf("%s\n", "Reading potentiometer\n");

		/* Start conversions on all enabled channels */
	adc_start(&AVR32_ADC);

		/* See if we can obtain the semaphore */
	if( xSemaphoreTake( SYNCHRO_SEMAPHORE,  portMAX_DELAY ) == 1 )
	{
		/* Get value for the potentiometer adc channel */
		adc_value_pot = adc_get_value(&AVR32_ADC, EXAMPLE_ADC_POTENTIOMETER_CHANNEL);
		xSemaphoreGive( SYNCHRO_SEMAPHORE );
	}

}

static void calculate_required_power()
{
	printf("%s\n", "Calculating required power\n");

		/* See if we can obtain the semaphore */
	if( xSemaphoreTake( SYNCHRO_SEMAPHORE,  portMAX_DELAY ) == 1 )
	{
		/* Compute power */
		power = (int)(((double) (temp_target - temp_room) / 6) * 100);
		xSemaphoreGive( SYNCHRO_SEMAPHORE );
	}

}

/*
static void write_hexadecimal_string(unsigned long n)
{

	char tmp[9];
	int i;

	// Convert the given number to an ASCII hexadecimal representation.
	tmp[8] = '\0';
	for (i = 7; i >= 0; i--)
	{
		tmp[i] = HEX_DIGITS[n & 0xF];
		n >>= 4;
	}

	dip204_write_data(n);
}
*/