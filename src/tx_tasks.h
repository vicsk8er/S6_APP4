#ifndef TX_TASKS_H_
#define TX_TASKS_H_

#include <Arduino.h>
extern TaskHandle_t txTaskHandle;
void uartTxTask(void *pvParameters);

void wakeTxTask();

#endif