#include <Arduino.h>
#include <stdint.h>
#include "rx_tasks.h"
#include "tx_tasks.h"
#include "protocol.h"
#include "utils/frame_buffer.h"

#define BUTTON_PIN 0

TaskHandle_t txTaskHandle = nullptr;
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
    delay(1000);  // Wait for serial monitor
    Serial.println("\n=== ESP32 Loopback Test ===\n");
    
    initFrameBuffer();

    Serial1.begin(115200, SERIAL_8N1, 4, 5); // Rx = 4, Tx = 5

    //pinMode(BUTTON_PIN, INPUT_PULLDOWN);

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
                buttonTrigger = 0;  // Indiquer que c'est le menu
                xTaskNotifyGive(txTaskHandle);  // Trigger TX task
                break;
            }
            
            case 'e':
            case 'E':
            {
                Serial.println("\n>>> Sending test message WITH error injection...\n");
                const uint8_t testData[] = "testing error detection and NACK";
                protocolSendMessage(testData, sizeof(testData) - 1, true);
                buttonTrigger = 0;  // Indiquer que c'est le menu
                xTaskNotifyGive(txTaskHandle);  // Trigger TX task
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
    
    vTaskDelay(100);
}