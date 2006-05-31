/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   --------------------------------------------------------------------  *
 *       Handles .BMP bitmap file load/save processing.                    *
 *       Used some .bmp articles from the 'net, describing the format      *
 *                                                                         *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "MyGlobal.h"
#include "MyTypes.h"
#include "bitmap.h"
#include "common.h"

/**************************************
 * Internal Proc - Create Raw         *
 *------------------------------------*
 *  x - desired width                 *
 *  y - desired height                *
 *------------------------------------*
 * Function: allocates and fills with *
 * black the bitmap                   *
 *------------------------------------*
 * @returns false on failure          *
 **************************************/

bool RawImg:: CreateRaw(int x, int y)
{
 int asize;
 this->x = x;
 this->y = y;
 if (data)
 	free(data);
 asize = x * y * 4;
 asize += 16; // leaves some additional unused space beyond the end of the texture
 data =  malloc(asize);
 if (data == NULL) {
	return false;
 	}
 int *iData = (int*)data;
 for (int i=0;i<x*y;i++) iData[i] = 0;
 return true;
}

void RawImg:: copyraw(const RawImg & r)
{
 CreateRaw(r.x, r.y);
 memcpy(data, r.data, x*y*4);
}

void RawImg:: default_init()
{
 fn[0] = 0;
 data = NULL;
 x = y = 0;
}


/**********************************
 * RawImg default constructor     *
 **********************************/

RawImg:: RawImg()
{
 default_init();
}

/**********************************
 * Data-initialized RawImg        *
 * constructor. Copies the given  *
 * image data to the RawImg       *
 * structure with the specified   *
 * dimensions                     *
 **********************************/
 RawImg:: RawImg(int x, int y, void * data)
 {
  default_init();
  CreateRaw(x,y);
  if (data) {
  	memcpy(this->data, data, x*y*4);
  }
 }


/**********************************
 * RawImg default destructor:     *
 * Frees the memory allocated for *
 * the data field of the given    *
 * RawImg                         *
 **********************************/
RawImg:: ~RawImg()
{
 close();
}

void RawImg:: close(void)
{
	if (data)
		free(data);
	data = NULL;
}


RawImg:: RawImg(const RawImg &r)
{
 copyraw(r);
}

RawImg & RawImg:: operator = (const RawImg & r)
{
	if (&r != this) {
		if (data)
			free(data);
		copyraw(r);
	}
	return *this;
}


/***************************************
 * Proc - Shrinks the RawImg to a new  *
 *        RawImg b, with 1:2 ratio     *
 *-------------------------------------*
 * *this - The source RawImg structure *
 * b     - Destination RawImg          *
 ***************************************/
void RawImg:: Shrink(RawImg &b)
{int i,j,p=0;
 int sum[MAX_TEXTURE_SIZE][3];
 int *adata = (int*) data;
 if (!b.CreateRaw(x/2, y/2)) return;

 int *bdata = (int*) b.data;
 for (j=0;j<y;j++) {
	if (!(j&1))	memset(sum, 0, 3*sizeof(int)*(x/2));
 	for (i=0;i<x;i++) {
		sum[i/2][0] +=  adata[p] & 0x0000ff;
		sum[i/2][1] +=  adata[p] & 0x00ff00;
		sum[i/2][2] +=  adata[p] & 0xff0000;
		p++;
		if (i&j&1)
			bdata[(i/2)+(j/2)*(x/2)] =      (((sum[i/2][0]+0x000002)>>2 )&0x0000ff) +
							(((sum[i/2][1]+0x000200)>>2 )&0x00ff00) +
							(((sum[i/2][2]+0x020000)>>2 )&0xff0000)  ;
		}
	}
 	bdata[b.x*b.y] = bdata[b.x*b.y - 1];
/*
	The last "hack" is needed for the SSE version of drawit. If you carefully trace the source, you will see
	that the bilinear filtering version gets 2 pixels with one `movq', which is not identical to the
	P5 code, which gets pixels one by one and mods the index of the pixel in the texture by the texture size (so
	no texel outside of the texture is taken). In the SSE version the second pixel is not checked for being
	outside the texture (but it can be just the pixel behind the last in the texture memory), so we copy the
	last pixel to the position after it. This is not correct, but does the job.
*/
}

/**************************************
 * Function - Loads a .bmp file into  *
 *            a RawImg                *
 * Supports 8-,24- and 32-bit images  *
 *------------------------------------*
 * fn - The path to the file.         *
 *------------------------------------*
 * On failure, LoadBmp returns false. *
 **************************************/
