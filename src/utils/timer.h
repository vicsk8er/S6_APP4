#ifndef TIMER_H_
#define TIMER_H_
#include <Arduino.h>
#include <stdint.h>
#include "config.h"
#include "frame_buffer.h"

struct timer
{
    uint32_t startTime;
    uint32_t elapsedTime;
    CommunicationType typeOfFrameSent;
    uint8_t sequenceNumber;
};


void initTimers();
void startTimer(CommunicationType type, uint8_t sequenceNumber);
uint32_t getElapsedTime(uint8_t sequenceNumber);
void stopTimer(uint8_t sequenceNumber);
void clearAllTimers();


#endif
