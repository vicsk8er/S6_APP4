#include "tx_tasks.h"
#include "protocol.h"
#include "send_frame.h"
#include "utils/frame_buffer.h"
#include <cstring>
#include <stdio.h>

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
		bool frameSent = sendFrame(frame);
		printf("[SEND_FRAME] Frame sent: type=%u seq=%u payloadLen=%u, succeed=%s\n", frame.heading.type,
           frame.heading.sequenceNumber,
           frame.heading.payloadLength,
		   frameSent ? "SUCCESS" : "FAILURE");
		vTaskDelay(pdMS_TO_TICKS(5));  // Petite pause pour éviter de saturer le bus

		// bool debugSent = sendDebug();
		// printf("[SEND_FRAME] Debug data sent: succeed=%s\n", debugSent ? "SUCCESS" : "FAILURE");
	}
}

extern volatile uint8_t buttonTrigger;  // 0 = menu, 1 = button

void uartTxTask(void *pvParameters)
{
	(void)pvParameters;

	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		
		// Appeler sendButtonMessage() seulement si c'est le bouton qui a déclenché
		if (buttonTrigger == 1)
		{
			sendButtonMessage();
		}
		
		transmitQueuedFrames();
	}
}
