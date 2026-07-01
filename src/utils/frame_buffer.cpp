#include "frame_buffer.h"

QueueHandle_t RxFrameQueue = nullptr;
QueueHandle_t TxHistoryQueue = nullptr;
QueueHandle_t TxPendingQueue = nullptr;

void initFrameBuffer()
{
    RxFrameQueue = xQueueCreate(MAX_FRAMES, sizeof(Frame));
    TxHistoryQueue = xQueueCreate(MAX_FRAMES, sizeof(Frame));
    TxPendingQueue = xQueueCreate(MAX_FRAMES, sizeof(Frame));
}

bool storeFrame(const Frame &frame)
{
    if (xQueueSend(RxFrameQueue, &frame, 0) != pdTRUE)
    {
        // queue pleine
        return false;
    }
    return true;
}

bool getFrame(Frame &frame)
{
    return xQueueReceive(RxFrameQueue, &frame, portMAX_DELAY) == pdTRUE;
}
