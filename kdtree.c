#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "kdtree.h"

/* Helper to calculate perceptually-weighted color distance
 * Uses a weighted Euclidean distance that approximates human color perception
 * Based on redmean approximation for better visual quality
 */
static inline uint32 color_distance_sq(uint8 r1, uint8 g1, uint8 b1, uint8 r2, uint8 g2, uint8 b2) {
    int dr = (int)r1 - (int)r2;
    int dg = (int)g1 - (int)g2;
    int db = (int)b1 - (int)b2;
    
    /* Redmean color distance formula - perceptually better than Euclidean
     * Weights channels based on human color perception
     * Red channel weight varies by average red value
     */
    int rmean = ((int)r1 + (int)r2) / 2;
    int wr = (512 + rmean);      /* red weight: 2.0 to 3.0 */
    int wg = 1024;                /* green weight: 4.0 (eyes most sensitive to green) */
    int wb = (767 - rmean);       /* blue weight: 2.0 to 3.0 */
    
    /* Calculate weighted squared distance, scaled down to prevent overflow */
    return (wr * dr * dr + wg * dg * dg + wb * db * db) >> 8;
}

/* Get color component by axis (0=R, 1=G, 2=B) */
static inline uint8 get_component(rgb_t *color, int axis) {
    switch (axis) {
        case 0: return color->r;
        case 1: return color->g;
        case 2: return color->b;
        default: return 0;
    }
}

/* Comparison function for qsort */
typedef struct {
    rgb_t color;
    uint8 index;
    int axis;
} color_sort_t;

static int compare_colors(const void *a, const void *b) {
    color_sort_t *ca = (color_sort_t *)a;
    color_sort_t *cb = (color_sort_t *)b;
    uint8 va = get_component(&ca->color, ca->axis);
    uint8 vb = get_component(&cb->color, cb->axis);
    return (int)va - (int)vb;
}

/* Build k-d tree recursively */
static kdnode_t *kdtree_build(color_sort_t *colors, int start, int end, int depth) {
    if (start > end) return NULL;
    
    int axis = depth % 3;  /* cycle through R, G, B */
    int mid = start + (end - start) / 2;
    
    /* Sort by current axis and find median */
    for (int i = start; i <= end; i++) {
        colors[i].axis = axis;
    }
    
    /* Simple interchange sort for small ranges, qsort for larger */
    if (end - start < 10) {
        for (int i = start; i < end; i++) {
            for (int j = i + 1; j <= end; j++) {
                if (get_component(&colors[i].color, axis) > get_component(&colors[j].color, axis)) {
                    color_sort_t temp = colors[i];
                    colors[i] = colors[j];
                    colors[j] = temp;
                }
            }
        }
    } else {
        qsort(&colors[start], end - start + 1, sizeof(color_sort_t), compare_colors);
    }
    
    /* Create node */
    kdnode_t *node = (kdnode_t *)malloc(sizeof(kdnode_t));
    if (!node) return NULL;
    
    node->color = colors[mid].color;
    node->index = colors[mid].index;
    node->left = kdtree_build(colors, start, mid - 1, depth + 1);
    node->right = kdtree_build(colors, mid + 1, end, depth + 1);
    
    return node;
}

/* Create a k-d tree from a color palette */
kdtree_t *kdtree_create(rgb_t *palette, uint32 num_colors) {
    if (!palette || num_colors == 0) return NULL;
    
    kdtree_t *tree = (kdtree_t *)malloc(sizeof(kdtree_t));
    if (!tree) return NULL;
    
    tree->size = num_colors;
    
    /* Create temporary array for building */
    color_sort_t *colors = (color_sort_t *)malloc(sizeof(color_sort_t) * num_colors);
    if (!colors) {
        free(tree);
        return NULL;
    }
    
    /* Copy palette data */
    for (uint32 i = 0; i < num_colors; i++) {
        colors[i].color = palette[i];
        colors[i].index = i;
        colors[i].axis = 0;
    }
    
    /* Build the tree */
    tree->root = kdtree_build(colors, 0, num_colors - 1, 0);
    
    free(colors);
    return tree;
}

/* Destroy k-d tree recursively */
static void kdtree_destroy_recursive(kdnode_t *node) {
    if (!node) return;
    kdtree_destroy_recursive(node->left);
    kdtree_destroy_recursive(node->right);
    free(node);
}

/* Destroy a k-d tree and free memory */
void kdtree_destroy(kdtree_t *tree) {
    if (!tree) return;
    kdtree_destroy_recursive(tree->root);
    free(tree);
}

/* Find nearest neighbor recursively */
static void kdtree_search(kdnode_t *node, uint8 r, uint8 g, uint8 b, int depth,
                         uint8 *best_index, uint32 *best_dist) {
    if (!node) return;
    
    /* Calculate distance to current node */
    uint32 dist = color_distance_sq(r, g, b, node->color.r, node->color.g, node->color.b);
    
    if (dist < *best_dist) {
        *best_dist = dist;
        *best_index = node->index;
        
        /* Early exit if exact match */
        if (dist == 0) return;
    }
    
    /* Determine which side to search first */
    int axis = depth % 3;
    uint8 split_val = get_component(&node->color, axis);
    uint8 search_val;
    
    switch (axis) {
        case 0: search_val = r; break;
        case 1: search_val = g; break;
        case 2: search_val = b; break;
        default: search_val = 0;
    }
    
    kdnode_t *near_node = (search_val < split_val) ? node->left : node->right;
    kdnode_t *far_node = (search_val < split_val) ? node->right : node->left;
    
    /* Search near side */
    kdtree_search(near_node, r, g, b, depth + 1, best_index, best_dist);
    
    /* Check if we need to search far side */
    int axis_dist = (int)search_val - (int)split_val;
    if ((uint32)(axis_dist * axis_dist) < *best_dist) {
        kdtree_search(far_node, r, g, b, depth + 1, best_index, best_dist);
    }
}

/* Find the nearest color in the tree to the given RGB color */
uint8 kdtree_nearest(kdtree_t *tree, uint8 r, uint8 g, uint8 b) {
    if (!tree || !tree->root) return 0;
    
    uint8 best_index = 0;
    uint32 best_dist = UINT_MAX;
    
    kdtree_search(tree->root, r, g, b, 0, &best_index, &best_dist);
    
    return best_index;
}

/* Find nearest color and return distance squared */
uint8 kdtree_nearest_dist(kdtree_t *tree, uint8 r, uint8 g, uint8 b, uint32 *dist_sq) {
    if (!tree || !tree->root) {
        if (dist_sq) *dist_sq = UINT_MAX;
        return 0;
    }
    
    uint8 best_index = 0;
    uint32 best_dist = UINT_MAX;
    
    kdtree_search(tree->root, r, g, b, 0, &best_index, &best_dist);
    
    if (dist_sq) *dist_sq = best_dist;
    return best_index;
}
