#ifndef MANCHESTER_DRIVER_H_
#define MANCHESTER_DRIVER_H_

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "manchester_config.h"

struct EdgeEvent
{
    uint32_t timestamp;
    bool level;
};



bool manchesterBegin(uint8_t rxPin = MANCHESTER_RX_PIN,
                     uint8_t txPin = MANCHESTER_TX_PIN,
                     uint32_t bitRate = MANCHESTER_BIT_RATE);

bool manchesterSendBytes(const uint8_t *data, size_t length);
bool manchesterReceiveByte(uint8_t &value, TickType_t timeout = portMAX_DELAY);
void manchesterFlushRx();

#endif