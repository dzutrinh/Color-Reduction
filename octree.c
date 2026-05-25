/*
 * Octree Color Quantization
 * 
 * This program reduces the number of colors in a 24-bit RGB image using
 * the octree color quantization algorithm. The algorithm builds an octree
 * structure to represent the color space and then prunes it to reduce
 * the palette to a specified number of colors.
 * 
 * Usage: octree <input.bmp> <output.bmp> [colors]
 *        colors: number of colors in output palette (default: 256, max: 256)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "image.h"
#include "bitmap.h"
#include "dithering.h"

#define MAX_DEPTH 8
#define MAX_COLORS 256

/* Octree node structure */
typedef struct octree_node {
    struct octree_node *children[8];   /* 8 children for each octant */
    struct octree_node *next;          /* for reducible list */
    uint8 is_leaf;                      /* 1 if this is a leaf node */
    uint32 pixel_count;                 /* number of pixels represented */
    uint32 red_sum;                     /* sum of red values */
    uint32 green_sum;                   /* sum of green values */
    uint32 blue_sum;                    /* sum of blue values */
    uint8 palette_index;                /* index in the color palette */
} octree_node_t;

/* Octree structure */
typedef struct {
    octree_node_t *root;
    uint32 leaf_count;
    uint32 max_colors;
    octree_node_t *reducible_nodes[MAX_DEPTH];  /* list of reducible nodes at each level */
} octree_t;

/* Function prototypes */
octree_t *octree_create(uint32 max_colors);
void octree_destroy(octree_t *tree);
octree_node_t *octree_node_create(void);
void octree_node_destroy(octree_node_t *node);
void octree_add_color(octree_t *tree, uint8 r, uint8 g, uint8 b);
void octree_insert(octree_t *tree, octree_node_t **node, uint8 r, uint8 g, uint8 b, uint32 depth);
void octree_reduce(octree_t *tree);
void octree_build_palette(octree_t *tree, rgb_t *palette, uint32 *num_colors);
void octree_get_palette_index(octree_node_t *node, uint8 r, uint8 g, uint8 b, uint32 depth, uint8 *index);
bitmap quantize_image(bitmap src, uint32 max_colors, int ditherStrength);

/* Create a new octree */
octree_t *octree_create(uint32 max_colors) {
    octree_t *tree = (octree_t *)malloc(sizeof(octree_t));
    if (!tree) return NULL;
    
    tree->root = NULL;
    tree->leaf_count = 0;
    tree->max_colors = (max_colors > MAX_COLORS) ? MAX_COLORS : max_colors;
    
    for (int i = 0; i < MAX_DEPTH; i++) {
        tree->reducible_nodes[i] = NULL;
    }
    
    return tree;
}

/* Create a new octree node */
octree_node_t *octree_node_create(void) {
    octree_node_t *node = (octree_node_t *)malloc(sizeof(octree_node_t));
    if (!node) return NULL;
    
    for (int i = 0; i < 8; i++) {
        node->children[i] = NULL;
    }
    
    node->next = NULL;
    node->is_leaf = 0;
    node->pixel_count = 0;
    node->red_sum = 0;
    node->green_sum = 0;
    node->blue_sum = 0;
    node->palette_index = 0;
    
    return node;
}

/* Destroy an octree node recursively */
void octree_node_destroy(octree_node_t *node) {
    if (!node) return;
    
    for (int i = 0; i < 8; i++) {
        if (node->children[i]) {
            octree_node_destroy(node->children[i]);
        }
    }
    
    free(node);
}

/* Destroy the octree */
void octree_destroy(octree_t *tree) {
    if (!tree) return;
    
    if (tree->root) {
        octree_node_destroy(tree->root);
    }
    
    free(tree);
}

/* Add a color to the octree */
void octree_add_color(octree_t *tree, uint8 r, uint8 g, uint8 b) {
    octree_insert(tree, &tree->root, r, g, b, 0);
}

