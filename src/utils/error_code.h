#ifndef ERROR_CODE_H_
#define ERROR_CODE_H_
#include <stdint.h>

// Selon moi, les erreus devrait seulement être vérifier par le récepteur
// Attention : l'émetteur initial peut devenir le récepteur pour une bref période 
// Par exemple : lorsqu'il reçoit un NACK de la part du récepteur original
enum ErrorCode : uint8_t
{
    COMM_OK = 0,         // Aucune erreur
    ERR_CRC,             // CRC invalide : trame corrompue
    ERR_SEQUENCE,        // Numéro de séquence inattendu
    ERR_LENGTH,          // Longueur de charge utile invalide (> 80 octets)
    ERR_TYPE,            // Type de trame inconnu ou invalide
    ERR_TIMEOUT,         // Délai d'attente dépassé
    ERR_BUFFER_FULL,     // Tampon de réception ou d'émission plein
    ERR_NOT_INITIALIZED, // Trame de début non reçue
    ERR_UNEXPECTED_END,  // Trame de fin reçu trop tôt
    ERR_VALUE_FIELD      // Un field comme le préambule, start ou end n'ont pas la bonne valeur 
};

#endif 