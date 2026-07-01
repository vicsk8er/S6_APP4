#include "CRC_calculator.h"
#include "CRC16.h"

CRC16 crc;

uint16_t crc_calculator(const Frame &frame)
{
    // CRC16 is calculated without the following field: preamble, start, end the CRC himself
    crc.restart();
    crc.add(frame.heading.type);
    crc.add(frame.heading.sequenceNumber);
    crc.add(frame.heading.payloadLength);
    crc.add(frame.heading.parameter);
    
    uint8_t payloadLen = frame.heading.payloadLength;
    if (payloadLen > MAX_PAYLOAD_BYTE_SIZE) payloadLen = MAX_PAYLOAD_BYTE_SIZE;
    crc.add(frame.payload, (crc_size_t)payloadLen);
    return crc.calc();
}