#include "manchester_driver.h"

#include "manchester_config.h"
#include "utils/config.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <Arduino.h>
#include "driver/rmt_rx.h"
#include "driver/rmt_tx.h"
#include "driver/uart.h"

static rmt_channel_handle_t rxChannel = nullptr;
static rmt_receive_config_t rxConfig = {};
static rmt_symbol_word_t rmtBuffer[64];
static rmt_channel_handle_t txChannel = nullptr;
static rmt_encoder_handle_t copyEncoder = nullptr;

static QueueHandle_t rxByteQueue = nullptr;
static TaskHandle_t rxDecodeTaskHandle = nullptr;
static uint8_t configuredRxPin = MANCHESTER_RX_PIN;
static uint8_t configuredTxPin = MANCHESTER_TX_PIN;
static uint32_t configuredHalfBitUs = MANCHESTER_HALF_BIT_US;
static bool driverStarted = false;
static volatile bool rxReceiving = false;


static bool rmt_rx_done_cb(
    rmt_channel_handle_t channel,
    const rmt_rx_done_event_data_t *edata,
    void *user_data)
{
    BaseType_t hpTaskWoken = pdFALSE;

    static uint8_t currentByte = 0;
    static uint8_t bitCount = 0;

    (void)user_data;

    auto decodeManchesterSymbol = [](const rmt_symbol_word_t &symbol, uint8_t &bit) -> bool {
        if (symbol.level0 == 1 && symbol.level1 == 0)
        {
            bit = 1U;
            return true;
        }

        if (symbol.level0 == 0 && symbol.level1 == 1)
        {
            bit = 0U;
            return true;
        }

        return false;
    };

    for (size_t i = 0; i < edata->num_symbols; i++)
    {
        const rmt_symbol_word_t &s = edata->received_symbols[i];

        uint8_t bit = 0U;
        
        if (!decodeManchesterSymbol(s, bit))
        {
            continue;
        }
        
        uart_write_bytes(UART_NUM_0, (const char[]){'0' + bit}, 1);

        currentByte = (currentByte << 1) | bit;
        // bitBuffer.push(bit);
        bitCount++;

        if (bitCount == 8)
        {
            xQueueSendFromISR(rxByteQueue, &currentByte, &hpTaskWoken);
            currentByte = 0;
            bitCount = 0;
        }
    }

    rmt_receive(channel, rmtBuffer, sizeof(rmtBuffer), &rxConfig);

    return true;
}

static void manchesterSendBit(bool bitValue)
{
    if (bitValue)
    {
        digitalWrite(configuredTxPin, HIGH);
        delayMicroseconds(configuredHalfBitUs);
        digitalWrite(configuredTxPin, LOW);
        delayMicroseconds(configuredHalfBitUs);
        return;
    }

    digitalWrite(configuredTxPin, LOW);
    delayMicroseconds(configuredHalfBitUs);
    digitalWrite(configuredTxPin, HIGH);
    delayMicroseconds(configuredHalfBitUs);
}

static inline rmt_symbol_word_t manchesterEncodeBit(bool bit)
{
    rmt_symbol_word_t sym = {};

    sym.duration0 = configuredHalfBitUs;
    sym.duration1 = configuredHalfBitUs;

    if (bit)
    {
        sym.level0 = 1;
        sym.level1 = 0;
    }
    else
    {
        sym.level0 = 0;
        sym.level1 = 1;
    }

    return sym;
}

static bool manchesterSendByteRMT(uint8_t value)
{
    static rmt_symbol_word_t symbols[8];

    for (int i = 0; i < 8; i++)
    {
        bool bit = (value >> (7 - i)) & 0x01;
        symbols[i] = manchesterEncodeBit(bit);
    }

    rmt_transmit_config_t tx_config = {
        .loop_count = 0
    };

    esp_err_t err = rmt_transmit(
        txChannel,
        copyEncoder,
        symbols,
        sizeof(symbols),
        &tx_config
    );

    if (err != ESP_OK)
    {
        printf("[RMT] transmit failed: %d\n", err);
        return false;
    }

    // 🔥 IMPORTANT: wait until transmission is finished
    rmt_tx_wait_all_done(txChannel, portMAX_DELAY);

    return true;
}

