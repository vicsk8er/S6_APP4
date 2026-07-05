#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include "utils/config.h"
#include "receive_frame.h"
#include <stdint.h>

enum class ProtocolResult
{
    ACCEPT,
    REJECT,
    NACK
};

// Main TX entry point
bool protocolSendMessage(const uint8_t* data, uint16_t length, bool inject_error);

// Retransmission on NACK request
bool protocolRetransmit(uint8_t sequence);

// RX processing (core logic)
ProtocolResult processFrame(const Frame &frame, ReceptionContext &ctx);

// Debug / test
void performanceTest();
void sendNackFrame(uint8_t currentFrame);
bool flushTxHistory();

#endif