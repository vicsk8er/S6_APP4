#include "receive_frame.h"

#include "manchester/manchester_driver.h"
#include "utils/CRC_calculator.h"
#include <cstring>

static ReceptionContext rxContext;

static bool readByteBlocking(uint8_t &value)
{
    TickType_t start = xTaskGetTickCount();

    while (true)
    {
        if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(1000))
        {
            return false;
        }

        if (manchesterReceiveByte(value, pdMS_TO_TICKS(10)))
        {
            return true;
        }
    }
}

void resetReceptionState(ReceptionContext &context)
{
    context.currentFrame = 0;
    context.expectedTotal = 0;
    context.receptionStarted = false;
    context.error = ErrorCode::COMM_OK;
}

static bool syncToFrameStart(uint8_t &preamble, uint8_t &start)
{
    while (true)
    {
        if (!readByteBlocking(preamble))
        {
            return false;
        }

        if (preamble != preamble_value)
        {
            continue;
        }

        if (!readByteBlocking(start))
        {
            return false;
        }

        if (start == start_value)
        {
            return true;
        }
    }
}

bool receivedFrame(Frame &frame, ReceptionContext &context)
{

    uint8_t byte = 0x00;

    if (!syncToFrameStart(frame.preamble, frame.start))
    {
        return false;
    }

    if (!readByteBlocking(frame.heading.type))
    {
        return false;
    }

    if (!readByteBlocking(frame.heading.sequenceNumber))
    {
        return false;
    }

    if (!readByteBlocking(frame.heading.payloadLength))
    {
        return false;
    }

    if (!readByteBlocking(frame.heading.parameter))
    {
        return false;
    }

    memset(frame.payload, 0, sizeof(frame.payload));

    uint8_t payloadLength = frame.heading.payloadLength;
    if (payloadLength > MAX_PAYLOAD_BYTE_SIZE)
    {
        printf("[alerte] : Michel rejean");
        payloadLength = MAX_PAYLOAD_BYTE_SIZE;
    }

    for (uint8_t index = 0; index < payloadLength; ++index)
    {
        if (!readByteBlocking(frame.payload[index]))
        {
            return false;
        }
    }

    if (!readByteBlocking(byte))
    {
        return false;
    }
    frame.CRC = byte;

    if (!readByteBlocking(byte))
    {
        return false;
    }
    frame.CRC |= ((uint16_t)byte) << 8;

    if (!readByteBlocking(frame.end))
    {
        return false;
    }

    return true;
}
