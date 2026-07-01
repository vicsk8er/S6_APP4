#ifndef SEND_FRAME_H_
#define SEND_FRAME_H_

#include <Arduino.h>
#include <stdbool.h>
#include "utils/config.h"

bool sendFrame(const Frame &frame, HardwareSerial &uart = Serial1);
// bool sendStartFrame(uint8_t nbOfFrame, bool inject_error = false);
// bool sendDataFrame(const uint8_t *data, uint8_t length, bool inject_error = false);
// bool sendEndFrame(bool inject_error = false);
// bool sendNackFrame(uint8_t nbOfFrame, bool inject_error = false);

struct TransmissionContext
{
    uint8_t currentFrame = 0;
};
#endif