bool RawImg:: LoadBmp(const char *fn)
{FILE *f;
 int i, j, p, k;
 BMHEADER hd;
 BMiHEADER hi;
 int pal[256];
 int toread = 0;
 unsigned char *xx;
 int rowsz;
 unsigned short sign;

 if ((f = fopen(fn, "rb"))==NULL) {
 	printf("LoadBmp:Can't open file: `%s'\n", fn);
 	return false;
	}
 if (!fread(&sign, 2, 1, f)) return false;
 if (sign!=itohs(BM_MAGIC)) { printf("LoadBmp:`%s' is not a BMP file.\n", fn); return false; }
 if (!fread(&hd, sizeof(hd), 1, f)) return false;
 if (!fread(&hi, sizeof(hi), 1, f)) return false;

 /* header correctness checks */
 if (!(itohs(hi.bitsperpixel)==8 || itohs(hi.bitsperpixel)==24 || itohs(hi.bitsperpixel)==32)){
	 printf("LoadBmp:Cannot handle file format at %d bpp.\n",itohs(hi.bitsperpixel)); 
	 return false;
 }
 if (itohs(hi.channels)!=1) {
 	printf("LoadBmp: cannot load multichannel .bmp!\n");
	return false;
 }
 /* ****** header is OK *******/

 // if image is 8 bits per pixel or less (indexed mode), read some pallete data
 if (itohs(hi.bitsperpixel) <= 8) {
	toread = (1<<itohs(hi.bitsperpixel));
	if (itohl(hi.colors)) toread=itohl(hi.colors);
 	if (!fread(pal, toread, 4, f)) return false;
 }
 toread = itohl(hd.bfImgOffset) - (54 + toread*4);
 if (toread)
 // skip the rest of the header
 	for (i=0;i<toread;i++)
		fgetc(f);
 k = itohs(hi.bitsperpixel) / 8;
 rowsz = itohl(hi.x) * k;
 if (rowsz%4)
 	rowsz = (rowsz / 4 + 1) * 4; // round the row size to the next exact multiple of 4
 xx = (unsigned char *) malloc(rowsz);
 p = itohl(hi.x) * itohl(hi.y);
 if (!CreateRaw(itohl(hi.x), itohl(hi.y))) {
 	printf("LoadBmp: cannot allocate memory for bitmap! Check file integrity!\n");
	fflush(stdout);
	free(xx);
	fclose(f);
	return false;
 	}
#ifdef DEBUG
 printf("LoadBmp: Loading \"%s\":%dx%dx%dbit BMP...", fn, itohl(hi.x), itohl(hi.y), itohs(hi.bitsperpixel)); fflush(stdout);
#endif
 int *adata = (int*)data;
 for (j=itohl(hi.y)-1;j>=0;j--) {// bitmaps are saved in inverted y
	if (!fread(xx, 1, rowsz, f)) {
		printf("LoadBmp: short read while opening `%s', file is probably incomplete!\n", fn);
		free(xx);
		fclose(f);
		return 0;
		}
	p -= itohl(hi.x);
	for (i=0;i<itohl(hi.x);i++){ // actually read the pixels
		if (itohs(hi.bitsperpixel)>8)
			adata[p++] = xx[i*k] + ((int) xx[i*k+1])*256 + ((int) xx[i*k+2])*65536;
			else
			adata[p++] = pal[xx[i*k]];
		}
		p -= itohl(hi.x);
 	}
 fclose(f);
 free(xx);
#ifdef DEBUG
 printf("OK\n");
 fflush(stdout);
#endif
 strcpy(this->fn, fn);
 return true;
}

/**************************************
 * Function - Loads a .fda file from  *
 *            disk                    *
 *------------------------------------*
 * @param fn - file name              *
 *------------------------------------*
 * @returns true on success, false on *
 *  failure                           *
 **************************************/

bool RawImg:: LoadFda(const char *fn)
{FILE *f;
 FDAHeaDeR FDA;
 if (( f = fopen(fn, "rb"))==NULL) {
 	printf("LoadFda: cannot open file: `%s'\n", fn);
	return false;
 }
 if (!fread(&FDA, sizeof(FDA), 1, f)) {
 	printf("LoadFda: short read while reading the header; file is broken\n");
	fclose(f);
	return false;
 }
 if (FDA.x_size <= 1 || power_of_2(FDA.x_size)==-1 ||
     FDA.y_size <= 1 || power_of_2(FDA.y_size)==-1) {
     	printf("LoadFda: bad .FDA file (dimensions are incorrect (%d,%d))\n", FDA.x_size, FDA.y_size);
	fclose(f);
	return false;
     }
 CreateRaw(FDA.x_size, FDA.y_size);
 unsigned totsize = FDA.x_size * FDA.y_size;
 if (totsize != fread(data, sizeof(float), totsize, f)) {
 	printf("LoadFda: short read while reading image info; file may be broken or incomplete\n");
	fclose(f);
	return false;
 }
 int __blah;
 if (0 != fread(&__blah, 1,1,f)) {
 	printf("LoadFda: warning: file is bigger than expected\n");
 }
 fclose(f);
 return true;
}

