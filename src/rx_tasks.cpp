#include "rx_tasks.h"

#include "protocol.h"
#include "receive_frame.h"
#include "send_frame.h"
#include "utils/frame_buffer.h"
#include "utils/timer.h"
#include <stdio.h>

static ReceptionContext rxContext;

void uartRxTask(void *pvParameters)
{
    (void)pvParameters;

    Frame frame = DEFAULT_FRAME;

    resetReceptionState(rxContext);

    while (true)
    {
        bool successReceivedFrame = receivedFrame(frame, rxContext);
        if (!successReceivedFrame)
        {
            continue;
        }
        stopTimer(frame.heading.sequenceNumber);
        printf("received frame\n");
        ProtocolResult result = processFrame(frame, rxContext);
        // printf("frame type = %d\n", (int8_t)result);
        switch (result)
        {
            case ProtocolResult::ACCEPT:
                // Store only DATA frames, not START/END/NACK
                if (frame.heading.type == 0x02)  // DATA frame
                {
                    storeFrame(frame, getElapsedTime(frame.heading.sequenceNumber));
                }
                else if(frame.heading.type == 0x03) // END frame
                {
                    // we have to flush the history queue once the END frame is received
                    flushTxHistory();
                }
                break;

            case ProtocolResult::NACK:
                printf("\n[RX] NACK sent for frame #%u\n", rxContext.currentFrame);
                sendNackFrame(rxContext.currentFrame);
                break;

            case ProtocolResult::REJECT:
            default:
                break;
        }
    }
}

