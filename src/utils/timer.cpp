#include "timer.h"

timer timers[MAX_FRAMES];

void initTimers()
{
    clearAllTimers();
}

void startTimer(CommunicationType type, uint8_t sequenceNumber)
{
    if (sequenceNumber < MAX_FRAMES)
    {
        timers[sequenceNumber].startTime = millis();
        timers[sequenceNumber].elapsedTime = 0;
        timers[sequenceNumber].typeOfFrameSent = type;
        timers[sequenceNumber].sequenceNumber = sequenceNumber;
    }
}

void stopTimer(uint8_t sequenceNumber){
    if (sequenceNumber < MAX_FRAMES && timers[sequenceNumber].startTime != 0)
    {
        timers[sequenceNumber].elapsedTime = millis() - timers[sequenceNumber].startTime;
    }
    else
    {
        timers[sequenceNumber].elapsedTime = 0;
    }
}

uint32_t getElapsedTime(uint8_t sequenceNumber)
{
    if (sequenceNumber < MAX_FRAMES)
    {
        return timers[sequenceNumber].elapsedTime;
    }
    return 0;
}

void clearAllTimers()
{
    for (uint32_t i = 0; i < MAX_FRAMES; ++i)
    {
        timers[i].startTime = 0;
        timers[i].elapsedTime = 0;
        timers[i].typeOfFrameSent = CommunicationType::Start; // Valeur par défaut
        timers[i].sequenceNumber = 0;
    }
}