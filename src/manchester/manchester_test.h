#ifndef MANCHESTER_TEST_H_
#define MANCHESTER_TEST_H_

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

bool testManchesterBegin(
    uint8_t rxPin,
    uint8_t txPin,
    uint32_t bitRate);

bool testManchesterSendBytes(
    const uint8_t *data,
    size_t length);

bool testManchesterReceiveByte(
    uint8_t &byte,
    TickType_t timeout);


#endif