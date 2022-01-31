/* TODO: - reading RLE compressed image */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitmap.h"

/*	bmp_open(): Open a Windows BMP image file for reading
 *
 *	Params:
 *		filename: image file name to open
 *	Returns:
 *		Context attatched to the opened image file on success
 *		NULL if error
 */
BMP_CONTEXT * bmp_open(const char * filename)
{
	BMP_CONTEXT * ctx = NULL;

	/* create a context */
	if (!(ctx = (BMP_CONTEXT *) malloc(sizeof(BMP_CONTEXT))))
	{
		return NULL;				/* not enough memory for context */
	}

	/* reset everything inside the context */
	memset(&ctx->hdr, 0, BMP_HEADER_SIZE);
	memset(&ctx->info, 0, BMP_INFO_HEADER_V3_SIZE);
	ctx->rowsize = 0;
	ctx->fields = BMPBF_UNKNOWN;
	ctx->scanline = NULL;

	/* open file for reading */
	if(!(ctx->fp = fopen(filename, "rb")))
	{
		free(ctx);					/* unable to open file */
		return NULL;
	}

	/* read up the header block first */
	if (!bmp_get_header_block(&ctx))
	{
		bmp_close(&ctx);			/* header read error */
		return NULL;
	}

	/* Windows BMP validation */
	if (ctx->hdr.signature != BMP_TYPE)
	{
		bmp_close(&ctx);			/* not a Windows BMP */
		return NULL;
	}

	/* read up the information header block */
	if (!bmp_get_info_block(&ctx))
	{
		bmp_close(&ctx);			/* header read error */
		return NULL;
	}

	/* detect total colors for the CLUT */
	switch (ctx->info.bitcount)
	{
	case 1:		/* monochrome bitmap */
		ctx->colors = 2;
		break;
	case 4:		/* 16 colors bitmap */
		ctx->colors = 16;
		break;
	case 8:		/* 256 colors bitmap */
		ctx->colors = 256;
		break;
	case 16:	/* RGB bitmap, no palette needed */
	case 24:
	case 32:
		ctx->colors = 0;
		break;
	default:	/* we ran into a corrupted bitmap */
		bmp_close(&ctx);
		return NULL;
		break;
	}

	/* load up the palette if presents */
	if (ctx->colors)
	{
		if (fread(ctx->palette, sizeof(rgba_t) * ctx->colors, 1, ctx->fp) != 1)
		{
			bmp_close(&ctx);
			return NULL;
		}
	}

	/* calculate the memory needed for each bitmap scanline */
	ctx->rowsize = ((ctx->info.width * ctx->info.bitcount+31)/32)*4;
	if (!(ctx->scanline = (uint8 *) malloc(ctx->rowsize)))
	{
		bmp_close(&ctx);			/* not enough memory */
	    return NULL;
	}

	/* jump to the bitmap data */
	fseek(ctx->fp, ctx->hdr.offset, SEEK_SET);
	return ctx;						/* return the context */
}

/*	bmp_create(): Open a Windows BMP image file for writing
 *
 *	Params:
 *		filename: image file name to create, any existed file will be
 *		overwritten.
 *	Returns:
 *		Context attatched to the created image file on success
 *		NULL if error
 */
BMP_CONTEXT * bmp_create(const char * filename)
{
	BMP_CONTEXT * ctx = (BMP_CONTEXT *) malloc(sizeof(BMP_CONTEXT));

	if (!ctx)
	{
		return NULL;
	}

	if(!(ctx->fp = fopen(filename, "wb")))
	{
		free(ctx);
		return NULL;
	}

	memset(&ctx->hdr, 0, BMP_HEADER_SIZE);
	memset(&ctx->info, 0, BMP_INFO_HEADER_V3_SIZE);
	ctx->rowsize = 0;
	ctx->scanline = NULL;

	return ctx;
}

/*	bmp_close(): Close a Windows BMP context
 *
 *	Params:
 *		ctx: context to be closed
 *	Returns:
 *		none
 */
 void bmp_close(BMP_CONTEXT ** ctx)
{
	if(!ctx)
	{
		return;
	}

	fclose((*ctx)->fp);

	if(!(*ctx)->scanline)
	{
		free((*ctx)->scanline);
		(*ctx)->scanline = NULL;
	}

	free((*ctx));
	(*ctx) = NULL;
}

/*	bmp_load(): Windows BMP easy loader. Supports loading of uncompressed
 *	1, 4, 8, 24 and 32-bit images.
 *
 *	Params:
 *		filename: Windows BMP file name to load
 *	Returns:
 *		bitmap contained the loaded image on success
 *		NULL if error
 */
