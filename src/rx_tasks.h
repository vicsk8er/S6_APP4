#ifndef RX_TASKS_H_
#define RX_TASKS_H_

#include <Arduino.h>
#include "utils/CRC_calculator.h"

void uartRxTask(void *pvParameters);
void sendNackFrame(uint8_t currentFrame);

#endif