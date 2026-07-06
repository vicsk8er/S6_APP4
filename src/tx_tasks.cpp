#include "tx_tasks.h"
#include "protocol.h"
#include "send_frame.h"
#include "utils/frame_buffer.h"
#include "manchester/manchester_config.h"
#include <cstring>
#include <stdio.h>
#include "utils/timer.h"

TaskHandle_t txTaskHandle = nullptr;

void wakeTxTask()
{
	vTaskDelay(pdMS_TO_TICKS(MANCHESTER_DELAY_BETWEEN_FRAMES_MS));  // Petite pause pour éviter de saturer le bus
	xTaskNotifyGive(txTaskHandle);
}

static void sendButtonMessage()
{
    const char *message = "Appui bouton GPIO0";
    uint8_t length = (uint8_t)strlen(message);

    if (length == 0U || length > (MAX_PAYLOAD_BYTE_SIZE*255)) // le protocol est prévue pour envoyé un max de 255 trame de suite
    {
        return;
    }

	printf("send message\n");
    protocolSendMessage(reinterpret_cast<const uint8_t *>(message), length, false);
}

static void transmitQueuedFrames()
{
	if (TxPendingQueue == nullptr)
	{
		return;
	}

	Frame frame;
	while (xQueueReceive(TxPendingQueue, &frame, 0) == pdTRUE)
	{
		if(frame.heading.type == (uint8_t)CommunicationType::Start){
			clearAllTimers();
		}
		
		startTimer(static_cast<CommunicationType>(frame.heading.type), frame.heading.sequenceNumber);
		bool frameSent = sendFrame(frame);
		
		printf("[SEND_FRAME] Frame sent: type=%u seq=%u payloadLen=%u, transmission status=%s\n", frame.heading.type,
           frame.heading.sequenceNumber,
           frame.heading.payloadLength,
		   frameSent ? "SUCCESS" : "FAILURE");
		vTaskDelay(pdMS_TO_TICKS(MANCHESTER_DELAY_BETWEEN_FRAMES_MS));  // Petite pause pour éviter de saturer le bus
	}
}

void uartTxTask(void *pvParameters)
{
	(void)pvParameters;

	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);		
		transmitQueuedFrames();
	}
}
