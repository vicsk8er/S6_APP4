#include <Arduino.h>
#include <stdint.h>
#include "rx_tasks.h"
#include "tx_tasks.h"
#include "protocol.h"
#include "utils/frame_buffer.h"
// #include "manchester/manchester_driver.h"
#include "manchester/manchester_test.h"
#include "manchester/manchester_config.h"

#define BUTTON_PIN 0
#define DEBUG_AUTO_SEND 0   // 0 = mode normal (UART), 1 = envoi automatique toutes les 2 s

// TaskHandle_t txTaskHandle = nullptr;
volatile uint8_t buttonTrigger = 0;  // 0 = menu, 1 = button

void IRAM_ATTR buttonISR()
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    buttonTrigger = 1;  // Indiquer que c'est le bouton

    vTaskNotifyGiveFromISR(
        txTaskHandle,
        &higherPriorityTaskWoken);

    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
    }
    delay(1000); // Wait for Serial to initialize
    Serial.println("\n=== ESP32 Loopback Test ===\n");
    
    initFrameBuffer();

    testManchesterBegin(MANCHESTER_RX_PIN, MANCHESTER_TX_PIN, MANCHESTER_BIT_RATE);

    //pinMode(BUTTON_PIN, INPUT_PULLDOWN);

    xTaskCreate(
        uartRxTask,
        "MANCHESTER_RX",
        4096,
        nullptr,
        2,
        nullptr);

    xTaskCreate(
        uartTxTask,
        "MANCHESTER_TX",
        4096,
        nullptr,
        2,
        &txTaskHandle);

    // attachInterrupt(
    //     BUTTON_PIN,
    //     buttonISR,
    //     RISING);

    // Print menu
    Serial.println("\n\n========== NACK DEMONSTRATION TEST ==========");
    Serial.println("Commands:");
    Serial.println("  's' - Send NORMAL test message (no error)");
    Serial.println("  'e' - Send with ERROR INJECTION (triggers NACK)");
    Serial.println("  'd' - Display received DATA frames with timestamps");
    Serial.println("  'c' - Clear received frames queue");
    Serial.println("  'p' - Performance test (measure max throughput)");
    Serial.println("\nExpected NACK demo flow (with 'e'):");
    Serial.println("  1) TX sends START frame with total packets");
    Serial.println("  2) TX sends first DATA frame WITH ERROR (bad CRC)");
    Serial.println("  3) RX detects error -> sends NACK with missing frame number");
    Serial.println("  4) TX receives NACK -> retransmits the missing DATA frame");
    Serial.println("  5) RX accepts retransmitted DATA -> continues reception");
    Serial.println("  6) TX sends remaining DATA frames + END frame");
    Serial.println("==========================================\n");
}

void loop()
{
#if DEBUG_AUTO_SEND

    static uint32_t lastSend = 0;

    if (millis() - lastSend >= 2000)
    {
        lastSend = millis();

        Serial.println("\n>>> Auto sending test message...\n");
        const uint8_t testData[] = "Valid Data";
        protocolSendMessage(testData, sizeof(testData) - 1, false);

        buttonTrigger = 0;  // Indiquer que c'est le menu
        xTaskNotifyGive(txTaskHandle);  // Trigger TX task
    }

#else

    if (Serial.available())
    {
        char cmd = Serial.read();

        switch (cmd)
        {
            case 's':
            case 'S':
            {
                Serial.println("\n>>> Sending test message...\n");
                const uint8_t testData[] = "Valid Data";
                protocolSendMessage(testData, sizeof(testData) - 1, false);
                buttonTrigger = 0;
                xTaskNotifyGive(txTaskHandle);
                break;
            }

            case 'e':
            case 'E':
            {
                Serial.println("\n>>> Sending test message WITH error injection...\n");
                const uint8_t testData[] = "testing error detection and NACK";
                protocolSendMessage(testData, sizeof(testData) - 1, true);
                buttonTrigger = 0;
                xTaskNotifyGive(txTaskHandle);
                break;
            }

            case 'd':
            case 'D':
            {
                displayReceivedFrames();
                break;
            }

            case 'c':
            case 'C':
            {
                clearRxQueue();
                break;
            }

            case 'p':
            case 'P':
            {
                Serial.println("\n>>> Starting performance test...");
                performanceTest();
                break;
            }

            default:
                break;
        }
    }

#endif

    vTaskDelay(pdMS_TO_TICKS(100));
}