/* Insert a color into the octree */
void octree_insert(octree_t *tree, octree_node_t **node, uint8 r, uint8 g, uint8 b, uint32 depth) {
    /* Create node if it doesn't exist */
    if (*node == NULL) {
        *node = octree_node_create();
        if (!*node) return;
        
        /* If we're at the maximum depth, make this a leaf */
        if (depth >= MAX_DEPTH) {
            (*node)->is_leaf = 1;
            tree->leaf_count++;
        } else {
            /* Add to reducible list */
            (*node)->next = tree->reducible_nodes[depth];
            tree->reducible_nodes[depth] = *node;
        }
    }
    
    /* If this is a leaf node, accumulate color */
    if ((*node)->is_leaf) {
        (*node)->pixel_count++;
        (*node)->red_sum += r;
        (*node)->green_sum += g;
        (*node)->blue_sum += b;
    } else {
        /* Determine which child to traverse */
        uint32 shift = 7 - depth;
        uint8 index = ((r >> shift) & 1) << 2 |
                      ((g >> shift) & 1) << 1 |
                      ((b >> shift) & 1);
        
        octree_insert(tree, &(*node)->children[index], r, g, b, depth + 1);
    }
}

/* Reduce the octree by merging the deepest nodes */
void octree_reduce(octree_t *tree) {
    /* Find the deepest level with reducible nodes */
    int level;
    for (level = MAX_DEPTH - 1; level >= 0; level--) {
        if (tree->reducible_nodes[level] != NULL) {
            break;
        }
    }
    
    if (level < 0) return;  /* Nothing to reduce */
    
    /* Get the first node from this level */
    octree_node_t *node = tree->reducible_nodes[level];
    tree->reducible_nodes[level] = node->next;
    
    /* Merge all children into this node */
    uint32 red_sum = 0, green_sum = 0, blue_sum = 0, pixel_count = 0;
    
    for (int i = 0; i < 8; i++) {
        if (node->children[i]) {
            red_sum += node->children[i]->red_sum;
            green_sum += node->children[i]->green_sum;
            blue_sum += node->children[i]->blue_sum;
            pixel_count += node->children[i]->pixel_count;
            
            if (node->children[i]->is_leaf) {
                tree->leaf_count--;
            }
            
            octree_node_destroy(node->children[i]);
            node->children[i] = NULL;
        }
    }
    
    /* Make this node a leaf */
    node->is_leaf = 1;
    node->red_sum = red_sum;
    node->green_sum = green_sum;
    node->blue_sum = blue_sum;
    node->pixel_count = pixel_count;
    tree->leaf_count++;
}

/* Build the color palette from the octree */
void octree_build_palette_recursive(octree_node_t *node, rgb_t *palette, uint32 *index) {
    if (!node) return;
    
    if (node->is_leaf) {
        /* Calculate average color for this node */
        if (node->pixel_count > 0) {
            palette[*index].r = node->red_sum / node->pixel_count;
            palette[*index].g = node->green_sum / node->pixel_count;
            palette[*index].b = node->blue_sum / node->pixel_count;
            node->palette_index = *index;
            (*index)++;
        }
    } else {
        /* Recurse into children */
        for (int i = 0; i < 8; i++) {
            if (node->children[i]) {
                octree_build_palette_recursive(node->children[i], palette, index);
            }
        }
    }
}

void octree_build_palette(octree_t *tree, rgb_t *palette, uint32 *num_colors) {
    uint32 index = 0;
    octree_build_palette_recursive(tree->root, palette, &index);
    *num_colors = index;
}

/* Get the palette index for a given color */
void octree_get_palette_index(octree_node_t *node, uint8 r, uint8 g, uint8 b, uint32 depth, uint8 *index) {
    if (!node) {
        *index = 0;
        return;
    }
    
    if (node->is_leaf) {
        *index = node->palette_index;
        return;
    }
    
    /* Determine which child to traverse */
    uint32 shift = 7 - depth;
    uint8 child_index = ((r >> shift) & 1) << 2 |
                        ((g >> shift) & 1) << 1 |
                        ((b >> shift) & 1);
    
    if (node->children[child_index]) {
        octree_get_palette_index(node->children[child_index], r, g, b, depth + 1, index);
    } else {
        /* Find the first available child */
        for (int i = 0; i < 8; i++) {
            if (node->children[i]) {
                octree_get_palette_index(node->children[i], r, g, b, depth + 1, index);
                return;
            }
        }
        *index = 0;
    }
}

