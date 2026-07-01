#ifndef FRAME_BUFFER_H_
#define FRAME_BUFFER_H_
#include "utils/config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define MAX_FRAMES 128
// struct FrameBuffer
// {
//     Frame frames[MAX_FRAMES];
//     uint8_t writeIndex = 0;
//     uint8_t readIndex = 0;
//     uint8_t count = 0;
// };

// void storeFrame(FrameBuffer &buf, const Frame &frame);
// bool getFrame(FrameBuffer &buf, uint8_t seq, Frame &frame);


extern QueueHandle_t RxFrameQueue;
extern QueueHandle_t TxHistoryQueue;//sert au retransmission (quand il y a un NACK)
extern QueueHandle_t TxPendingQueue; //contient les trames qui attende d'être envoyé dans une transmission normale

void initFrameBuffer();

bool storeFrame(const Frame &frame);

bool getFrame(Frame &frame);


#endif