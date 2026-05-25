/*
 * colormatch - Fast color matching using k-d tree
 * 
 * This tool maps pixels in a 24-bit RGB image to the nearest colors
 * in a provided palette using k-d tree for fast lookups.
 * 
 * Usage: colormatch <input.bmp> <palette.bmp> <output.bmp> [-d[1-9]]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"
#include "bitmap.h"
#include "dithering.h"
#include "kdtree.h"

/* Extract palette from an indexed bitmap */
bool extract_palette(bitmap bmp, rgb_t *palette, uint32 *num_colors) {
    if (!bmp || !bmp->pal) return false;
    
    /* Determine palette size based on format */
    uint32 pal_size = 0;
    switch (bmp->format) {
        case BMF_INDEXED2: pal_size = 4; break;
        case BMF_INDEXED4: pal_size = 16; break;
        case BMF_INDEXED8: pal_size = 256; break;
        default: return false;
    }
    
    /* Count actual used colors and swap R/B since BMP palettes are in BGR format */
    *num_colors = 0;
    for (uint32 i = 0; i < pal_size; i++) {
        if (bmp->pal[i].r != 0 || bmp->pal[i].g != 0 || bmp->pal[i].b != 0) {
            /* Swap B and R to convert from BMP's BGR to RGB */
            palette[*num_colors].r = bmp->pal[i].b;
            palette[*num_colors].g = bmp->pal[i].g;
            palette[*num_colors].b = bmp->pal[i].r;
            (*num_colors)++;
        }
    }
    
    /* If palette appears empty, copy all entries anyway */
    if (*num_colors == 0) {
        for (uint32 i = 0; i < pal_size; i++) {
            /* Swap B and R to convert from BMP's BGR to RGB */
            palette[i].r = bmp->pal[i].b;
            palette[i].g = bmp->pal[i].g;
            palette[i].b = bmp->pal[i].r;
        }
        *num_colors = pal_size;
    }
    
    return true;
}

/* Map image to palette using k-d tree */
bitmap map_to_palette(bitmap src, rgb_t *palette, uint32 num_colors, int ditherStrength) {
    if (!src || !palette || num_colors == 0) return NULL;
    
    /* Only support 24-bit RGB images */
    if (src->format != BMF_RGB24) {
        printf("Error: Only 24-bit RGB images are supported\n");
        return NULL;
    }
    
    printf("Building k-d tree from %u colors...\n", num_colors);
    kdtree_t *kdtree = kdtree_create(palette, num_colors);
    if (!kdtree) {
        printf("Error: Failed to create k-d tree\n");
        return NULL;
    }
    
    /* Create output indexed bitmap - always use 8-bit format */
    bitmap dst = bitmap_create(src->width, src->height, BMF_INDEXED8, true);
    if (!dst) {
        printf("Error: Failed to create output bitmap\n");
        kdtree_destroy(kdtree);
        return NULL;
    }
    
    /* Copy palette */
    memcpy(dst->pal, palette, num_colors * sizeof(rgb_t));
    
    /* Map pixels */
    printf("Mapping pixels to palette%s...\n", ditherStrength ? " (dithered)" : "");
    uint8 *data = src->data;
    uint32 total_pixels = src->width * src->height;
    
    if (ditherStrength) {
        /* Dithered mapping */
        uint32 idx = 0;
        for (uint32 y = 0; y < src->height; y++) {
            int yIndex = (y & 3) << 2;
            for (uint32 x = 0; x < src->width; x++) {
                int t = (bayerMatrix[yIndex + (x & 3)] - 8) * ditherStrength;
                uint8 r = clamp(data[idx * 3 + 0] + t);
                uint8 g = clamp(data[idx * 3 + 1] + t);
                uint8 b = clamp(data[idx * 3 + 2] + t);
                
                dst->data[idx] = kdtree_nearest(kdtree, r, g, b);
                idx++;
            }
            
            if ((y & 31) == 0) {
                printf("  Progress: %u%%\r", (y * 100) / src->height);
                fflush(stdout);
            }
        }
    } else {
        /* Non-dithered mapping */
        for (uint32 i = 0; i < total_pixels; i++) {
            uint8 r = data[i * 3 + 0];
            uint8 g = data[i * 3 + 1];
            uint8 b = data[i * 3 + 2];
            
            dst->data[i] = kdtree_nearest(kdtree, r, g, b);
            
            if (i % (total_pixels / 20) == 0) {
                printf("  Progress: %u%%\r", (i * 100) / total_pixels);
                fflush(stdout);
            }
        }
    }
    
    printf("  Progress: 100%%\n");
    
    kdtree_destroy(kdtree);
    return dst;
}

int main(int argc, char *argv[]) {
    char *input_file, *palette_file, *output_file;
    int ditherStrength = 0;
    
    printf("Color Matcher - k-d tree based palette mapping\n");
    printf("===============================================\n\n");
    
    if (argc < 4) {
        printf("Usage: %s <input.bmp> <palette.bmp> <output.bmp> [-d[1-9]]\n", argv[0]);
        printf("  input.bmp   - Input 24-bit RGB bitmap\n");
        printf("  palette.bmp - Indexed bitmap containing the palette\n");
        printf("  output.bmp  - Output indexed bitmap\n");
        printf("  -d[1-9]     - Optional dithering strength (1-9)\n");
        return 1;
    }
    
    input_file = argv[1];
    palette_file = argv[2];
    output_file = argv[3];
    
    /* Check for dither flag */
    if (argc >= 5) {
        if (argv[4][0] == '-' && (argv[4][1] == 'd' || argv[4][1] == 'D')) {
            if (argv[4][2] >= '1' && argv[4][2] <= '9')
                ditherStrength = argv[4][2] - '0';
            else
                ditherStrength = 2;
        }
    }
    
    printf("Input:   %s\n", input_file);
    printf("Palette: %s\n", palette_file);
    printf("Output:  %s\n\n", output_file);
    
    /* Load input image */
    printf("Loading input image...\n");
    bitmap src = bmp_load(input_file);
    if (!src) {
        printf("Error: Failed to load input image\n");
        return 1;
    }
    printf("  Size: %ux%u, %u-bit\n", src->width, src->height, src->format);
    
    /* Load palette image */
    printf("Loading palette image...\n");
    bitmap pal_bmp = bmp_load(palette_file);
    if (!pal_bmp) {
        printf("Error: Failed to load palette image\n");
        bitmap_destroy(&src);
        return 1;
    }
    
    /* Extract palette */
    rgb_t palette[256];
    uint32 num_colors = 0;
    if (!extract_palette(pal_bmp, palette, &num_colors)) {
        printf("Error: Failed to extract palette (image must be indexed)\n");
        bitmap_destroy(&src);
        bitmap_destroy(&pal_bmp);
        return 1;
    }
    printf("  Palette contains %u colors\n\n", num_colors);
    bitmap_destroy(&pal_bmp);
    
    /* Map to palette */
    bitmap dst = map_to_palette(src, palette, num_colors, ditherStrength);
    if (!dst) {
        printf("Error: Failed to map to palette\n");
        bitmap_destroy(&src);
        return 1;
    }
    
    /* Save result */
    printf("Saving output...\n");
    if (!bmp_save(output_file, &dst)) {
        printf("Error: Failed to save output\n");
        bitmap_destroy(&src);
        bitmap_destroy(&dst);
        return 1;
    }
    
    printf("Done!\n");
    
    bitmap_destroy(&src);
    bitmap_destroy(&dst);
    return 0;
}
