#include "protocol.h"
#include "utils/CRC_calculator.h"
#include "utils/frame_buffer.h"
#include "utils/error_injector.h"
#include "utils/error_code.h"
#include <math.h>
#include <cstring>

static uint8_t txSequence = 0;

const char* communicationTypeToString(CommunicationType type)
{
    switch (type)
    {
        case CommunicationType::Start:
            return "START";

        case CommunicationType::Data:
            return "DATA";

        case CommunicationType::End:
            return "END";

        case CommunicationType::Nack:
            return "NACK";

        default:
            return "UNKNOWN_TYPE";
    }
}

const char* protocolResultToString(ProtocolResult protocolResult)
{
    switch (protocolResult)
    {
        case ProtocolResult::ACCEPT:
            return "ACCEPT";

        case ProtocolResult::NACK:
            return "NACK";

        case ProtocolResult::REJECT:
            return "REJECT";
        
        default:
            return "UNKNOWN_RESULT";
    }
}

const char* errorCodeToString(ErrorCode error)
{
    switch (error)
    {
        case COMM_OK:
            return "COMM_OK";

        case ERR_CRC:
            return "ERR_CRC";

        case ERR_SEQUENCE:
            return "ERR_SEQUENCE";

        case ERR_LENGTH:
            return "ERR_LENGTH";

        case ERR_TYPE:
            return "ERR_TYPE";

        case ERR_TIMEOUT:
            return "ERR_TIMEOUT";

        case ERR_BUFFER_FULL:
            return "ERR_BUFFER_FULL";

        case ERR_NOT_INITIALIZED:
            return "ERR_NOT_INITIALIZED";

        case ERR_UNEXPECTED_END:
            return "ERR_UNEXPECTED_END";

        case ERR_VALUE_FIELD:
            return "ERR_VALUE_FIELD";

        default:
            return "UNKNOWN_ERROR";
    }
}

static bool enqueueTxFrame(const Frame &frame)
{
    bool ok = true;

    if (TxPendingQueue != nullptr)
    {
        ok = (xQueueSend(TxPendingQueue, &frame, 0) == pdTRUE) && ok;
    }

    if (TxHistoryQueue != nullptr)
    {
        ok = (xQueueSend(TxHistoryQueue, &frame, 0) == pdTRUE) && ok;
    }

    return ok;
}

// static Frame buildStartFrame(uint8_t totalPackets);
// static Frame buildDataFrame(const uint8_t* data,
//                             uint8_t length);
// static Frame buildEndFrame();

static Frame buildStartFrame(uint8_t nbOfFrame, bool inject_error)
{
    txSequence = 0;
    Frame frame = DEFAULT_FRAME;
    frame.heading.type = CommunicationType::Start;
    frame.heading.sequenceNumber = txSequence;
    frame.heading.payloadLength = 0;
    frame.heading.parameter = nbOfFrame;
    frame.CRC = crc_calculator(frame);
    if(inject_error)
    {
        injectError(frame);
    }
    txSequence = 1;
    return frame;
}

static Frame buildDataFrame(const uint8_t *data, uint8_t length, bool inject_error)
{
    Frame frame = DEFAULT_FRAME;
    frame.heading.type = CommunicationType::Data;
    frame.heading.sequenceNumber = txSequence;
    frame.heading.payloadLength = length;
    frame.heading.parameter = 0;
    if (length > 0U)
    {
        memcpy(frame.payload, data, length);
    }
    frame.CRC = crc_calculator(frame);
    if(inject_error)
    {
        injectError(frame);
    }
    return frame;
}

static Frame buildEndFrame(bool inject_error)
{
    Frame frame = DEFAULT_FRAME;
    frame.heading.type = CommunicationType::End;
    frame.heading.sequenceNumber = txSequence;
    frame.heading.payloadLength = 0;
    frame.heading.parameter = 0;
    frame.CRC = crc_calculator(frame);
    if(inject_error)
    {
        injectError(frame);
    }
    return frame;
}

static void setError(ReceptionContext &context, ErrorCode code)
{
    context.error = code;
}

