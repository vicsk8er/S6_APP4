#include "error_injector.h"
#include <string.h>
#include <math.h>

void injectError(Frame &frame)// probléme avec les random ici
{
    frame.CRC ^= 0xFFFF; // Inverser les bits du CRC pour simuler une erreur
}