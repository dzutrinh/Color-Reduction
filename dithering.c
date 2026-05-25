#include "dithering.h"

/* Bayer matrix for ordered dithering (4x4) */
uint8 bayerMatrix[16] = { 
    0,  8,  2, 10, 
   12,  4, 14,  6,
    3, 11,  1,  9,
   15,  7, 13,  5
};
