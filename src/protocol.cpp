#include "protocol.h"
#include "utils/CRC_calculator.h"
#include "utils/error_injector.h"
#include "utils/frame_buffer.h"

#include <Arduino.h>
#include <cstring>
#include <math.h>
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

    if (TxPendingQueue)
        ok &= (xQueueSend(TxPendingQueue, &frame, 0) == pdTRUE);

    if (TxHistoryQueue)
        ok &= (xQueueSend(TxHistoryQueue, &frame, 0) == pdTRUE);

    return ok;
}

// static Frame buildStartFrame(uint8_t totalPackets);
// static Frame buildDataFrame(const uint8_t* data,
//                             uint8_t length);
// static Frame buildEndFrame();

static Frame buildStartFrame(uint8_t totalFrames)
{
    txSequence = 0;

    Frame f = DEFAULT_FRAME;
    f.heading.type = CommunicationType::Start;
    f.heading.sequenceNumber = 0;
    f.heading.payloadLength = 0;
    f.heading.parameter = totalFrames;

    f.CRC = crc_calculator(f);

    txSequence = 1;
    return f;
}

static Frame buildDataFrame(const uint8_t* data, uint8_t len)
{
    Frame f = DEFAULT_FRAME;

    f.heading.type = CommunicationType::Data;
    f.heading.sequenceNumber = txSequence++;
    f.heading.payloadLength = len;
    f.heading.parameter = 0;

    memcpy(f.payload, data, len);

    f.CRC = crc_calculator(f);
    return f;
}

static Frame buildEndFrame()
{
    Frame f = DEFAULT_FRAME;

    f.heading.type = CommunicationType::End;
    f.heading.sequenceNumber = txSequence;
    f.heading.payloadLength = 0;
    f.heading.parameter = 0;

    f.CRC = crc_calculator(f);
    return f;
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
    if (!data && length > 0)
        return false;

    uint8_t totalDataFrames =
        (length + MAX_PAYLOAD_BYTE_SIZE - 1) / MAX_PAYLOAD_BYTE_SIZE;

    uint8_t totalFrames = totalDataFrames + 2;

    printf("\n[PROTOCOL] TX start (%u frames)\n", totalFrames);

    enqueueTxFrame(buildStartFrame(totalFrames));

    uint16_t offset = 0;

    while (offset < length)
    {
        uint8_t chunk = fmin((uint16_t)MAX_PAYLOAD_BYTE_SIZE, length - offset);

        Frame f = buildDataFrame(&data[offset], chunk);

        if (inject_error && offset == 0)
        {
            injectError(f);
        }

        enqueueTxFrame(f);

        offset += chunk;
    }

    enqueueTxFrame(buildEndFrame());

    printf("[PROTOCOL] TX queued\n");
    return true;
}

bool protocolRetransmit(uint8_t sequence)
{
    if (!TxHistoryQueue || !TxPendingQueue)
        return false;

    QueueHandle_t temp = xQueueCreate(MAX_FRAMES, sizeof(Frame));
    if (!temp)
        return false;

    bool found = false;
    Frame f;

    while (xQueueReceive(TxHistoryQueue, &f, 0) == pdTRUE)
    {
        if (!found && f.heading.sequenceNumber == sequence)
        {
            found = true;

            f.CRC = crc_calculator(f);
            xQueueSend(TxPendingQueue, &f, 0);
        }

        xQueueSend(temp, &f, 0);
    }

    while (xQueueReceive(temp, &f, 0) == pdTRUE)
    {
        xQueueSend(TxHistoryQueue, &f, 0);
    }

    vQueueDelete(temp);
    return found;
}

ProtocolResult processFrame(const Frame &frame, ReceptionContext &ctx)
{
    CommunicationType type = (CommunicationType)frame.heading.type;

    // CRC déjà validé par receive_frame → sécurité double check légère
    if (frame.heading.payloadLength > MAX_PAYLOAD_BYTE_SIZE)
        return ProtocolResult::REJECT;

    switch (type)
    {
        // =========================
        case CommunicationType::Start:
        // =========================
            resetReceptionState(ctx);

            ctx.receptionStarted = true;
            ctx.expectedTotal = frame.heading.parameter;
            ctx.currentFrame = 1;

            printf("[PROTOCOL] START received (total=%u)\n", ctx.expectedTotal);

            return ProtocolResult::ACCEPT;

        // =========================
        case CommunicationType::Data:
        // =========================
            if (!ctx.receptionStarted)
                return ProtocolResult::REJECT;

            if (frame.heading.sequenceNumber != ctx.currentFrame)
            {
                printf("[PROTOCOL] SEQ ERROR expected=%u got=%u\n",
                       ctx.currentFrame,
                       frame.heading.sequenceNumber);

                return ProtocolResult::NACK;
            }

            ctx.currentFrame++;

            return ProtocolResult::ACCEPT;

        // =========================
        case CommunicationType::End:
        // =========================
            if (!ctx.receptionStarted)
                return ProtocolResult::REJECT;

            printf("[PROTOCOL] END received\n");

            resetReceptionState(ctx);

            return ProtocolResult::ACCEPT;

        // =========================
        case CommunicationType::Nack:
        // =========================
        {
            uint8_t seq = frame.heading.parameter;

            printf("[PROTOCOL] NACK received for %u\n", seq);

            bool ok = protocolRetransmit(seq);

            return ok ? ProtocolResult::ACCEPT : ProtocolResult::REJECT;
        }

        default:
            return ProtocolResult::REJECT;
    }
}

