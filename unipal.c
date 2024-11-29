/* UNIPAL.C: reduce an RGB 24-bit image to 8-bit using uniform palette */
/* Coded by Trinh D.D. Nguyen, Dec 2024 */

#include <stdio.h>
#include <string.h>
#include "image.h"
#include "bitmap.h"

typedef struct cube_t {
    uint32  r, g, b;
    uint32  count;
} cube, cubes[256];

/* for ordered dithering */
uint8 bayerMatrix[16] = { 0,  8,  2, 10, 
                         12,  4, 14,  6,
                          3, 11,  1,  9,
                         15,  7, 13,  5};

/* slightly fast color clamping */
inline uint8 clamp(int n) {
    n &= -(n >= 0);
    return n | ((255 - n) >> 31);
}

/* fast RGB quantization */
bitmap quantize_uniform(const bitmap bmp, bool dither) {
    if (!bmp) return NULL;
    if (bmp->format != BMF_RGB24) return NULL;

    /* create output indexed bitmap */
    bitmap res = bitmap_create(bmp->width, bmp->height, BMF_INDEXED8, true);
        
    cubes uniCubes = {0};       /* RGB cubes */ 
    uint8 * src = bmp->data;    /* pointer to source bitmap bits */
    uint8 * dst = res->data;    /* pointer to output bitmap bits */    
    
    /* quantization phase */
    for (int y = 0; y < bmp->height; y++) {
        for (int x = 0; x < bmp->width; x++) {
            /* dithering if needed */
            int t = dither ? bayerMatrix[((y & 3) << 2) + (x & 3)] : 0;
            int b = clamp((*src++) + (t << 1));
            int g = clamp((*src++) + (t << 1));
            int r = clamp((*src++) + (t << 1));

            /* quantize */
            uint8 k = ((r >> 5) << 5) + ((g >> 5) << 2) + (b >> 6);
		    (*dst++) = k;

            /* preparing RGB cubes for CLUT */
            uniCubes[k].r += r;
            uniCubes[k].g += g;
            uniCubes[k].b += b;
            uniCubes[k].count++;		
        }
    }

    /* generate a uniform CLUT based on the quantized colors */
    for (int i = 0; i < 256; i++)
        if (uniCubes[i].count) {
            res->pal[i].r = (uniCubes[i].r / uniCubes[i].count);
            res->pal[i].g = (uniCubes[i].g / uniCubes[i].count);
            res->pal[i].b = (uniCubes[i].b / uniCubes[i].count);
        }
        else {
            res->pal[i].r = 0;
            res->pal[i].g = 0;
            res->pal[i].b = 0;
        }

    return res;
}

/* main program */
int main(int argc, char * argv[]) {
    bitmap  bmp;
    bool    dither = false;
    char    input[256] = {0}, output[256] = "output.bmp";

    if (argc < 2) {
        printf("Usage: unipal image.bmp [output.bmp] [-d[ither]]\n");
        return -1;
    }

    if (argc >= 2) {
        strncpy(input, argv[1], 255);
        if (argc == 3) {
            dither = !strcmp(argv[2], "-dither") || !strcmp(argv[2], "-d") ;
            if (!dither)
                strncpy(output, argv[2], 255);
        }
        else 
        if (argc > 3) {
            strncpy(output, argv[2], 255);
            dither = !strcmp(argv[3], "-dither") || !strcmp(argv[3], "-d") ;
        }
    }

    printf(". Input  = [%s]\n", input);
    printf(". Output = [%s]\n", output);

    printf(". Loading bitmap [%s]...\n", input);
    if (!(bmp = bmp_load(input))) {
        printf("ERROR: cannot load [%s]\n", input);
        return -1;
    }
    printf("  - Image dimensions = %d x %d\n", bmp->width, bmp->height);

    printf(". Quantizing colors (dithering: %s)...\n", dither ? "yes" : "no");
    bitmap res = quantize_uniform(bmp, dither);

    if (res) {
        printf(". Saving output to [%s]...\n", output);
        if (!bmp_save(output, &res))
            printf("ERROR: cannot write output bitmap.\n");
        bitmap_destroy(&res);
    }
    else
        printf("ERROR: input bitmap must be 24-bit.\n");

    bitmap_destroy(&bmp);
    return 0;
}
