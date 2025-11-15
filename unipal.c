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
static inline uint8 clamp(int n) {
    n &= -(n >= 0);
    return n | ((255 - n) >> 31);
}

/* fast RGB quantization */
bitmap quantize_uniform(const bitmap bmp, int ditherStrength) {
    if (!bmp) return NULL;
    if (bmp->format != BMF_RGB24) return NULL;

    /* create output indexed bitmap */
    bitmap res = bitmap_create(bmp->width, bmp->height, BMF_INDEXED8, true);
        
    cubes uniCubes = {0};       /* RGB cubes */ 
    uint8 * src = bmp->data;    /* pointer to source bitmap bits */
    uint8 * dst = res->data;    /* pointer to output bitmap bits */    
    
    /* quantization phase */
    if (ditherStrength) {
        /* Dithered quantization */
        for (int y = 0; y < bmp->height; y++) {
            int yIndex = (y & 3) << 2;  /* precompute row offset in Bayer matrix */
            for (int x = 0; x < bmp->width; x++) {
                int t = (bayerMatrix[yIndex + (x & 3)] - 8) * ditherStrength;
                int b = clamp((*src++) + t);
                int g = clamp((*src++) + t);
                int r = clamp((*src++) + t);
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
    } else {
        /* Non-dithered quantization (faster path) */
        uint32 totalPixels = bmp->width * bmp->height;
        for (uint32 i = 0; i < totalPixels; i++) {
            int b = *src++;
            int g = *src++;
            int r = *src++;
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

    return res;
}

/* main program */
int main(int argc, char * argv[]) {
    bitmap  bmp;
    int     ditherStrength = 0;  /* 0=off, 1-9=strength levels */
    char    input[256] = {0}, output[256] = "output.bmp";

    if (argc < 2) {
        printf("Usage: unipal image.bmp [output.bmp] [-d[1-9]]\n");
        printf("  -d or -dither: enable dithering (default strength=2)\n");
        printf("  -d1 to -d9: enable dithering with strength 1-9\n");
        return -1;
    }

    if (argc >= 2) {
        strncpy(input, argv[1], 255);
        if (argc == 3) {
            /* Check if it's a dither flag */
            if (argv[2][0] == '-' && (argv[2][1] == 'd' || argv[2][1] == 'D')) {
                if (argv[2][2] >= '1' && argv[2][2] <= '9')
                    ditherStrength = argv[2][2] - '0';
                else if (!strcmp(argv[2], "-dither") || !strcmp(argv[2], "-d"))
                    ditherStrength = 2;  /* default strength */
            }
            else
                strncpy(output, argv[2], 255);
        }
        else 
        if (argc > 3) {
            strncpy(output, argv[2], 255);
            /* Check if it's a dither flag */
            if (argv[3][0] == '-' && (argv[3][1] == 'd' || argv[3][1] == 'D')) {
                if (argv[3][2] >= '1' && argv[3][2] <= '9')
                    ditherStrength = argv[3][2] - '0';
                else if (!strcmp(argv[3], "-dither") || !strcmp(argv[3], "-d"))
                    ditherStrength = 2;  /* default strength */
            }
        }
    }

    printf(". Input  = [%s]\n", input);
    printf(". Output = [%s]\n", output);

    /* Validate input file extension */
    size_t len = strlen(input);
    if (len < 4 || (strcmp(input + len - 4, ".bmp") != 0 && strcmp(input + len - 4, ".BMP") != 0)) {
        printf("ERROR: input file must be a BMP file (.bmp extension)\n");
        return -1;
    }

    printf(". Loading bitmap [%s]...\n", input);
    if (!(bmp = bmp_load(input))) {
        printf("ERROR: cannot load [%s] or file is not a valid BMP\n", input);
        return -1;
    }
    printf("  - Image dimensions = %d x %d\n", bmp->width, bmp->height);

    if (ditherStrength)
        printf(". Quantizing colors (dithering: yes, strength=%d)...\n", ditherStrength);
    else
        printf(". Quantizing colors (dithering: no)...\n");
    bitmap res = quantize_uniform(bmp, ditherStrength);

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
