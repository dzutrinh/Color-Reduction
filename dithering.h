#ifndef __DITHERING_H__
#define __DITHERING_H__

#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bayer matrix for ordered dithering (4x4) */
extern uint8 bayerMatrix[16];

/* Fast color clamping to 0-255 range */
static inline uint8 clamp(int n) {
    n &= -(n >= 0);
    return n | ((255 - n) >> 31);
}

#ifdef __cplusplus
}
#endif

#endif /* __DITHERING_H__ */
