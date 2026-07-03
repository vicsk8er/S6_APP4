#ifndef RECEIVE_FRAME_H_
#define RECEIVE_FRAME_H_

#include <stdint.h>

#include "utils/config.h"
#include "utils/error_code.h"

struct ReceptionContext
{
    uint8_t currentFrame = 0;
    uint8_t expectedTotal = 0;
    bool receptionStarted = false;
    ErrorCode error = ErrorCode::COMM_OK;
};

bool receivedFrame(Frame &frame, ReceptionContext &context);

void resetReceptionState(ReceptionContext &context);

// bool isFrameValide(const Frame &frame, ReceptionContext &context);
// bool isFrameInOrder(const Frame &frame, ReceptionContext &context);

#endif