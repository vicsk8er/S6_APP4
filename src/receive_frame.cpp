#include "receive_frame.h"

#include "utils/CRC_calculator.h"
#include <cstring>

static ReceptionContext rxContext;

static bool readByteBlocking(HardwareSerial &uart, uint8_t &value)
{
    TickType_t start = xTaskGetTickCount();

    while (uart.available() == 0)
    {
        if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(1000))
        {
            return false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    int readValue = uart.read();
    if (readValue < 0)
    {
        return false;
    }

    value = (uint8_t)readValue;
    return true;
}

void resetReceptionState(ReceptionContext &context)
{
    context.currentFrame = 0;
    context.expectedTotal = 0;
    context.receptionStarted = false;
    context.error = ErrorCode::COMM_OK;
}

bool receivedFrame(Frame &frame, ReceptionContext &context, HardwareSerial &uart)
{

    uint8_t byte = 0x00;

    do
    {
        if (!readByteBlocking(uart, byte))
        {
            return false;
        }
    } while (byte != preamble_value);

    frame.preamble = byte;

    do
    {
        if (!readByteBlocking(uart, byte))
        {
            return false;
        }
    } while (byte != start_value);

    frame.start = byte;

    if (!readByteBlocking(uart, frame.heading.type))
    {
        return false;
    }

    if (!readByteBlocking(uart, frame.heading.sequenceNumber))
    {
        return false;
    }

    if (!readByteBlocking(uart, frame.heading.payloadLength))
    {
        return false;
    }

    if (!readByteBlocking(uart, frame.heading.parameter))
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
        if (!readByteBlocking(uart, frame.payload[index]))
        {
            return false;
        }
    }

    if (!readByteBlocking(uart, byte))
    {
        return false;
    }
    frame.CRC = byte;

    if (!readByteBlocking(uart, byte))
    {
        return false;
    }
    frame.CRC |= ((uint16_t)byte) << 8;

    if (!readByteBlocking(uart, frame.end))
    {
        return false;
    }

    return true;
}
