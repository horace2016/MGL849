#ifndef PROJECT_TASKS_H
#define PROJECT_TASKS_H

/* Final decimal variables */
extern volatile int short temp_room;
extern volatile int short temp_target;
extern volatile int short power;

/* Tasks priorities */
#define READ_ADC_POTENTIOMETER_PRIORITY 6
#define READ_ADC_SENSOR_PRIORITY 6
#define COMPUTE_POWER_PRIORITY 5
#define REFRESH_LCD_PRIORITY 3
#define POWER_LEDS_PRIORITY 2

/* Tasks IDs */
extern volatile xTaskHandle *id_vTaskReadADCPotentiometer;
extern volatile xTaskHandle *id_vTaskReadADCSensor;
extern volatile xTaskHandle *id_vTaskComputePower;
extern volatile xTaskHandle *id_vTaskRefreshLCD;
extern volatile xTaskHandle *id_vTaskPowerLEDs;

/* Tasks */
void vTaskReadADCPotentiometer(void *);

void vTaskReadADCSensor(void *);

void vTaskComputePower(void *);

void vTaskRefreshLCD(void *);

void vTaskPowerLEDs(void *);

/* Semaphore */
extern volatile xSemaphoreHandle SEMAPHORE_TEMP_ROOM;
extern volatile xSemaphoreHandle SEMAPHORE_TEMP_TARGET;
extern volatile xSemaphoreHandle SEMAPHORE_POWER;

#endif /* PROJECT_TASKS_H */
