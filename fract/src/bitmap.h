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

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdio.h>
#include "MyTypes.h"

// the 'BM' magic in the beginning of a .BMP phile
#define BM_MAGIC 19778

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
	
	/// frees the memory, allocated by the bitmap
	void close(void);

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

	/// loads floating-point data from a .FDA file
	bool LoadFda(const char * fn);

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

	/// fills the bitmap with a constant color
	void fill(unsigned color);
	
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
}
#ifdef __GNUC__
__attribute__((packed))
#endif
BMiHEADER;

typedef struct {
	Uint32 x_size, y_size;
	char __padding[24];
}FDAHeaDeR;

void gridify_texture(RawImg & a);
// make a checker texture, with color1 and color2 alternating
void checker_texture(RawImg & a, int size = 8, unsigned color1=0x0, unsigned color2=0xffffff);

void Take_Snapshot(RawImg & a);
void bitmap_close(void);

Uint32 byteswap_32(Uint32);
Uint16 byteswap_16(Uint16);


#if __BYTE_ORDER == __LITTLE_ENDIAN
#	define itohl(x) (x)
#	define itohs(x) (x)
#	define htoil(x) (x)
#	define htois(x) (x)
#else
#	define itohl(x) byteswap_32(x)
#	define itohs(x) byteswap_16(x)
#	define htoil(x) byteswap_32(x)
#	define htois(x) byteswap_16(x)
#endif


#endif //__BITMAP_H__
