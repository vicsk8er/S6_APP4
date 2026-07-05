#ifndef MANCHESTER_CONFIG_H_
#define MANCHESTER_CONFIG_H_

#include <stdint.h>

static constexpr uint8_t MANCHESTER_RX_PIN = 12U;
static constexpr uint8_t MANCHESTER_TX_PIN = 14U;
static constexpr uint32_t MANCHESTER_BIT_RATE = 3000U; //MAX bit rate
static constexpr uint32_t MANCHESTER_HALF_BIT_US = (1000000UL / MANCHESTER_BIT_RATE) / 2UL;
static constexpr uint32_t MANCHESTER_QUARTER_BIT_US = MANCHESTER_HALF_BIT_US / 2UL;

#endif