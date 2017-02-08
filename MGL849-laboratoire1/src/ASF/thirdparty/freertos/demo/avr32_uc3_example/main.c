
/************************************************************************/
/* MGL849 Laboratoire 1                                                 */
/* Noms et Code permanent des etudiants:                                */
/* GANDJI Horace -                                                      */
/* BOUFFARD Nicolas - BOUN15039408                                      */
/************************************************************************/

/* Standard header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Environment header files. */
#include "asf.h"
#include "project_utils.h"
#include "project_tasks.h"

int main(void) {

    // Configure Osc0 in crystal mode (i.e. use of an external crystal source, with
    // frequency FOSC0 (12Mhz) ) with an appropriate startup time then switch the main clock
    // source to Osc0.
    pcl_switch_to_osc(PCL_OSC0, FOSC0, OSC0_STARTUP);

    /* Initialize ADC */
    initializeADC();

    /* Initialize LCD */
    initializeLCD();

    /* Initialize semaphores */
    SEMAPHORE_TEMP_TARGET = xSemaphoreCreateMutex();
    SEMAPHORE_TEMP_ROOM = xSemaphoreCreateMutex();
    SEMAPHORE_POWER = xSemaphoreCreateMutex();

    if (!(pdPASS == xTaskCreate(vTaskReadADCPotentiometer, (signed char *) "Read ADC Potentiometer", 200, NULL, READ_ADC_POTENTIOMETER_PRIORITY,
                                (xTaskHandle *) id_vTaskReadADCPotentiometer)))
        goto hell;
    if (!(pdPASS == xTaskCreate(vTaskReadADCSensor, (signed char *) "Read ADC Sensor", 200, NULL, READ_ADC_SENSOR_PRIORITY,
							    (xTaskHandle *) id_vTaskReadADCSensor)))
    goto hell;
	if (!(pdPASS == xTaskCreate(vTaskComputePower, (signed char *) "Compute Power", 200, NULL, COMPUTE_POWER_PRIORITY,
								(xTaskHandle *) id_vTaskComputePower)))
	goto hell;
    if (!(pdPASS == xTaskCreate(vTaskRefreshLCD, (signed char *) "Refresh LCD", 5000, NULL, REFRESH_LCD_PRIORITY,
                                (xTaskHandle *) id_vTaskRefreshLCD)))
        goto hell;
    if (!(pdPASS == xTaskCreate(vTaskPowerLEDs, (signed char *) "Power LEDs", 2000, NULL, POWER_LEDS_PRIORITY,
                                (xTaskHandle *) id_vTaskPowerLEDs)))
        goto hell;

    /* Start the scheduler. */
    vTaskStartScheduler();

    hell:

    /* Will only get here if there was insufficient memory to create the idle task. */
    return 0;
}