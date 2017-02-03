
/************************************************************************/
/* MGL849 Laboratoire 1 squelette                                       */
/* Noms et Code permanent des �tudiants:                                */
/*  Mon Nom - codepermanent                                             */
/************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Environment header files. */
#include "power_clocks_lib.h"

/* Scheduler header files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

//#include "asf.h"  // horace
#include <asf.h>
#include "laboMGL849.h"
#include "myAllTask.h"


int main(void) {

    // Configure Osc0 in crystal mode (i.e. use of an external crystal source, with
    // frequency FOSC0 (12Mhz) ) with an appropriate startup time then switch the main clock
    // source to Osc0.
    //pcl_switch_to_osc(PCL_OSC0, FOSC0, OSC0_STARTUP);
    pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

    /* Initialisation de l'ADC*/
    initializeADC();

    /* Initialisation du LCD*/
    initializeLCD();

    // Initialize your semaphore
    SYNCHRO_SEMAPHORE = xSemaphoreCreateMutex();

    if (!(pdPASS == xTaskCreate(vTaskReadADC, (signed char *) "Lecture ADC", 200, NULL, READ_ADC_PRIORITY,
                                (xTaskHandle *) id_vTaskReadADC)))
        goto hell;
    if (!(pdPASS == xTaskCreate(vTaskRefreshLCD, (signed char *) "Task afficage", 5000, NULL, LCD_REFRESH_PRIORITY,
                                (xTaskHandle *) id_vTaskRefreshLCD)))
        goto hell;
    if (!(pdPASS == xTaskCreate(vTaskLedPOWER, (signed char *) "Task LED", 2000, NULL, TRAITEMENT_PRIORITY,
                                (xTaskHandle *) id_vTaskLedPOWER)))
        goto hell;

    /* Start the scheduler. */
    vTaskStartScheduler();

    hell:
    /* Will only get here if there was insufficient memory to create the idle
    task. */
    return 0;
}