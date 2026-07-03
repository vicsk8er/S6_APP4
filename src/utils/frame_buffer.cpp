#include "frame_buffer.h"
#include <Arduino.h>
#include <stdio.h>
#include <time.h>

QueueHandle_t RxFrameQueue = nullptr;
QueueHandle_t TxHistoryQueue = nullptr;
QueueHandle_t TxPendingQueue = nullptr;

static uint32_t frameCounter = 0;  // Compteur global de frames

void initFrameBuffer()
{
    RxFrameQueue = xQueueCreate(MAX_FRAMES, sizeof(FrameWithMetadata));
    TxHistoryQueue = xQueueCreate(MAX_FRAMES, sizeof(Frame));
    TxPendingQueue = xQueueCreate(MAX_FRAMES, sizeof(Frame));
    frameCounter = 0;
}

bool storeFrame(const Frame &frame)
{
    FrameWithMetadata frameData;
    frameData.frame = frame;
    frameData.timestamp = time(nullptr);  // Unix timestamp avec secondes
    frameData.frameNumber = frameCounter++;
    
    if (xQueueSend(RxFrameQueue, &frameData, 0) != pdTRUE)
    {
        // queue pleine
        return false;
    }
    return true;
}

bool getFrame(Frame &frame)
{
    FrameWithMetadata frameData;
    if (xQueueReceive(RxFrameQueue, &frameData, portMAX_DELAY) == pdTRUE)
    {
        frame = frameData.frame;
        return true;
    }
    return false;
}

void displayReceivedFrames()
{
    FrameWithMetadata frameData;
    uint32_t frameCount = uxQueueMessagesWaiting(RxFrameQueue);
    
    if (frameCount == 0)
    {
        Serial.println("\n>>> Queue vide - Aucune frame reçue\n");
        return;
    }
    
    Serial.printf("\n>>> DATA frames reçues:\n");
    Serial.println("========================================");
    
    uint32_t displayedCount = 0;
    while (xQueueReceive(RxFrameQueue, &frameData, 0) == pdTRUE)
    {
        // Afficher seulement les DATA frames avec payload
        if (frameData.frame.heading.type == 0x02 && frameData.frame.heading.payloadLength > 0)
        {
            Serial.printf("\nFrame #%lu | Timestamp: %lu ms | Seq: %u | PayloadLen: %u\n",
                frameData.frameNumber, 
                frameData.timestamp,
                frameData.frame.heading.sequenceNumber,
                frameData.frame.heading.payloadLength);
            
            Serial.print("  Payload: ");
            for (uint8_t i = 0; i < frameData.frame.heading.payloadLength; i++)
            {
                Serial.write(frameData.frame.payload[i]);
            }
            Serial.println();
            Serial.printf("  CRC: 0x%04X\n", frameData.frame.CRC);
            displayedCount++;
        }
    }
    
    if (displayedCount == 0)
    {
        Serial.println("  Aucune DATA frame avec payload trouvée");
    }
    
    Serial.println("========================================\n");
  }

void clearRxQueue()
{
    FrameWithMetadata frameData;
    uint32_t clearedCount = 0;
    
    while (xQueueReceive(RxFrameQueue, &frameData, 0) == pdTRUE)
    {
        clearedCount++;
    }
    
    if (clearedCount == 0)
    {
        Serial.println("\n>>> Queue déjà vide\n");
    }
    else
    {
        Serial.printf("\n>>> Queue vidée - %lu frames supprimées\n\n", clearedCount);
    }
}
