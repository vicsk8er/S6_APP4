#include "rx_tasks.h"

#include "protocol.h"
#include "receive_frame.h"
#include "send_frame.h"
#include "utils/frame_buffer.h"
#include <stdio.h>

static ReceptionContext rxContext;

void uartRxTask(void *pvParameters)
{
    (void)pvParameters;

    Frame frame;

    while (true)
    {
        if (!receivedFrame(frame, rxContext, Serial1))
        {
            continue;
        }
        // printf("received frame\n");
        ProtocolResult result = processFrame(frame, rxContext);
        // printf("frame type = %d\n", (int8_t)result);
        switch (result)
        {
            case ProtocolResult::ACCEPT:
                storeFrame(frame);
                break;

            case ProtocolResult::NACK:
                // sendNackFrame(rxContext.currentFrame); // À coder
                break;

            case ProtocolResult::REJECT:
            default:
                break;
        }
    }
}