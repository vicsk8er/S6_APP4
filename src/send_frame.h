#ifndef SEND_FRAME_H_
#define SEND_FRAME_H_

#include <stdbool.h>
#include "utils/config.h"

bool sendFrame(const Frame &frame);
struct TransmissionContext
{
    uint8_t currentFrame = 0;
};
#endif
