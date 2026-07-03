#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include "utils/config.h"
#include "receive_frame.h"

enum class ProtocolResult
{
    ACCEPT,
    REJECT,
    NACK
};

// bool isFrameValid(const Frame &frame, ReceptionContext &ctx);
bool protocolSendMessage(const uint8_t* data, uint16_t length, bool inject_error); 
bool protocolRetransmit(uint8_t sequence); // NACK
ProtocolResult processFrame(const Frame &frame, ReceptionContext &ctx); // Réception
void performanceTest(); // Throughput measurement

#endif