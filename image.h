#ifndef __IMAGE_H__
#define __IMAGE_H__ (1)

#ifdef __cplusplus
extern "C" {
#endif

//#define DEBUG

/*------------------------ CROSS-PLATFORM PRIMITIVES -------------------------*/
#ifndef __cplusplus
    typedef int bool;
    #define false (0)
    #define true (1)
#endif

#ifndef uint8
    typedef unsigned char uint8;
#endif

#ifndef uint16
    #ifdef MSDOS
        typedef unsigned int uint16;
    #else
        typedef unsigned short uint16;
    #endif
#endif

#ifndef uint32
    #ifdef MSDOS
        typedef unsigned long uint32;
    #else
        typedef unsigned int uint32;
    #endif
#endif

typedef enum {  IMR_OK = 0,
                IMR_FILE_NOT_FOUND,
                IMR_FILE_CREATE_ERROR,
                IMR_FILE_CORRUPTED,
                IMR_FORMAT_INVALID,
                IMR_FORMAT_UNSUPPORTED,
                IMR_NOT_ENOUGH_MEMORY,
                IMR_UNKNOWN_ERROR
            } image_result_t;

/*
extern image_result_t image_result;
*/

/*------------------------- COLOR LOOKUP TABLE ENTRY -------------------------*/
typedef struct _rgb
{
    uint8 r;
    uint8 g;
    uint8 b;
} rgb_t;

typedef struct _rgba
{
    uint8 r;
    uint8 g;
    uint8 b;
    uint8 a;
} rgba_t;

/*------------------------------ MEMORY BITMAP -------------------------------*/

typedef enum {  BFM_BMP,
                BFM_CEL,
                BFM_COL,
                BFM_GIF,
                BFM_PCX,
                BFM_PIC,
                BFM_PNG,
                BFM_RAW,
                BFM_RIX,
                BFM_SGI,
                BFM_TGA,
                BFM_TIF} bitmap_type_t;

typedef enum {  BCF_NONE,
                BCF_RLE,
                BCF_LZW,
                BCF_HUFFMAN,
                BCF_PACKBITS} bitmap_compress_t;

typedef struct _bitmap_info
{
    bitmap_compress_t   compress;
    bitmap_type_t       type;
    uint32              version;
    uint32              width;
    uint32              height;
    uint32              bitcount;
} bitmap_info_t;

typedef enum {BMF_BINARY = 1,
              BMF_INDEXED2 = 2,
              BMF_INDEXED4 = 4,
              BMF_INDEXED8 = 8,
              BMF_RGB24 = 24,
              BMF_RGB32 = 32} bitmap_format_t;

typedef struct _bitmap
{
    bitmap_format_t format;     /* bitmap format */
    uint32  width;              /* bitmap's width in pixels */
    uint32  height;             /* bitmap's height in pixels */
    uint32  size;               /* bitmap's size = width x height */
    uint32  rowsize;            /* width in bytes of each scanline */
    rgb_t   * pal;              /* accompanied color palette */
    uint8   * data;             /* bitmap's bits */
} * bitmap, bitmap_t;

bitmap  bitmap_create (uint32 width, uint32 height, bitmap_format_t format, bool hasPal);
void    bitmap_destroy(bitmap * bmp);
uint32  bitmap_row_size(const bitmap * bmp);
bool    bitmap_has_pal(const bitmap * bmp);

/*----------------------------- WINDOWS BITMAP -------------------------------*/
#define	BMP_TYPE				(0x4D42)
#define BMP_HEADER_SIZE			(14)
#define	BMP_INFO_HEADER_V3_SIZE	(40)
#define	BMP_INFO_HEADER_V4_SIZE	(108)

typedef struct _BMP_HEADER
{
	uint16 signature;	//	2
	uint32 size;		//	4
	uint16 reserved1;	//	2
	uint16 reserved2;	//	2
	uint32 offset;		//	4
} BMP_HEADER;

typedef	struct _BMP_INFO_HEADER
{
    uint32  size;			//	4
    uint32  width;			//	4
    uint32  height;			//	4
    uint16  planes;			//	2
    uint16  bitcount;		//	2
    uint32  compress;		//	4
    uint32  imagesize;		//	4
    uint32  xppm;			//	4
    uint32  yppm;			//	4
    uint32  clrused;		//	4
    uint32  clrimp;			//	4

    /* Fields added for Windows 4.x follow this line */
    uint32  maskr;
    uint32  maskg;
    uint32  maskb;
    uint32  maskalpha;
    uint32  colorspace;
    long    xr;
    long    yr;
    long    zr;
    long    xg;
    long    yg;
    long    zg;
    long    xb;
    long    yb;
    long    zb;
    uint32  gammar;
    uint32  gammag;
    uint32  gammab;
} BMP_INFO_HEADER;

/* only valid for Windows Bitmap v3.x and 4.x */
typedef	enum {	/* 16-bit */
                BMPBF_UNKNOWN,      /* for generic 16-bit v3.x bitmap */
				BMPBF_A1R5G5B5,     /* 4.x */
				BMPBF_A4R4G4B4,     /* 4.x */
				BMPBF_R5G6B5,       /* 4.x */
				/* 32-bit */
			 	BMPBF_A8R8G8B8      /* 4.x */} BMP_BITFIELDS;

/*---------------------------- PORTABLE ANY MAP ------------------------------*/
typedef enum {PNM_P1, PNM_P2, PNM_P3, PNM_P4, PNM_P5, PNM_P6} PNM_TYPE;

typedef struct _PNM_HEADER
{
    uint8    signature[2];
    uint32   width;
    uint32   height;
    uint16   bitcount;
    uint32   pixelsize;
    PNM_TYPE type;
} PNM_HEADER;

#define PNM_HEADER_SIZE (16)

/*------------------------- EASY LOADERS/WRITERS -----------------------------*/
bitmap	image_load(const char * filename);
bool	image_save(const char * filename, const bitmap * bmp);
bool    image_info(const char * filename, bitmap_info_t * info);
bitmap_type_t image_detect(const char * filename);

/*--------------------- SPECIFIC TYPE LOADERS/WRITERS ------------------------*/
bitmap	bmp_load(const char * filename);
bool	bmp_save(const char * filename, const bitmap * bmp);

bitmap  pnm_load(const char * filename);
bool    pnm_save(const char * filename, const bitmap * bmp);

#ifdef __cplusplus
}
#endif

#endif
