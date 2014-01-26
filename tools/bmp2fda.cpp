/***************************************************************************
 *   Copyright (C) 2005 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>



// The implementation of the BMP handling is copied from fract

// the 'BM' magic in the beginning of a .BMP phile
#define MAX_TEXTURE_SIZE 1024
#define RES_MAXX 1280
#define BM_MAGIC 19778

typedef unsigned short Uint16;
typedef unsigned int   Uint32;


/// basic bitmap class
class RawImg {
	int x, y;
	void * data;
	char fn[64];//optional if the RawImg was loaded from a file

	bool CreateRaw(int x, int y);
	void copyraw(const RawImg &);
	void default_init();
public:

	RawImg();
	~RawImg();

	/// loads from file. Supported formats: BMP FDA
	RawImg(const char*);

	/// Creates a bitmap from the source image data with the
	/// specified dimensions
	/// if `data' is NULL the bitmap is black-filled
	/// if not the data is copied to the RawImg structure
	RawImg(int x, int y, void *data = NULL);


	RawImg(const RawImg &);
	RawImg & operator = (const RawImg &);

	/// loads from a BMP file
	bool LoadBmp(const char * fn);

	/// saves a bitmap to a .BMP file
	bool SaveBmp(const char * fn);

	void SaveFda(const char * fn);

	/// Shrinks the source bitmap twice and puts it in B
	void Shrink(RawImg &b);


	/// access functions:
	int get_x() const {return x;}
	int get_y() const {return y;}
	int get_size() const { return x*y;}
	/// returns a pointer to the drawable area;
	/// note that it may be null
	void* get_data() const {return data;}

	const char* getTexFn() const { return fn;}

	void RenderHack();
};

// the .BMP header structure is taken from http://www.fortunecity.com/skyscraper/windows/364/bmpffrmt.html
typedef struct {
int fs;       // filesize
int lzero;
int bfImgOffset;  // basic header size
}BMHEADER;

typedef struct {
int ihdrsize; 	// info header size
int x,y;      	// image dimensions
Uint16 channels;// number of planes
Uint16 bitsperpixel;
int compression; // 0 = no compression
int biSizeImage; // used for compression; otherwise 0
int pixPerMeterX, pixPerMeterY; // dots per meter settings (why meters and not inches... maybe an attempt to conform to SI? :)
int colors;	 // number of used colors. If all specified by the bitsize are used, then it should be 0
int colorsImportant; // number of "important" colors (All in wonder..)
}BMiHEADER;


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
 if (data)
	free(data);
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
 if (sign!=BM_MAGIC) { printf("LoadBmp:`%s' is not a BMP file.\n", fn); return false; }
 if (!fread(&hd, sizeof(hd), 1, f)) return false;
 if (!fread(&hi, sizeof(hi), 1, f)) return false;

 /* header correctness checks */
 if (!(hi.bitsperpixel==8 || hi.bitsperpixel==24 || hi.bitsperpixel==32))
 	{printf("LoadBmp:Cannot handle file format at %d bpp.\n",hi.bitsperpixel); return false ; }
 if (hi.channels!=1) {
 	printf("LoadBmp: cannot load multichannel .bmp!\n");
	return false;
 	}
 /* ****** header is OK *******/

 // if image is 8 bits per pixel or less (indexed mode), read some pallete data
 if (hi.bitsperpixel <= 8) {
 	toread = (1<<hi.bitsperpixel);
 	if (hi.colors) toread=hi.colors;
 	if (!fread(pal, toread, 4, f)) return false;
	}
 toread = hd.bfImgOffset - (54 + toread*4);
 if (toread)
 // skip the rest of the header
 	for (i=0;i<toread;i++)
		fgetc(f);
 k = hi.bitsperpixel / 8;
 rowsz = hi.x * k;
 if (rowsz%4)
 	rowsz = (rowsz / 4 + 1) * 4; // round the row size to the next exact multiple of 4
 xx = (unsigned char *) malloc(rowsz);
 p = hi.x * hi.y;
 if (!CreateRaw(hi.x, hi.y)) {
 	printf("LoadBmp: cannot allocate memory for bitmap! Check file integrity!\n");
	fflush(stdout);
	free(xx);
	fclose(f);
	return false;
 	}
#ifdef DEBUG
 printf("LoadBmp: Loading \"%s\":%dx%dx%dbit BMP...", fn, hi.x, hi.y, hi.bitsperpixel); fflush(stdout);
#endif
 int *adata = (int*)data;
 for (j=hi.y-1;j>=0;j--) {// bitmaps are saved in inverted y
	if (!fread(xx, 1, rowsz, f)) {
		printf("LoadBmp: short read while opening `%s', file is probably incomplete!\n", fn);
		free(xx);
		fclose(f);
		return 0;
		}
	p -= hi.x;
	for (i=0;i<hi.x;i++){ // actually read the pixels
		if (hi.bitsperpixel>8)
			adata[p++] = xx[i*k] + ((int) xx[i*k+1])*256 + ((int) xx[i*k+2])*65536;
			else
			adata[p++] = pal[xx[i*k]];
		}
	p -= hi.x;
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
 * Function - Writes a .bmp file from *
 *            a RawImg                *
 *------------------------------------*
 * fn - The path to the file.         *
 * *a - The source RawImg structure   *
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
RawImg:: RawImg(const char*fn)
{
	default_init();
	int i = strlen(fn)-1;
	while (i>0 && fn[i]!='.') i--;
	const char *extptr = fn + i + 1;
	if (!strcasecmp("BMP", extptr)) {
		LoadBmp(fn);
		return;
	}
	printf("Unknown file type: .%s\n", extptr);
	CreateRaw(64,64);
}

void RawImg:: RenderHack()
{
 int * adata = (int*) data;
 adata[x*y]=adata[x*y-1];
 // this is the `awesome' hack needed by render_floor
 // consult Shrink() method for more info
}

static inline float average(unsigned x)
{
	return 0.33333333 * ((x&0xff) + ((x>>8)&0xff) + ((x>>16)&0xff));
}

typedef struct {
	Uint32 x_size, y_size;
	char __padding[24];
}FDAHeaDeR;

void RawImg::SaveFda(const char *fn)
{
	FDAHeaDeR FDA;

	memset(&FDA, 0, sizeof(FDA));
	FDA.x_size = x;
	FDA.y_size = y;

	FILE *f = fopen(fn, "wb");
	if (!f) return;
	fwrite(&FDA, sizeof(FDA), 1, f);
	fwrite(data, sizeof(float), x*y, f);
	fclose(f);
}

int main(int argc, char**argv)
{
	if (argc != 3) {
		printf("Usage: bmp2fda <bmp_file> <fda_file>\n");
		return -1;
	}
	RawImg in(argv[1]);
	RawImg oo(in.get_x(), in.get_y());
	unsigned *Pin = (unsigned *) in.get_data();
	float    *Pou = (float    *) oo.get_data();
	for (int i = 0; i < in.get_size(); i++) {
		Pou[i] = average(Pin[i]);
	}
	oo.SaveFda(argv[2]);
	return 0;
}