bitmap bmp_load(const char * filename)
{
	BMP_CONTEXT 	* ctx;
	bitmap_format_t	fmt;
	bitmap			bmp = NULL;			/* Result bitmap */
	uint32			linew;
	bool			hasPal = false;

	/* open the Windows BMP image file */
	if (!(ctx = bmp_open(filename)))
	{
		return NULL;			/* error opening the file */
	}

	/* at the moment, we don't support reading compression BMPs */
	if (!(ctx->info.compress == 0 || ctx->info.compress == 3))
	{
		bmp_close(&ctx);
		return NULL;
	}

	/* determine the proper bitmap format */
	switch (ctx->info.bitcount)
	{
	case 1:
		fmt = BMF_BINARY; hasPal = true; break;
	case 4:
		fmt = BMF_INDEXED4; hasPal = true; break;
	case 8:
		fmt = BMF_INDEXED8; hasPal = true; break;
	case 16:
	case 24:
		fmt = BMF_RGB24; break;
	case 32:
		fmt = BMF_RGB32; break;
	default:
		bmp_close(&ctx);
		return NULL;
	}

	/* create a place holder for the upcoming bitmap */
	if (!(bmp = bitmap_create(ctx->info.width, ctx->info.height,
							  fmt, hasPal)))
	{
		bmp_close(&ctx);		/* not enuf memory */
	    return NULL;
	}

	/* fetch the color lookup table from the context */
	if (ctx->colors && bitmap_has_pal(&bmp))
	{
		for (int i = 0; i < ctx->colors; i++)
		{
			/* only take R, G, B and discard the A component */
			bmp->pal[i].r = ctx->palette[i].r;
			bmp->pal[i].g = ctx->palette[i].g;
			bmp->pal[i].b = ctx->palette[i].b;
		}
	}

	linew = bitmap_row_size(&bmp);
	/* read up each scanline and store it in the top-down order */
	for (int i = 0; i < ctx->info.height; i++)
	{
		uint32	index = (bmp->height-i-1)*linew;
		if (!bmp_get_row(&ctx, bmp->data+index, linew))
		{
			bmp_close(&ctx);
			bitmap_destroy(&bmp);
			return NULL;
		}
	}

	bmp_close(&ctx);
	return bmp;
}

/*	bmp_save(): Windows BMP easy writer. Supports saving of uncompressed
 *	1, 4, 8, 24 and 32-bit images.
 *
 *	Params:
 *		filename: Windows BMP file name to save
 *	Returns:
 *		true on success
 *		false if error
 */
bool bmp_save(const char * filename, const bitmap * bmp)
{
	BMP_CONTEXT * ctx;
	uint32		linew;

	if (!bmp)
	{
		return false;
	}

	/* open file for writing */
	if (!(ctx = bmp_create(filename)))
	{
		return false;
	}

	bmp_setup_header(&ctx, bmp);			/* header preparation */
	if (!bmp_put_header_block(&ctx))		/* write the header block */
	{
		bmp_close(&ctx);
		return false;
	}

	if(!bmp_put_info_block(&ctx))			/* write the information block */
	{
		bmp_close(&ctx);
		return false;
	}

	linew = bitmap_row_size(bmp);			/* determine source line width */
	for (int i = 0; i < (*bmp)->height; i++)
	{
		/* write a bitmap scanline down to file bottom-up */
		if (!bmp_put_row(&ctx,
						(*bmp)->data+((*bmp)->height-i-1)*linew,
						linew))
		{
			bmp_close(&ctx);
			return false;
		}
	}

	/* close the bitmap file */
	bmp_close(&ctx);
	return true;
}