// ---------------- CRC + format ----------------
static bool isFrameValid(const Frame &frame, ReceptionContext &ctx)
{
    if (frame.preamble != preamble_value ||
        frame.start != start_value ||
        frame.end != end_value)
    {
        setError(ctx, ErrorCode::ERR_VALUE_FIELD);
        return false;
    }

    if (frame.heading.payloadLength > MAX_PAYLOAD_BYTE_SIZE)
    {
        setError(ctx, ErrorCode::ERR_LENGTH);
        return false;
    }

    if (frame.heading.type < (uint8_t)CommunicationType::Start ||
        frame.heading.type > (uint8_t)CommunicationType::Nack)
    {
        setError(ctx, ErrorCode::ERR_TYPE);
        return false;
    }

    if ( (frame.heading.type == (uint8_t)CommunicationType::Start ||
          frame.heading.type == (uint8_t)CommunicationType::End   ||
          frame.heading.type == (uint8_t)CommunicationType::Nack) && 
          frame.heading.payloadLength != 0U)
    {
        setError(ctx, ErrorCode::ERR_LENGTH);
        return false;
    }

    if (frame.CRC != crc_calculator(frame))
    {
        setError(ctx, ErrorCode::ERR_CRC);
        return false;
    }

    setError(ctx, ErrorCode::COMM_OK);
    return true;
}

bool protocolSendMessage(const uint8_t* data, uint16_t length, bool inject_error)
{
    if (data == nullptr && length > 0U)
    {
        return false;
    }

    uint8_t totalPackets =
        (length + MAX_PAYLOAD_BYTE_SIZE - 1)
        / MAX_PAYLOAD_BYTE_SIZE;
    totalPackets += 2; // Car il y a un start et un end
    if (!enqueueTxFrame(buildStartFrame(totalPackets, false)))
    {
        return false;
    }

    uint16_t offset = 0;
    bool injected = false;

    while (offset < length)
    {
        uint8_t currentSize =
            fmin(MAX_PAYLOAD_BYTE_SIZE,
                length - offset);

        bool mustInject = inject_error && !injected && (random(0, totalPackets == 0 ? 1 : totalPackets) == 0); // a valider

        if (!enqueueTxFrame(
                buildDataFrame(
                    &data[offset],
                    currentSize,
                    mustInject)))
        {
            return false;
        }

        injected = injected || mustInject;

        offset += currentSize;
        ++txSequence;
    }

    if (!enqueueTxFrame(buildEndFrame(false)))
    {
        return false;
    }

    return true;
}

bool protocolRetransmit(uint8_t sequence)
{
    if (TxHistoryQueue == nullptr || TxPendingQueue == nullptr)
    {
        return false;
    }

    QueueHandle_t tempQueue = xQueueCreate(MAX_FRAMES, sizeof(Frame));
    if (tempQueue == nullptr)
    {
        return false;
    }

    bool found = false;
    Frame frame = DEFAULT_FRAME;

    while (xQueueReceive(TxHistoryQueue, &frame, 0) == pdTRUE)
    {
        if (!found && frame.heading.sequenceNumber == sequence)
        {
            found = true;
            xQueueSend(TxPendingQueue, &frame, 0);
            xQueueSend(tempQueue, &frame, 0);
            continue;
        }

        xQueueSend(tempQueue, &frame, 0);
    }

    while (xQueueReceive(tempQueue, &frame, 0) == pdTRUE)
    {
        xQueueSend(TxHistoryQueue, &frame, 0);
    }

    vQueueDelete(tempQueue);
    return found;
}

