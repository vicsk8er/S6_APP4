#include "manchester_test.h"
#include "manchester_config.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// ======================================================
// CONFIG
// ======================================================

static uint8_t rxPin = 0;
static uint8_t txPin = 0;

static uint32_t halfBitUs = 0;

static bool driverStarted = false;

// ======================================================
// RX STATE
// ======================================================

static QueueHandle_t rxEdgeQueue = nullptr;
static QueueHandle_t rxByteQueue = nullptr;

// reconstruction bit -> byte
static uint8_t currentByte = 0;
static uint8_t bitCount = 0;

// timing ISR
static volatile uint32_t lastEdgeUs = 0;
static volatile uint8_t lastLevel = 0;

static uint32_t bitUs = (1000000UL / MANCHESTER_BIT_RATE);


typedef struct
{
    bool rising;        // true = LOW->HIGH, false = HIGH->LOW
    uint32_t deltaUs;   // temps depuis le front précédent
} ManchesterEdgeEvent;
// ======================================================
// TX CONFIG
// ======================================================
static bool firstByteOfFrame = false;

static inline void testWriteBit(uint8_t bit)
{
    if (bit == 0)
    {
        digitalWrite(txPin, HIGH);
        delayMicroseconds(halfBitUs);

        digitalWrite(txPin, LOW);
        delayMicroseconds(halfBitUs);
    }
    else
    {
        digitalWrite(txPin, LOW);
        delayMicroseconds(halfBitUs);

        digitalWrite(txPin, HIGH);
        delayMicroseconds(halfBitUs);
    }
}

static void testManchesterRxTask(void *)
{
    ManchesterEdgeEvent evt;

    bool waitingMidBit = false;

    while (true)
    {
        if (!xQueueReceive(rxEdgeQueue, &evt, portMAX_DELAY))
            continue;

        //--------------------------------------------------
        // transition espacée d'environ 1 bit
        //--------------------------------------------------

        if (evt.deltaUs > (halfBitUs * 1.5))
        {
            uint8_t bit = evt.rising ? 1 : 0;

            currentByte = (currentByte << 1) | bit;
            bitCount++;

            waitingMidBit = false;
        }

        //--------------------------------------------------
        // transition espacée d'un demi-bit
        //--------------------------------------------------

        else
        {
            if (!waitingMidBit)
            {
                // transition de frontière
                waitingMidBit = true;
                continue;
            }

            // deuxième transition -> milieu du bit
            uint8_t bit = evt.rising ? 1 : 0;

            currentByte = (currentByte << 1) | bit;
            bitCount++;

            waitingMidBit = false;
        }

        //--------------------------------------------------

        if (bitCount == 8)
        {
            if (firstByteOfFrame)
            {
                currentByte ^= 0x80; // hacking de magouille
                firstByteOfFrame = false;
            }
            xQueueSend(rxByteQueue, &currentByte, 0);

            currentByte = 0;
            bitCount = 0;
        }
    }
}

void IRAM_ATTR testManchesterIsr()
{
    uint32_t now = micros();
    uint32_t delta = now - lastEdgeUs;
    lastEdgeUs = now;

    if (delta < (halfBitUs / 2))
        return;

    // détection début de trame
    if (delta >= 4500) // 4.5ms
    {
        firstByteOfFrame = true;
    }

    ManchesterEdgeEvent evt;
    evt.deltaUs = delta;
    evt.rising = digitalRead(rxPin);

    BaseType_t hpTaskWoken = pdFALSE;
    xQueueSendFromISR(rxEdgeQueue, &evt, &hpTaskWoken);

    if (hpTaskWoken)
        portYIELD_FROM_ISR();
}

// ======================================================
// PUBLIC INIT
// ======================================================

bool testManchesterBegin(uint8_t rx, uint8_t tx, uint32_t bitrate)
{
    rxPin = rx;
    txPin = tx;

    if (bitrate == 0)
        bitrate = MANCHESTER_BIT_RATE;

    halfBitUs = (1000000UL / bitrate) / 2;

    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);

    digitalWrite(txPin, LOW);

    rxEdgeQueue = xQueueCreate(256, sizeof(ManchesterEdgeEvent));
    rxByteQueue = xQueueCreate(256, sizeof(uint8_t));
    if (!rxEdgeQueue || !rxByteQueue)
        return false;

    attachInterrupt(digitalPinToInterrupt(rxPin), testManchesterIsr, CHANGE);

    driverStarted = true;

    xTaskCreate(
    testManchesterRxTask,
    "MANCHESTER_BIT_TASK",
    4096,
    nullptr,
    5,
    nullptr
    );
    Serial.println("[TEST MANCHESTER] init OK");
    Serial.printf("halfBitUs = %lu us\n", halfBitUs);

    return true;
}

// ======================================================
// PUBLIC TX
// ======================================================

bool testManchesterSendByte(uint8_t byte)
{
    if (!driverStarted)
        return false;

    for (int i = 7; i >= 0; --i)
    {
        uint8_t bit = (byte >> i) & 0x01;
        testWriteBit(bit);
    }

    return true;
}

bool testManchesterSendBytes(const uint8_t *data, size_t len)
{
    if (!driverStarted || !data)
        return false;

    for (size_t i = 0; i < len; i++)
    {
        if (!testManchesterSendByte(data[i]))
            return false;
    }

    return true;
}

// ======================================================
// PUBLIC RX
// ======================================================

bool testManchesterReceiveByte(uint8_t &byte, TickType_t timeout)
{
    if (!driverStarted)
        return false;

    return xQueueReceive(rxByteQueue, &byte, timeout) == pdTRUE;
}