bool bmp_get_row(BMP_CONTEXT ** ctx, uint8 * buf, uint32 len)
{
	uint8	* src   = (uint8*)   (*ctx)->scanline, * dst = buf;
	uint16	* src16 = (uint16 *) (*ctx)->scanline;
	uint32	* src32 = (uint32 *) (*ctx)->scanline;
	uint32	pixel;

	if (fread((*ctx)->scanline, (*ctx)->rowsize, 1, (*ctx)->fp) != 1)
		return false;

	switch ((*ctx)->info.bitcount)
	{
	case 1:
	case 4:
	case 8:
		memcpy(dst, src, len);	/* len = buffer size */
		break;
	case 16:
		switch ((*ctx)->fields)
		{
		case BMPBF_R5G6B5:
			for (int j = 0; j < (*ctx)->info.width*3; j+= 3)
			{
				pixel = (*src16++);
				dst[j+0] = ((pixel & (*ctx)->info.maskr) >> 11) << 3;
				dst[j+1] = ((pixel & (*ctx)->info.maskg) >> 5) << 2;
				dst[j+2] = ((pixel & (*ctx)->info.maskb) << 3);
			}
			break;
		case BMPBF_A1R5G5B5:
			for (int j = 0; j < (*ctx)->info.width*3; j+= 3)
			{
				pixel = (*src16++);
				dst[j+0] = ((pixel & (*ctx)->info.maskr) >> 10) << 3;
				dst[j+1] = ((pixel & (*ctx)->info.maskg) >> 5) << 3;
				dst[j+2] = ((pixel & (*ctx)->info.maskb) << 3);
			}
			break;
		case BMPBF_A4R4G4B4:
			for (int j = 0; j < (*ctx)->info.width*3; j+= 3)
			{
				pixel = (*src16++);
				dst[j+0] = ((pixel & (*ctx)->info.maskr) >> 8) << 4;
				dst[j+1] = ((pixel & (*ctx)->info.maskg) >> 4) << 4;
				dst[j+2] = ((pixel & (*ctx)->info.maskb) << 4);
			}
			break;
		default:	/* it must be X1R5G5B5 */
			for (int j = 0; j < (*ctx)->info.width*3; j+= 3)
			{
				pixel = (*src16++);
				dst[j+0] = ((pixel & 0x7c00) >> 10) << 3;
				dst[j+1] = ((pixel & 0x3e0) >> 5) << 3;
				dst[j+2] = (pixel & 0x1f) << 3;
			}
		}
		break;
	case 24:
		for (int j = 0; j < (*ctx)->info.width*3; j+= 3)
		{
			dst[j+2] = (*src++);
			dst[j+1] = (*src++);
			dst[j+0] = (*src++);
		}
		break;
	case 32:
		switch ((*ctx)->fields)
		{
		case BMPBF_A8R8G8B8:
			for (int j = 0; j < (*ctx)->info.width*4; j+= 4)
			{
				pixel = (*src32++);
				dst[j+1] = ((pixel & (*ctx)->info.maskr) >> 24);
				dst[j+2] = ((pixel & (*ctx)->info.maskg) >> 16);
				dst[j+3] = ((pixel & (*ctx)->info.maskb) >> 8);
				dst[j+0] = (pixel & 0xff);
			}
			break;
		default:
			for (int j = 0; j < (*ctx)->info.width*4; j+= 4)
			{
				dst[j+3] = (*src++);
				dst[j+2] = (*src++);
				dst[j+1] = (*src++);
				dst[j+0] = (*src++);
			}
			break;
		}
	}
	return true;
}

bool bmp_put_row(BMP_CONTEXT ** ctx, uint8 * buf, uint32 len)
{
	uint8 * src = buf, * dst = (*ctx)->scanline;
	switch ((*ctx)->info.bitcount)
	{
	case 1:
	case 4:
	case 8:
		memcpy((*ctx)->scanline, buf, len);	/* len = buffer size */
		break;
	case 16:
		break;
	case 24:
		for (int j = 0; j < (*ctx)->rowsize; j+=3)
		{
			dst[j+2] = (*src++);
			dst[j+1] = (*src++);
			dst[j+0] = (*src++);
		}
		break;
	case 32:
		for (int j = 0; j < (*ctx)->rowsize; j+=4)
		{
			dst[j+3] = (*src++);
			dst[j+2] = (*src++);
			dst[j+1] = (*src++);
			dst[j+0] = (*src++);
		}
		break;
	}
	if (fwrite((*ctx)->scanline, (*ctx)->rowsize, 1, (*ctx)->fp) != 1)
		return false;
	return true;
}

bool bmp_get_header_block(BMP_CONTEXT ** ctx)
{
	/* avoid structure alignment on modern C compilers,
	   read single field each time */
	if (fread(&(*ctx)->hdr.signature, 2, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->hdr.size,      4, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->hdr.reserved1, 2, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->hdr.reserved2, 2, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->hdr.offset,    4, 1, (*ctx)->fp) != 1) return false;

	return true;
}

bool bmp_put_header_block(BMP_CONTEXT ** ctx)
{
	/* avoid structure alignment on modern C compilers,
	   write single field each time */
	if (fwrite(&(*ctx)->hdr.signature, 2, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->hdr.size,      4, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->hdr.reserved1, 2, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->hdr.reserved2, 2, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->hdr.offset,    4, 1, (*ctx)->fp) != 1) return false;

	return true;
}