bool manchesterSendBytes(const uint8_t *data, size_t length)
{
    if (!data || length == 0)
        return false;

    if (!driverStarted)
    {
        if (!manchesterBegin(configuredRxPin, configuredTxPin, MANCHESTER_BIT_RATE))
            return false;
    }

    for (size_t i = 0; i < length; i++)
    {
        if (!manchesterSendByteRMT(data[i]))
            return false;
    }

    return true;
}

bool manchesterBegin(uint8_t rxPin, uint8_t txPin, uint32_t bitRate)
{
    configuredRxPin = rxPin;
    configuredTxPin = txPin;

    if (bitRate == 0U)
        bitRate = MANCHESTER_BIT_RATE;

    configuredHalfBitUs = (1000000UL / bitRate) / 2UL;

    pinMode(configuredRxPin, INPUT_PULLUP);
    pinMode(configuredTxPin, OUTPUT);
    digitalWrite(configuredTxPin, HIGH);

    if (rxByteQueue == nullptr)
    {
        rxByteQueue = xQueueCreate(MANCHESTER_RX_BYTE_QUEUE_LENGTH, sizeof(uint8_t));
        if (!rxByteQueue)
            return false;
    }

    // =========================
    // RMT RX CONFIGURATION
    // =========================

    rmt_rx_channel_config_t rx_chan_config = {
        .gpio_num = (gpio_num_t)configuredRxPin,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, // 1 tick = 1 µs
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false
        }
    };

    if (rmt_new_rx_channel(&rx_chan_config, &rxChannel) != ESP_OK)
    {
        printf("[RMT] Failed to create RX channel\n");
        return false;
    }

    // =========================
    // RMT TX CONFIGURATION
    // =========================

    rmt_tx_channel_config_t tx_chan_config = {
        .gpio_num = (gpio_num_t)configuredTxPin,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, // 1 tick = 1 µs
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        .flags = {
            .with_dma = false
        }
    };

    if (rmt_new_tx_channel(&tx_chan_config, &txChannel) != ESP_OK)
    {
        printf("[RMT] Failed to create TX channel\n");
        return false;
    }
    esp_err_t err = rmt_enable(txChannel);
    vTaskDelay(pdMS_TO_TICKS(10));
    if (err != ESP_OK)
    {
        printf("[RMT] Failed to enable TX channel\n");
        return false;
    }

    // encoder simple (copie brute de symbols)
    rmt_copy_encoder_config_t copy_cfg = {};
    err = rmt_new_copy_encoder(&copy_cfg, &copyEncoder);
    if (err != ESP_OK)
    {
        printf("[RMT] copy encoder creation failed: %d\n", err);
        return false;
    }
    if (copyEncoder == nullptr)
    {
        printf("[RMT] copyEncoder is NULL\n");
        return false;
    }

    // callback
    rmt_rx_event_callbacks_t cbs = {};
    cbs.on_recv_done = rmt_rx_done_cb;

    if (rmt_rx_register_event_callbacks(rxChannel, &cbs, nullptr) != ESP_OK)
    {
        printf("[RMT] Failed to register callbacks\n");
        return false;
    }

    // start RX config
    rxConfig = {
        .signal_range_min_ns = 1000,
        .signal_range_max_ns = 10000000
    };

    if (rmt_enable(rxChannel) != ESP_OK)
    {
        printf("[RMT] Failed to enable RX channel\n");
        return false;
    }

    if (rmt_receive(rxChannel, rmtBuffer, sizeof(rmtBuffer), &rxConfig) != ESP_OK)
    {
        printf("[RMT] Failed to start RX\n");
        return false;
    }

    driverStarted = true;

    printf("RMT Manchester RX started on pin %u\n", configuredRxPin);

    return true;
}


bool manchesterReceiveByte(uint8_t &value, TickType_t timeout)
{
    if (rxByteQueue == nullptr)
    {
        return false;
    }

    return xQueueReceive(rxByteQueue, &value, timeout) == pdTRUE;
}

void manchesterFlushRx()
{
    if (rxByteQueue == nullptr)
    {
        return;
    }

    uint8_t discard = 0U;
    while (xQueueReceive(rxByteQueue, &discard, 0) == pdTRUE)
    {
    }
}