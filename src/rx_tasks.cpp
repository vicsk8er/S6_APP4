#include "rx_tasks.h"

#include "protocol.h"
#include "receive_frame.h"
#include "send_frame.h"
#include "utils/frame_buffer.h"
#include "manchester/manchester_driver.h"
#include <stdio.h>

static ReceptionContext rxContext;

void uartRxTask(void *pvParameters)
{
    (void)pvParameters;

    Frame frame = DEFAULT_FRAME;

    resetReceptionState(rxContext);

    while (true)
    {
        if (!receivedFrame(frame, rxContext))
        {
            continue;
        }
        printf("received frame\n");
        ProtocolResult result = processFrame(frame, rxContext);
        // printf("frame type = %d\n", (int8_t)result);
        switch (result)
        {
            case ProtocolResult::ACCEPT:
                // Store only DATA frames, not START/END/NACK
                if (frame.heading.type == 0x02)  // DATA frame
                {
                    storeFrame(frame);
                }
                break;

            case ProtocolResult::NACK:
                sendNackFrame(rxContext.currentFrame);
                break;

            case ProtocolResult::REJECT:
            default:
                break;
        }
    }
}

void sendNackFrame(uint8_t currentFrame)
{
    Frame nackFrame;
    nackFrame.preamble = 0x55;
    nackFrame.start = 0x7E;
    nackFrame.heading.type = 0x04;
    nackFrame.heading.sequenceNumber = 0;  
    nackFrame.heading.payloadLength = 0;
    nackFrame.heading.parameter = currentFrame;  // Numéro du paquet à renvoyer
    nackFrame.CRC = crc_calculator(nackFrame);
    nackFrame.end = 0x7E;

    sendFrame(nackFrame);
}