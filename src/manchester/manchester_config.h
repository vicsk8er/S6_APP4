#ifndef MANCHESTER_CONFIG_H_
#define MANCHESTER_CONFIG_H_

#include <stdint.h>

static constexpr uint8_t MANCHESTER_RX_PIN = 12U;
static constexpr uint8_t MANCHESTER_TX_PIN = 14U;
static constexpr uint32_t MANCHESTER_BIT_RATE = 2000U;
static constexpr uint32_t MANCHESTER_BIT_PERIOD_US = 1000000UL / MANCHESTER_BIT_RATE;
static constexpr uint32_t MANCHESTER_HALF_BIT_US = MANCHESTER_BIT_PERIOD_US / 2UL;
static constexpr uint16_t MANCHESTER_EDGE_QUEUE_LENGTH = 512U;
static constexpr uint16_t MANCHESTER_RX_BYTE_QUEUE_LENGTH = 64U;
static constexpr uint8_t MANCHESTER_SYNC_PREAMBLE_BYTES = 4U;

#endif