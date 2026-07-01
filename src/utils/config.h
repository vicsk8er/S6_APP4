#ifndef CONFIG_H_
#define CONFIG_H_
#include <stdint.h>

//Frame field size
const uint8_t PREAMBLE_BYTE_SIZE = 1U;  // Préambule
const uint8_t START_BYTE_SIZE = 1U;     // Start
const uint8_t HEADING_BYTE_SIZE = 4U;   // Entête
const uint8_t MAX_PAYLOAD_BYTE_SIZE = 80U; // Charge utile
const uint8_t CRC_BYTE_SIZE = 2U;       // Contrôle
const uint8_t END_BYTE_SIZE = 1U;       // End
//
const uint8_t MAX_FRAME_SIZE = PREAMBLE_BYTE_SIZE + START_BYTE_SIZE + HEADING_BYTE_SIZE + MAX_PAYLOAD_BYTE_SIZE + CRC_BYTE_SIZE + END_BYTE_SIZE;

const uint8_t preamble_value = 0b01010101;
const uint8_t start_value = 0b01111110;
const uint8_t end_value = 0b01111110;


enum CommunicationType : uint8_t
{
    Start = 0x01,
    Data = 0x02,
    End = 0x03,
    Nack = 0x04,
};

struct Heading
{
    uint8_t type; // type de trame
    uint8_t sequenceNumber; // Nombre de trame à envoyer
    uint8_t payloadLength; // longueur des données 
    uint8_t parameter; // paramètre spéciaux (seulement utile pour type "START" et "NACK")
};

struct Frame
{
    uint8_t preamble;
    uint8_t start;
    Heading heading;
    uint8_t payload[MAX_PAYLOAD_BYTE_SIZE];
    uint16_t CRC;
    uint8_t end; 
};

const Frame DEFAULT_FRAME = {
    .preamble = preamble_value,
    .start = start_value,
    .heading = 
    {
        .type = (uint8_t)(CommunicationType::Start),
        .sequenceNumber = 0U,
        .payloadLength = 0U,
        .parameter = 0U,
    },
    .payload = { 0U },
    .CRC = 0U,
    .end = end_value,
};
#endif