/* Quantize a 24-bit RGB image to an indexed color image */
bitmap quantize_image(bitmap src, uint32 max_colors, int ditherStrength) {
    if (!src) return NULL;
    
    /* Only support 24-bit RGB images */
    if (src->format != BMF_RGB24) {
        printf("Error: Only 24-bit RGB images are supported\n");
        return NULL;
    }
    
    if (ditherStrength)
        printf("Building octree from image colors (dithering: yes, strength=%d)...\n", ditherStrength);
    else
        printf("Building octree from image colors...\n");
    
    /* Create octree */
    octree_t *tree = octree_create(max_colors);
    if (!tree) {
        printf("Error: Failed to create octree\n");
        return NULL;
    }
    
    /* Add all colors to the octree */
    uint8 *data = src->data;
    uint32 total_pixels = src->width * src->height;
    uint32 progress_step = total_pixels / 20;  /* Update every 5% instead of 10% */
    
    if (ditherStrength) {
        /* Dithered quantization - build full tree first */
        uint32 idx = 0;
        for (uint32 y = 0; y < src->height; y++) {
            int yIndex = (y & 3) << 2;  /* precompute row offset in Bayer matrix */
            for (uint32 x = 0; x < src->width; x++) {
                int t = (bayerMatrix[yIndex + (x & 3)] - 8) * ditherStrength;
                uint8 r = clamp(data[idx * 3 + 0] + t);
                uint8 g = clamp(data[idx * 3 + 1] + t);
                uint8 b = clamp(data[idx * 3 + 2] + t);
                octree_add_color(tree, r, g, b);
                
                if (progress_step > 0 && idx % progress_step == 0) {
                    printf("  Building: %u%%\r", (idx * 100) / total_pixels);
                    fflush(stdout);
                }
                idx++;
            }
        }
    } else {
        /* Non-dithered quantization - build full tree first */
        for (uint32 i = 0; i < total_pixels; i++) {
            uint8 r = data[i * 3 + 0];
            uint8 g = data[i * 3 + 1];
            uint8 b = data[i * 3 + 2];
            octree_add_color(tree, r, g, b);
        
            if (progress_step > 0 && i % progress_step == 0) {
                printf("  Building: %u%%\r", (i * 100) / total_pixels);
                fflush(stdout);
            }
        }
    }
    
    printf("\n  Initial colors: %u\n", tree->leaf_count);
    
    /* Now reduce tree to target size if needed */
    if (tree->leaf_count > max_colors) {
        printf("  Reducing to %u colors...\n", max_colors);
        while (tree->leaf_count > max_colors) {
            octree_reduce(tree);
            
            /* Show progress for large reductions */
            if (tree->leaf_count % 1000 == 0) {
                printf("    Colors remaining: %u\r", tree->leaf_count);
                fflush(stdout);
            }
        }
        printf("    Reduction complete!      \n");
    }
    
    printf("  Final colors: %u\n", tree->leaf_count);
    
    /* Build the color palette */
    rgb_t palette[MAX_COLORS];
    uint32 num_colors = 0;
    octree_build_palette(tree, palette, &num_colors);
    
    /* Create the output indexed image */
    bitmap dst = bitmap_create(src->width, src->height, BMF_INDEXED8, true);
    if (!dst) {
        printf("Error: Failed to create output bitmap\n");
        octree_destroy(tree);
        return NULL;
    }
    
    /* Copy palette */
    memcpy(dst->pal, palette, num_colors * sizeof(rgb_t));
    
    /* Map each pixel to the nearest color in the palette using octree lookup */
    printf("Mapping pixels to palette (octree lookup)...\n");
    
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
                
                uint8 palette_idx = 0;
                octree_get_palette_index(tree->root, r, g, b, 0, &palette_idx);
                dst->data[idx] = palette_idx;
                idx++;
            }
            
            /* Update progress per row instead of per pixel */
            if ((y & 31) == 0) {  /* Every 32 rows */
                printf("  Mapping: %u%%\r", (y * 100) / src->height);
                fflush(stdout);
            }
        }
    } else {
        /* Non-dithered mapping */
        for (uint32 i = 0; i < total_pixels; i++) {
            uint8 r = data[i * 3 + 0];
            uint8 g = data[i * 3 + 1];
            uint8 b = data[i * 3 + 2];
            
            uint8 palette_idx = 0;
            octree_get_palette_index(tree->root, r, g, b, 0, &palette_idx);
            dst->data[i] = palette_idx;
        }
    }
    
    printf("  Mapping: 100%%\n");
    
    printf("\nQuantization complete!\n");
    
    /* Clean up */
    octree_destroy(tree);
    
    return dst;
}

