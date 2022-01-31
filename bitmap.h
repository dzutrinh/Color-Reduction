#ifndef	__BMP_H__
#define	__BMP_H__	(1)

#ifdef __cplusplus
extern "C" {
#endif

#include "image.h"

typedef	struct	_BMP_CONTEXT
{
	BMP_HEADER		hdr;
	BMP_INFO_HEADER	info;
	BMP_BITFIELDS	fields;
	rgba_t			palette[256];
	uint32			colors;
	uint32			rowsize;
	uint8			* scanline;
	FILE			* fp;
} BMP_CONTEXT;

/* BMP API */
BMP_CONTEXT * bmp_open(const char * filename);
BMP_CONTEXT * bmp_create(const char * filename);
void bmp_close(BMP_CONTEXT ** ctx);

bool bmp_setup_header(BMP_CONTEXT ** ctx, const bitmap * bmp);
bool bmp_get_header_block(BMP_CONTEXT ** ctx);
bool bmp_put_header_block(BMP_CONTEXT ** ctx);
bool bmp_get_info_block(BMP_CONTEXT ** ctx);
bool bmp_put_info_block(BMP_CONTEXT ** ctx);
bool bmp_get_row(BMP_CONTEXT ** ctx, uint8 * buf, uint32 len);
bool bmp_put_row(BMP_CONTEXT ** ctx, uint8 * buf, uint32 len);

#ifdef __cplusplus
}
#endif

#endif