bool bmp_get_info_block(BMP_CONTEXT ** ctx)
{
	/* read the bitmap information block, field by field */
	if (fread(&(*ctx)->info.size,         4, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->info.width,        4, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->info.height,       4, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->info.planes,       2, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->info.bitcount,     2, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->info.compress,     4, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->info.imagesize,    4, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->info.xppm,         4, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->info.yppm,         4, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->info.clrused,      4, 1, (*ctx)->fp) != 1) return false;
	if (fread(&(*ctx)->info.clrimp,       4, 1, (*ctx)->fp) != 1) return false;

	/* read the V4.0 header */
	if ((*ctx)->info.size > BMP_INFO_HEADER_V3_SIZE)
	{
		if (fread(&(*ctx)->info.maskr,  4, 1, (*ctx)->fp) != 1) return false;
		if (fread(&(*ctx)->info.maskg,  4, 1, (*ctx)->fp) != 1) return false;
		if (fread(&(*ctx)->info.maskb,  4, 1, (*ctx)->fp) != 1) return false;

		/* detect the bitfield formats for both 16 and 32-bit images */
		switch ((*ctx)->info.maskr)
		{
		case 0x7800    : (*ctx)->fields = BMPBF_R5G6B5;   break;
		case 0x7c00    : (*ctx)->fields = BMPBF_A1R5G5B5; break;
		case 0x0f00    : (*ctx)->fields = BMPBF_A4R4G4B4; break;
		case 0xff000000: (*ctx)->fields = BMPBF_A8R8G8B8; break;
		}
	}

	return true;
}

bool bmp_put_info_block(BMP_CONTEXT ** ctx)
{
	/* store the bitmap information block, field by field */
	if (fwrite(&(*ctx)->info.size,         4, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->info.width,        4, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->info.height,       4, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->info.planes,       2, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->info.bitcount,     2, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->info.compress,     4, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->info.imagesize,    4, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->info.xppm,         4, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->info.yppm,         4, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->info.clrused,      4, 1, (*ctx)->fp) != 1) return false;
	if (fwrite(&(*ctx)->info.clrimp,       4, 1, (*ctx)->fp) != 1) return false;

	/* store the color palette */
	if ((*ctx)->info.bitcount <= 8)
	{
		if(fwrite(	(*ctx)->palette,
					(1 << (*ctx)->info.bitcount) * 4, 1,
					(*ctx)->fp) != 1)
			return false;
	}

	return true;
}

bool bmp_setup_header(BMP_CONTEXT ** ctx, const bitmap * bmp)
{
	(*ctx)->info.size		= BMP_INFO_HEADER_V3_SIZE;
	(*ctx)->info.width		= (*bmp)->width;
	(*ctx)->info.height		= (*bmp)->height;
	(*ctx)->info.planes		= 1;

	switch ((*bmp)->format)
	{
	case BMF_BINARY:	(*ctx)->info.bitcount = 1;	break;
	case BMF_INDEXED4:	(*ctx)->info.bitcount = 4;	break;
	case BMF_INDEXED8:	(*ctx)->info.bitcount = 8;	break;
	case BMF_RGB24:		(*ctx)->info.bitcount = 24;	break;
	case BMF_RGB32:		(*ctx)->info.bitcount = 32;	break;
	default: break;
	}

	(*ctx)->info.compress	= 0;
	(*ctx)->info.imagesize	= (*bmp)->height * bitmap_row_size(bmp);
	(*ctx)->info.xppm		= 2835;
	(*ctx)->info.yppm		= 2835;
	(*ctx)->info.clrused	= (1 << (*ctx)->info.bitcount);
	(*ctx)->info.clrimp		= 0;

	(*ctx)->hdr.signature	= BMP_TYPE;
	(*ctx)->hdr.reserved1	= 0;
	(*ctx)->hdr.reserved2	= 0;
	(*ctx)->hdr.offset		= BMP_HEADER_SIZE + BMP_INFO_HEADER_V3_SIZE;
	(*ctx)->hdr.size		= (*bmp)->height * bitmap_row_size(bmp) +
							  BMP_HEADER_SIZE +
							  BMP_INFO_HEADER_V3_SIZE;

	if ((*ctx)->info.bitcount <= 8)
	{
		if (!(*bmp)->pal)
		{
			return false;
		}

		for (int i = 0; i < (1 << (*ctx)->info.bitcount); i++)
		{
			(*ctx)->palette[i].r = (*bmp)->pal[i].r;
			(*ctx)->palette[i].g = (*bmp)->pal[i].g;
			(*ctx)->palette[i].b = (*bmp)->pal[i].b;
			(*ctx)->palette[i].a = 0;
		}
	}

	switch((*bmp)->format)
	{
	case BMF_BINARY:
	case BMF_INDEXED4:
	case BMF_INDEXED8:
		(*ctx)->hdr.size += (1 << (*ctx)->info.bitcount) * sizeof(rgba_t);
		(*ctx)->hdr.offset +=  (1 << (*ctx)->info.bitcount) * sizeof(rgba_t);
		break;
	default:
		break;
	}

	/* calculate the memory needed for each bitmap scanline */
	(*ctx)->rowsize = (((*ctx)->info.width * (*ctx)->info.bitcount+31)/32)*4;
	if (!((*ctx)->scanline = (uint8 *) malloc((*ctx)->rowsize)))
	{
	    return false;
	}

	return true;
}