/* Main program */
int main(int argc, char *argv[]) {
    char *input_file;
    char *output_file;
    uint32 max_colors = 256;
    int ditherStrength = 0;  /* 0=off, 1-9=strength levels */
    
    printf("Octree Color Quantization\n");
    printf("=========================\n\n");
    
    /* Parse command line arguments */
    if (argc < 3) {
        printf("Usage: %s <input.bmp> <output.bmp> [colors] [-d[1-9]]\n", argv[0]);
        printf("  input.bmp  - Input 24-bit RGB bitmap file\n");
        printf("  output.bmp - Output 8-bit indexed bitmap file\n");
        printf("  colors     - Number of colors in palette (default: 256, max: 256)\n");
        printf("  -d or -dither: enable dithering (default strength=2)\n");
        printf("  -d1 to -d9: enable dithering with strength 1-9\n");
        return 1;
    }
    
    input_file = argv[1];
    output_file = argv[2];
    
    /* Parse optional arguments */
    for (int i = 3; i < argc; i++) {
        if (argv[i][0] == '-' && (argv[i][1] == 'd' || argv[i][1] == 'D')) {
            /* Dither flag */
            if (argv[i][2] >= '1' && argv[i][2] <= '9')
                ditherStrength = argv[i][2] - '0';
            else if (!strcmp(argv[i], "-dither") || !strcmp(argv[i], "-d"))
                ditherStrength = 2;  /* default strength */
        } else {
            /* Assume it's the color count */
            max_colors = atoi(argv[i]);
            if (max_colors < 2 || max_colors > MAX_COLORS) {
                printf("Warning: colors must be between 2 and %d, using default 256\n", MAX_COLORS);
                max_colors = 256;
            }
        }
    }
    
    printf("Input file:  %s\n", input_file);
    printf("Output file: %s\n", output_file);
    printf("Max colors:  %u\n\n", max_colors);
    
    /* Load the input image */
    printf("Loading input image...\n");
    bitmap src = bmp_load(input_file);
    if (!src) {
        printf("Error: Failed to load input image '%s'\n", input_file);
        return 1;
    }
    
    printf("Image loaded: %ux%u, %u-bit\n", src->width, src->height, src->format);
    
    /* Quantize the image */
    bitmap dst = quantize_image(src, max_colors, ditherStrength);
    if (!dst) {
        printf("Error: Failed to quantize image\n");
        bitmap_destroy(&src);
        return 1;
    }
    
    /* Save the output image */
    printf("Saving output image...\n");
    if (!bmp_save(output_file, &dst)) {
        printf("Error: Failed to save output image '%s'\n", output_file);
        bitmap_destroy(&src);
        bitmap_destroy(&dst);
        return 1;
    }
    
    printf("Image saved successfully!\n");
    
    /* Clean up */
    bitmap_destroy(&src);
    bitmap_destroy(&dst);
    
    return 0;
}
