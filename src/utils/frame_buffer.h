#ifndef FRAME_BUFFER_H_
#define FRAME_BUFFER_H_
#include "utils/config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define MAX_FRAMES 128

// Structure pour stocker une frame avec ses métadonnées
struct FrameWithMetadata
{
    Frame frame;
    uint32_t timestamp;  // Timestamp en millisecondes
    uint32_t frameNumber; // Numéro séquentiel de la frame
};

extern QueueHandle_t RxFrameQueue;
extern QueueHandle_t TxHistoryQueue;//sert au retransmission (quand il y a un NACK)
extern QueueHandle_t TxPendingQueue; //contient les trames qui attende d'être envoyé dans une transmission normale

void initFrameBuffer();

bool storeFrame(const Frame &frame);

bool getFrame(Frame &frame);

void displayReceivedFrames();

void clearRxQueue();


#endif