// ========== PERFORMANCE TEST ==========
// Measure maximum throughput on ESP32 data link layer
void performanceTest()
{
    printf("\n\n========== PERFORMANCE TEST ==========\n");
    printf("Measuring ESP32 data link layer throughput\n");
    printf("(Abstraction of RF layer limitations)\n\n");
    
    // Frame overhead: 10 bytes per frame (preamble + start + header(4) + crc + end)
    const uint16_t frameOverhead = 10;
    
    // Test different payload sizes
    uint8_t payloadSizes[] = {1, 10, 20, 40, 80};
    uint32_t totalTime = 0;
    uint32_t totalDataBytes = 0;
    uint8_t numTests = sizeof(payloadSizes) / sizeof(payloadSizes[0]);
    
    printf("Testing %u payload sizes:\n", numTests);
    printf("%-15s %-20s %-20s %-20s\n", "Payload", "Frame Size", "Time (ms)", "Throughput");
    printf("%-15s %-20s %-20s %-20s\n", "-------", "----------", "---------", "----------");
    
    for (uint8_t i = 0; i < numTests; i++)
    {
        uint8_t payloadSize = payloadSizes[i];
        
        // Total frame size: START(10) + DATA(10 + payload) + END(10)
        uint16_t totalFrameSize = frameOverhead + (frameOverhead + payloadSize) + frameOverhead;
        totalDataBytes += payloadSize;  // Count only payload bytes
        
        // Create test data
        uint8_t testData[80];
        for (uint8_t j = 0; j < payloadSize; j++)
        {
            testData[j] = (uint8_t)(j + i);
        }
        
        // Measure transmission time
        uint32_t startTime = millis();
        protocolSendMessage(testData, payloadSize, false);
        uint32_t endTime = millis();
        
        uint32_t transmissionTime = endTime - startTime;
        totalTime += transmissionTime;
        
        // Calculate throughput
        // Serial transmission @ 115200 baud = 115200 bits/sec = 14400 bytes/sec
        // But we measure actual ESP32 processing time
        float throughputBytes = (transmissionTime > 0) ? 
            (float)(payloadSize * 1000) / transmissionTime : 0;
        float throughputKbps = throughputBytes * 8 / 1000;  // Convert to Kbps
        
        printf("%-15u %-20u %-20lu %-20.2f Kbps\n",
            payloadSize, totalFrameSize, transmissionTime, throughputKbps);
        
        delay(100);  // Small delay between tests
    }
    
    // Calculate and display average throughput
    printf("\n========== RESULTS ==========\n");
    printf("Total tests: %u\n", numTests);
    printf("Total time: %lu ms\n", totalTime);
    printf("Total payload data: %lu bytes\n", totalDataBytes);
    
    if (totalTime > 0)
    {
        float avgThroughputBytes = (float)(totalDataBytes * 1000) / totalTime;
        float avgThroughputKbps = avgThroughputBytes * 8 / 1000;
        float avgThroughputMbps = avgThroughputKbps / 1000;
        
        printf("\nAverage Throughput:\n");
        printf("  %.2f bytes/sec\n", avgThroughputBytes);
        printf("  %.2f Kbps (Kilobits/sec)\n", avgThroughputKbps);
        printf("  %.3f Mbps (Megabits/sec)\n", avgThroughputMbps);
        
        // Theoretical maximum with 115200 baud UART
        printf("\nComparison with UART capacity:\n");
        printf("  UART max capacity: 115200 bps = 14400 bytes/sec\n");
        printf("  ESP32 data link layer efficiency: %.1f%%\n", 
            (avgThroughputBytes / 14400) * 100);
    }
    
    printf("\n=====================================\n\n");
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