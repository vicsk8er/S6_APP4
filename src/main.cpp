#include <Arduino.h>
#include <stdint.h>
#include "rx_tasks.h"
#include "tx_tasks.h"
#include "utils/frame_buffer.h"

#define BUTTON_PIN 0

TaskHandle_t txTaskHandle = nullptr;

void IRAM_ATTR buttonISR()
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(
        txTaskHandle,
        &higherPriorityTaskWoken);

    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

void setup()
{
    Serial.begin(115200);
    initFrameBuffer();

    Serial1.begin(115200, SERIAL_8N1, 4, 5); // Rx = 4, Tx = 5

    pinMode(BUTTON_PIN, INPUT_PULLDOWN);

    xTaskCreate(
        uartRxTask,
        "UART_RX",
        4096,
        nullptr,
        2,
        nullptr);

    xTaskCreate(
        uartTxTask,
        "UART_TX",
        4096,
        nullptr,
        2,
        &txTaskHandle);

    attachInterrupt(
        BUTTON_PIN,
        buttonISR,
        RISING);
}

void loop()
{
    vTaskDelay(portMAX_DELAY);
}