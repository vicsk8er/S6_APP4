#include "send_frame.h"
// #include "manchester/manchester_driver.h"
#include "manchester/manchester_test.h"
#include "utils/CRC_calculator.h"
#include "utils/error_injector.h"
#include <cstring>

// bool sendFrame(const Frame &frame)
// {
//     if (frame.heading.payloadLength > MAX_PAYLOAD_BYTE_SIZE)
//     {
//         return false;
//     }

//     if (!manchesterSendBytes(&frame.preamble, 1U)) {
//         printf("[SEND_FRAME] Failed to send preamble\n");
//         return false;
//     }
//     if (!manchesterSendBytes(&frame.start, 1U)) {
//         printf("[SEND_FRAME] Failed to send start delimiter\n");
//         return false;
//     }
//     if (!manchesterSendBytes(&frame.heading.type, 1U)) {
//         printf("[SEND_FRAME] Failed to send frame type\n");
//         return false;
//     }
//     if (!manchesterSendBytes(&frame.heading.sequenceNumber, 1U)) {
//         printf("[SEND_FRAME] Failed to send sequence number\n");
//         return false;
//     }
//     if (!manchesterSendBytes(&frame.heading.payloadLength, 1U)) {
//         printf("[SEND_FRAME] Failed to send payload length\n");
//         return false;
//     }
//     if (!manchesterSendBytes(&frame.heading.parameter, 1U)) {
//         printf("[SEND_FRAME] Failed to send parameter\n");
//         return false;
//     }
//     if (!manchesterSendBytes(frame.payload, frame.heading.payloadLength)) {
//         printf("[SEND_FRAME] Failed to send payload\n");
//         return false;
//     }
//     const uint8_t crcLow = static_cast<uint8_t>(frame.CRC & 0xFFU);
//     const uint8_t crcHigh = static_cast<uint8_t>((frame.CRC >> 8) & 0xFFU);
//     if (!manchesterSendBytes(&crcLow, 1U)){
//         printf("[SEND_FRAME] Failed to send CRC low\n");
//         return false;
//     }
//     if (!manchesterSendBytes(&crcHigh, 1U)){
//         printf("[SEND_FRAME] Failed to send CRC high\n");
//         return false;
//     }
//     if (!manchesterSendBytes(&frame.end, 1U)){
//         printf("[SEND_FRAME] Failed to send end delimiter\n");
//         return false;
//     }
//     return true;
// }

bool sendFrame(const Frame &frame)
{
    if (frame.heading.payloadLength > MAX_PAYLOAD_BYTE_SIZE)
    {
        return false;
    }

    if (!testManchesterSendBytes(&frame.preamble, 1U)) {
        printf("[SEND_FRAME] Failed to send preamble\n");
        return false;
    }
    if (!testManchesterSendBytes(&frame.start, 1U)) {
        printf("[SEND_FRAME] Failed to send start delimiter\n");
        return false;
    }
    if (!testManchesterSendBytes(&frame.heading.type, 1U)) {
        printf("[SEND_FRAME] Failed to send frame type\n");
        return false;
    }
    if (!testManchesterSendBytes(&frame.heading.sequenceNumber, 1U)) {
        printf("[SEND_FRAME] Failed to send sequence number\n");
        return false;
    }
    if (!testManchesterSendBytes(&frame.heading.payloadLength, 1U)) {
        printf("[SEND_FRAME] Failed to send payload length\n");
        return false;
    }
    if (!testManchesterSendBytes(&frame.heading.parameter, 1U)) {
        printf("[SEND_FRAME] Failed to send parameter\n");
        return false;
    }
    if (!testManchesterSendBytes(frame.payload, frame.heading.payloadLength)) {
        printf("[SEND_FRAME] Failed to send payload\n");
        return false;
    }
    const uint8_t crcLow = static_cast<uint8_t>(frame.CRC & 0xFFU);
    const uint8_t crcHigh = static_cast<uint8_t>((frame.CRC >> 8) & 0xFFU);
    if (!testManchesterSendBytes(&crcLow, 1U)){
        printf("[SEND_FRAME] Failed to send CRC low\n");
        return false;
    }
    if (!testManchesterSendBytes(&crcHigh, 1U)){
        printf("[SEND_FRAME] Failed to send CRC high\n");
        return false;
    }
    if (!testManchesterSendBytes(&frame.end, 1U)){
        printf("[SEND_FRAME] Failed to send end delimiter\n");
        return false;
    }
    return true;
}

bool sendDebug(){
    uint8_t data[] = {0xAA, 0x55, 0xFF, 0x00};
    if(testManchesterSendBytes(data, sizeof(data))){
        return true;
    }
    return false;
}
