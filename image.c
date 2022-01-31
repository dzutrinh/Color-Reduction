#include <stdio.h>
#include <stdlib.h>
#include "image.h"

/*  bitmap_create()
*   creates a bitmap on memory and return its handle
*/
bitmap bitmap_create(uint32 width, uint32 height, bitmap_format_t format, bool hasPal)
{
    bitmap  bmp = (bitmap) malloc(sizeof(bitmap_t));    /* allocates bitmap handle */
    uint32  linew = width;

    if (!bmp) return NULL;  /* not enough memory ? */

    bmp->format = format;   /* update the bitmap format */
    bmp->width = width;     /* update the bitmap's width in pixels */
    bmp->height = height;   /* update the bitmap's height in pixels */

    /* allocates the color palette if requested */
    if (hasPal)
    {
        bmp->pal = (rgb_t *) malloc(768);
        if (!bmp->pal)      /* not enough memory? */
        {
            free(bmp);      /* releases the bitmap handle */
            return NULL;    /* and returns nothing */
        }
    }
    else
    {
        bmp->pal = NULL;    /* no color palette requested */
    }

    /* calculates the correct bitmap storage size and the width of each
       scanline */
    switch (format)
    {
    case BMF_BINARY:    linew = (linew+7)/8; break;
    case BMF_INDEXED2:  linew = (linew+3)/4; break;
    case BMF_INDEXED4:  linew = (linew+1)/2; break;
    case BMF_INDEXED8:  break;  /* nothing to do here, it's already there */
    case BMF_RGB24:     linew *= 3; break;
    case BMF_RGB32:     linew *= 4; break;
    }
    bmp->size = height * linew;   /* update the correct size */
    bmp->rowsize = linew;

    /* allocates the bitmap bits */
    bmp->data = (uint8 *) malloc(bmp->size);
    if (!bmp)           /* not enough memory */
    {
        /* releases the color palette if present */
        if (bmp->pal)
        {
            free(bmp->pal);
        }
        /* and returns nothing */
        return NULL;
    }

    /* everything is ok for now, return the created bitmap */
    return  bmp;
}

/*  bitmap_destroy()
*   destroys a bitmap and releases all it occuppied memory
*/
void bitmap_destroy(bitmap * bmp)
{
    /* a valid bitmap provided? */
    if ((*bmp))
    {
        /* releases the color palette if present */
        if ((*bmp)->pal)
        {
            free((*bmp)->pal);
            (*bmp)->pal = NULL;
        }

        /* releases the bitmap bits if present */
        if ((*bmp)->data)
        {
            free((*bmp)->data);
            (*bmp)->data = NULL;
        }

        free((*bmp));
        (*bmp) = NULL;      /* safety practice :D */
    }
}

uint32 bitmap_row_size(const bitmap * bmp)
{
    return (*bmp)->rowsize;
}

bool bitmap_has_pal(const bitmap * bmp)
{
    return ((*bmp)->pal != NULL);
}
