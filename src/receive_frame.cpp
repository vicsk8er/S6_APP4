#include "receive_frame.h"
// #include "manchester/manchester_driver.h"
#include "manchester/manchester_test.h"
#include "utils/CRC_calculator.h"
#include <cstring>

// =====================================================
// INTERNAL RX STATE MACHINE
// =====================================================

enum class RxState
{
    SEARCH_PREAMBLE,
    SEARCH_START,
    READ_HEADER,
    READ_PAYLOAD,
    READ_CRC_LOW,
    READ_CRC_HIGH,
    READ_END
};

static RxState state = RxState::SEARCH_PREAMBLE;

// =====================================================
// INTERNAL PARSER STATE
// =====================================================

static Frame frame;
static uint8_t headerIndex = 0;
static uint8_t payloadIndex = 0;
static uint16_t crc = 0;

// =====================================================
// RESET
// =====================================================

void resetReceptionState(ReceptionContext &context)
{
    context.currentFrame = 0;
    context.expectedTotal = 0;
    context.receptionStarted = false;
    context.error = ErrorCode::COMM_OK;

    state = RxState::SEARCH_PREAMBLE;

    headerIndex = 0;
    payloadIndex = 0;
    crc = 0;

    memset(&frame, 0, sizeof(frame));
}

// =====================================================
// BYTE INPUT
// =====================================================

static bool getByte(uint8_t &b)
{
    return testManchesterReceiveByte(b, pdMS_TO_TICKS(20));
}

// =====================================================
// MAIN RX FUNCTION
// =====================================================

bool receivedFrame(Frame &outFrame, ReceptionContext &context)
{
    uint8_t byte;

    while (getByte(byte))
    {
        // printf("RX byte : 0x%02X\n", byte);
        switch (state)
        {
            // ---------------------------------------------
            case RxState::SEARCH_PREAMBLE:
            // ---------------------------------------------
                if (byte == preamble_value)
                {
                    // printf("[RX] Preamble detected\n");
                    frame.preamble = byte;
                    state = RxState::SEARCH_START;
                }
                break;

            // ---------------------------------------------
            case RxState::SEARCH_START:
            // ---------------------------------------------
                if (byte == start_value)
                {
                    //printf("[RX] Start detected\n");
                    frame.start = byte;
                    state = RxState::READ_HEADER;
                    headerIndex = 0;
                }
                else
                {
                    state = RxState::SEARCH_PREAMBLE;
                }
                break;

            // ---------------------------------------------
            case RxState::READ_HEADER:
            // ---------------------------------------------
                ((uint8_t*)&frame.heading)[headerIndex++] = byte;

                if (headerIndex >= sizeof(frame.heading))
                {
                    //printf("[RX] Header received: type=%u seq=%u payloadLen=%u param=%u\n",
                    //     frame.heading.type,
                    //     frame.heading.sequenceNumber,
                    //     frame.heading.payloadLength,
                    //     frame.heading.parameter);
                    payloadIndex = 0;
                    if(frame.heading.type == (uint8_t)CommunicationType::Data && frame.heading.payloadLength > 0)
                        state = RxState::READ_PAYLOAD;
                    else{
                        state = RxState::READ_CRC_LOW;
                    }
                }
                else{
                    //printf("[RX] Reading header...\n");
                    state = RxState::READ_HEADER;
                }
                break;

            // ---------------------------------------------
            case RxState::READ_PAYLOAD:
            // ---------------------------------------------
            {
                uint8_t len = frame.heading.payloadLength;

                if (len > MAX_PAYLOAD_BYTE_SIZE)
                    len = MAX_PAYLOAD_BYTE_SIZE;

                frame.payload[payloadIndex++] = byte;

                if (payloadIndex >= len)
                {
                    //printf("[RX] Payload received (%u bytes)\n", len);
                    state = RxState::READ_CRC_LOW;
                }
                break;
            }

            // ---------------------------------------------
            case RxState::READ_CRC_LOW:
                crc = byte;
                state = RxState::READ_CRC_HIGH;
                break;

            case RxState::READ_CRC_HIGH:
                crc |= ((uint16_t)byte << 8);
                state = RxState::READ_END;
                break;

            // ---------------------------------------------
            case RxState::READ_END:
            // ---------------------------------------------
                if (byte != end_value)
                {
                    printf("[RX] End value error\n");
                    state = RxState::SEARCH_PREAMBLE;
                    context.error = ErrorCode::ERR_VALUE_FIELD;
                    break;
                }
                // printf("[RX] End detected\n");

                frame.end = byte;

                // =========================
                // CRC CHECK
                // =========================
                if (crc_calculator(frame) != crc)
                {
                    printf("[RX] CRC error\n");
                    context.error = ErrorCode::ERR_CRC;
                    state = RxState::SEARCH_PREAMBLE;
                    break;
                }
                frame.CRC = crc;

                // =========================
                // OUTPUT FRAME
                // =========================
                //printf("[RX] Frame received\n");
                outFrame = frame;

                // =========================
                // UPDATE CONTEXT (minimal RX only)
                // =========================
                context.receptionStarted = true;

                // reset parser for next frame
                state = RxState::SEARCH_PREAMBLE;
                headerIndex = 0;
                payloadIndex = 0;
                crc = 0;

                return true;
        }
        byte = 0;
    }

    return false;
}