/**************************************
 * Function - Writes a .bmp file from *
 *            a RawImg                *
 *------------------------------------*
 * fn - The path to the file.         *
 *------------------------------------*
 * On failure, SaveBmp returns 0.     *
 **************************************/
bool RawImg:: SaveBmp(const char *fn)
{FILE *f;
 int i,j, rowsz;
 unsigned short msh=BM_MAGIC;
 BMHEADER hd;
 BMiHEADER hi;
 char xx[RES_MAXX*3];


 if ((f=fopen(fn, "wb"))==NULL) {
	printf("SaveBmp:can't open `%s' for writing, check permissions!\n", fn);
	return false;
	}
 // fill in the header:
 rowsz = x * 3;
 if (rowsz % 4)
 	rowsz += 4 - (rowsz % 4); // each row in of the image should be filled with zeroes to the next multiple-of-four boundary
 hd.fs = rowsz*y + 54; //std image size :)
 hd.lzero = 0;
 hd.bfImgOffset = 54;
 hi.ihdrsize = 40;
 hi.x = x; hi.y = y;
 hi.channels = 1;
 hi.bitsperpixel = 24; //RGB format
 // set the rest of the header to default values:
 hi.compression = hi.biSizeImage = 0;
 hi.pixPerMeterX = hi.pixPerMeterY = 0;
 hi.colors = hi.colorsImportant = 0;

 fwrite(&msh, 2, 1, f); // write 'BM'
 fwrite(&hd, sizeof(hd), 1, f); // write file header
 fwrite(&hi, sizeof(hi), 1, f); // write image header
 int *adata = (int*) data;
 for (j=y-1;j>=0;j--) {
 	for (i=0;i<x;i++) {
		xx[i*3  ] = (0xff     & adata[j*x+i]);
		xx[i*3+1] = (0xff00   & adata[j*x+i]) >> 8;
		xx[i*3+2] = (0xff0000 & adata[j*x+i]) >> 16;
		}
        fwrite(xx, rowsz, 1, f);
 	}
 fclose(f);
 return true;
}

void RawImg:: RenderHack()
{
 int * adata = (int*) data;
 adata[x*y]=adata[x*y-1];
 // this is the `awesome' hack needed by render_floor
 // consult Shrink() method for more info
}

void Take_Snapshot(RawImg & a)
{int i=1;
 char fn[30];
 FILE *f;

 while (42) {
	sprintf(fn, "fract_snapshot%02d.bmp", i);
	f = fopen(fn, "rb");
	if (f==NULL) break;
	   else fclose(f);
	i++;
 	}
 a.SaveBmp(fn);
}

RawImg:: RawImg(const char*fn)
{
	default_init();
	int i = strlen(fn)-1;
	while (i>0 && fn[i]!='.') i--;
	const char *extptr = fn + i + 1;
	if (!strcmp_without_case("BMP", extptr)) {
		LoadBmp(fn);
		return;
	}
	if (!strcmp_without_case("FDA", extptr)) {
		LoadFda(fn);
		return ;
	}
	printf("Unknown file type: .%s\n", extptr);
	CreateRaw(64,64);
}

void RawImg:: fill(unsigned color)
{
	Uint32 * dat = (Uint32*)data;
	for (int i = 0; i < x*y; i++)
		dat[i] = color;
}

void gridify_texture(RawImg & a)
{
	Uint32 *data = (Uint32* )a.get_data();
	int idx = 0;
	for (int j = 0; j < a.get_y(); j++) {
		for (int i = 0; i < a.get_x(); i++,idx++) {
			unsigned x = 255*(((i&15)==0)||((j&15)==0));
			data[idx] = x | (x << 8) | (x << 16);
		}
	}
}

void checker_texture(RawImg & a, int size, unsigned color1, unsigned color2)
{
	Uint32 *data = (Uint32 *) a.get_data();
	int idx = 0;
	for (int j = 0; j < a.get_y(); j++) {
		for (int i = 0; i < a.get_x(); i++,idx++) {
			int sb = (i / size + j / size) % 2;
			data[idx] = sb ? color1 : color2;
		}
	}
}

void bitmap_close(void)
{
}

Uint32 byteswap_32(Uint32 x)
{
	x = ((x & 0xffff) << 16) | ((x >> 16) & 0xffff);
	x = ((x & 0x00ff00ff) << 8) | ((x >> 8) & 0x00ff00ff);
	return x;
}
Uint16 byteswap_16(Uint16 x)
{
	x = ((x & 0xff) << 8) | ((x >> 8) & 0xff);
	return x;
}