ProtocolResult processFrame(const Frame &frame, ReceptionContext &ctx)
{
    CommunicationType type = static_cast<CommunicationType>(frame.heading.type);

    ProtocolResult result = ProtocolResult::REJECT;
    bool resetAfterLog = false;

    if (!isFrameValid(frame, ctx))
    {
        result = ProtocolResult::REJECT;
    }
    else
    {
        switch (type)
        {
            case CommunicationType::Start:

                if (frame.heading.sequenceNumber != 0U)
                {
                    setError(ctx, ErrorCode::ERR_SEQUENCE);
                    result = ProtocolResult::REJECT;
                    break;
                }

                resetReceptionState(ctx);
                ctx.receptionStarted = true;
                ctx.expectedTotal = frame.heading.parameter;
                ctx.currentFrame = 1U;

                result = ProtocolResult::ACCEPT;
                break;

            case CommunicationType::Data:

                if (!ctx.receptionStarted)
                {
                    setError(ctx, ErrorCode::ERR_NOT_INITIALIZED);
                    result = ProtocolResult::REJECT;
                    break;
                }

                if (frame.heading.sequenceNumber != ctx.currentFrame)
                {
                    setError(ctx, ErrorCode::ERR_SEQUENCE);
                    result = ProtocolResult::NACK;
                    break;
                }

                ctx.currentFrame++;
                result = ProtocolResult::ACCEPT;
                break;

            case CommunicationType::End:

                if (!ctx.receptionStarted)
                {
                    setError(ctx, ErrorCode::ERR_NOT_INITIALIZED);
                    result = ProtocolResult::REJECT;
                    break;
                }

                if (frame.heading.sequenceNumber != ctx.currentFrame)
                {
                    setError(ctx, ErrorCode::ERR_SEQUENCE);
                    result = ProtocolResult::NACK;
                    break;
                }

                if (ctx.expectedTotal != 0U &&
                    frame.heading.sequenceNumber != ctx.expectedTotal - 1U)
                {
                    setError(ctx, ErrorCode::ERR_UNEXPECTED_END);
                    result = ProtocolResult::REJECT;
                    break;
                }
                
                ctx.currentFrame++;
                result = ProtocolResult::ACCEPT;
                resetAfterLog = true;
                break;

            case CommunicationType::Nack:// TODO: Définir behavior NACK
            default:

                setError(ctx, ErrorCode::ERR_TYPE);
                result = ProtocolResult::REJECT;
                break;
        }
    }

    Serial.println();
    Serial.println("========== RX FRAME ==========");
    Serial.printf("Type            : %s\n", communicationTypeToString(type));
    Serial.printf("Sequence number : %u\n", frame.heading.sequenceNumber);
    Serial.printf("Payload length  : %u\n", frame.heading.payloadLength);
    Serial.printf("Parameter       : %u\n", frame.heading.parameter);
    Serial.printf("CRC             : 0x%04X\n", frame.CRC);
    Serial.printf("Expected frame  : %u\n", ctx.currentFrame);
    Serial.printf("Expected total  : %u\n", ctx.expectedTotal);
    Serial.printf("Error           : %s\n", errorCodeToString(ctx.error));
    Serial.printf("Result          : %s\n", protocolResultToString(result));
    Serial.println("==============================");

    if (resetAfterLog)
    {
        resetReceptionState(ctx);
    }

    return result;
}







// ProtocolResult processFrame(const Frame &frame, ReceptionContext &ctx)
// {
//     if (!isFrameValid(frame, ctx))
//         return ProtocolResult::REJECT;

//     CommunicationType type = (CommunicationType)frame.heading.type;

//     switch (type)
//     {
//         case CommunicationType::Start:
//             if (frame.heading.sequenceNumber != 0)
//                 return ProtocolResult::REJECT;

//             resetReceptionState(ctx);
//             ctx.receptionStarted = true;
//             ctx.expectedTotal = frame.heading.parameter;
//             ctx.currentFrame = 1;

//             return ProtocolResult::ACCEPT;

//         case CommunicationType::Data:
//             if (!ctx.receptionStarted)
//                 return ProtocolResult::REJECT;

//             if (frame.heading.sequenceNumber != ctx.currentFrame)
//                 return ProtocolResult::NACK;

//             ctx.currentFrame++;
//             return ProtocolResult::ACCEPT;

//         case CommunicationType::End:
//             if (!ctx.receptionStarted)
//                 return ProtocolResult::REJECT;

//             if (frame.heading.sequenceNumber != ctx.currentFrame)
//                 return ProtocolResult::NACK;

//             resetReceptionState(ctx);
//             return ProtocolResult::ACCEPT;

//         case CommunicationType::Nack:
//         default:
//             return ProtocolResult::REJECT;
//     }
// }