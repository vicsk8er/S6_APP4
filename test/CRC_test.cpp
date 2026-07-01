#include <Arduino.h>
#include <stdint.h>
#include <cstring>
#include "CRC16.h"

CRC16 CRC; // Polynomial = 0x9001, initial value = 0x0, Final Xor Valeur = 0x0;
void setup() 
{
  	Serial.begin(115200);
	while(!Serial);
}

void loop() {
	const char *str = "Hello";
	CRC.add((const uint8_t*)str, (crc_size_t)strlen(str));
	uint16_t valeurCRC = CRC.calc();
	Serial.print("Value of str CRC = ");
	Serial.println(valeurCRC, HEX);
	CRC.restart();
	delay(1000);
}


// Pour le loopback, utiliser Serial1 ou Serial2 (pas Serial)
// Il faudrait que la task de réception d'écoute s'acctive seulement 