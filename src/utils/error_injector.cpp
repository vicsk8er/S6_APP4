#include "error_injector.h"
#include <string.h>
#include <math.h>

// static uint8_t pickRandomHeaderOffset()
// {
//     return (uint8_t)(2U + random(0, 4));
// }

void injectError(Frame &frame)// probléme avec les random ici
{
    frame.payload[0] ^= 0xFF;
    // if (frame.heading.payloadLength > 0U)
    // {
    //     uint8_t index = (uint8_t)random( 0U, (uint8_t)frame.heading.payloadLength);
    //     frame.payload[index] ^= (uint8_t)(1U << random(0, 8));
    //     return;
    // }

    // uint8_t *raw = reinterpret_cast<uint8_t *>(&frame);
    // raw[pickRandomHeaderOffset()] ^= (uint8_t)(1U << random(0, 8));
}