#ifndef __KDTREE_H__
#define __KDTREE_H__

#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

/* k-d tree node for 3D color space (RGB) */
typedef struct kdnode {
    rgb_t color;           /* RGB color value */
    uint8 index;           /* palette index */
    struct kdnode *left;   /* left child (lesser values) */
    struct kdnode *right;  /* right child (greater values) */
} kdnode_t;

/* k-d tree structure */
typedef struct {
    kdnode_t *root;
    uint32 size;           /* number of colors in tree */
} kdtree_t;

/* Create a k-d tree from a color palette */
kdtree_t *kdtree_create(rgb_t *palette, uint32 num_colors);

/* Destroy a k-d tree and free memory */
void kdtree_destroy(kdtree_t *tree);

/* Find the nearest color in the tree to the given RGB color */
uint8 kdtree_nearest(kdtree_t *tree, uint8 r, uint8 g, uint8 b);

/* Find nearest color and return distance squared */
uint8 kdtree_nearest_dist(kdtree_t *tree, uint8 r, uint8 g, uint8 b, uint32 *dist_sq);

#ifdef __cplusplus
}
#endif

#endif /* __KDTREE_H__ */
