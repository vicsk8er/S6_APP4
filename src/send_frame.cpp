#include "send_frame.h"
#include "manchester/manchester_driver.h"
#include "utils/CRC_calculator.h"
#include "utils/error_injector.h"
#include <cstring>


// bool sendStartFrame(uint8_t nbOfFrame, bool inject_error)
// {
//     current_frame_counter = 0;
//     Frame frame = DEFAULT_FRAME;
//     frame.heading.type = CommunicationType::Start;
//     frame.heading.sequenceNumber = current_frame_counter;
//     frame.heading.payloadLength = 0;
//     frame.heading.parameter = nbOfFrame;
//     frame.CRC = crc_calculator(frame);
//     if(inject_error)
//     {
//         injectError(frame);
//     }
//     return sendFrame(frame);
// }

// bool sendDataFrame(const uint8_t *data, uint8_t length, bool inject_error)
// {
//     Frame frame = DEFAULT_FRAME;
//     frame.heading.type = CommunicationType::Data;
//     frame.heading.sequenceNumber = current_frame_counter;
//     frame.heading.payloadLength = length;
//     frame.heading.parameter = 0;
//     if (length > 0U)
//     {
//         memcpy(frame.payload, data, length);
//     }
//     frame.CRC = crc_calculator(frame);
//     if(inject_error)
//     {
//         injectError(frame);
//     }
//     return sendFrame(frame);
// }

// bool sendEndFrame(bool inject_error)
// {
//     Frame frame = DEFAULT_FRAME;
//     frame.heading.type = CommunicationType::End;
//     frame.heading.sequenceNumber = current_frame_counter;
//     frame.heading.payloadLength = 0;
//     frame.heading.parameter = 0;
//     frame.CRC = crc_calculator(frame);
//     if(inject_error)
//     {
//         injectError(frame);
//     }
//     return sendFrame(frame);
// }

// bool sendNackFrame(uint8_t nbOfFrame, bool inject_error)
// {
//     Frame frame = DEFAULT_FRAME;
//     frame.heading.type = CommunicationType::Nack;
//     frame.heading.sequenceNumber = 0U;
//     frame.heading.payloadLength = 0U;
//     frame.heading.parameter = nbOfFrame;
//     frame.CRC = crc_calculator(frame);
//     if (inject_error)
//     {
//         injectError(frame);
//     }
//     return sendFrame(frame);
// }


bool sendFrame(const Frame &frame)
{
    if (frame.heading.payloadLength > MAX_PAYLOAD_BYTE_SIZE)
    {
        return false;
    }

    if (!manchesterSendBytes(&frame.preamble, 1U)) return false;
    if (!manchesterSendBytes(&frame.start, 1U)) return false;
    if (!manchesterSendBytes(&frame.heading.type, 1U)) return false;
    if (!manchesterSendBytes(&frame.heading.sequenceNumber, 1U)) return false;
    if (!manchesterSendBytes(&frame.heading.payloadLength, 1U)) return false;
    if (!manchesterSendBytes(&frame.heading.parameter, 1U)) return false;
    if (!manchesterSendBytes(frame.payload, frame.heading.payloadLength)) return false;
    const uint8_t crcLow = static_cast<uint8_t>(frame.CRC & 0xFFU);
    const uint8_t crcHigh = static_cast<uint8_t>((frame.CRC >> 8) & 0xFFU);
    if (!manchesterSendBytes(&crcLow, 1U)) return false;
    if (!manchesterSendBytes(&crcHigh, 1U)) return false;
    if (!manchesterSendBytes(&frame.end, 1U)) return false;
    return true;
}
