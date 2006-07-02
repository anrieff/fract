/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *                                                                         *
 * This file contains every single assembly line used in the project       *
 * As it name suggests, (almost) only x86 assembly code is in here         *
 *                                                                         *
 ***************************************************************************/

//#ifndef __X86_ASM_H__
//#define __X86_ASM_H__

#ifdef USE_ASSEMBLY
#ifdef __GNUC__

// Mac OSX's assembler doesn't like inline aligment requests
#ifndef __APPLE__
#	define XALIGN ".balign 16\n"
#else
#	define XALIGN
#endif

// generic assembly snipplet, emits the EMMS instruction
#ifdef emit_emms
		__asm __volatile ("emms");
#endif

// originaly taken from render.cpp:739:
#ifdef render1
			__asm __volatile (
			"	movups	%0,	%%xmm0\n"
			"	movups	%1,	%%xmm1\n"
			"	movups	%2,	%%xmm2\n"
			"	movups	%3,	%%xmm3\n"
			"	movups	%6,	%%xmm7\n"

			"	movq	%4,	%%mm0\n"
			"	movq	8%4,	%%mm1\n"
			"	movq	%5,	%%mm2\n"
			"	movq	8%5,	%%mm3\n"
			"	pxor	%%mm6,	%%mm6\n"
			"	movd	%7,	%%mm6\n"
			:
			:"m"(*fx),"m"(*fy),"m"(*fxi),"m"(*fyi),"m"(*gx),"m"(*gy),"m"(*c64k), "m"(xtex)
			:"memory"
			);
#endif
// originaly taken from render.cpp:758:
#ifdef render2
					__asm __volatile (
				// calculate light degradation:
				    "   push	%%esi\n"
					"	movaps	%5,	%%xmm4\n" // load light x in xmm4
					"	movaps	0x10%5,	%%xmm5\n" // load light y in xmm5
					"	movaps	%%xmm7,	%%xmm6\n"
					"	subps	%%xmm0,	%%xmm4\n" // xmm4 = [ -current x + lightx ]   x 4
					"	subps	%%xmm1,	%%xmm5\n" // xmm5 = [ -current y + lighty ]   x 4
					"	mulps	%%xmm4,	%%xmm4\n" // xmm4 = [(-current x + lightx)^2] x 4
					"	mulps	%%xmm5,	%%xmm5\n" // xmm5 = [(-current y + lighty)^2] x 4
					"	addps	0x20%5,	%%xmm4\n" // xmm4 = xmm4 + (1, 1, 1, 1)
					"	addps	0x30%5,	%%xmm5\n" // xmm5 = xmm5 + [ysqrd] x 4
					"	addps	%%xmm5,	%%xmm4\n" // xmm4 = xmm4 + xmm5 = dist^2

					"	rsqrtps	%%xmm4,	%%xmm4\n" // xmm4 = 1 / dist
					"	mulps	%%xmm4,	%%xmm6\n" // xmm6 = const / dist
					"	rsqrtps %%xmm4, %%xmm4\n" // xmm4 = sqrt(dist)
					"	mulps	%%xmm4,	%%xmm6\n" // xmm6 = const / sqrt(dist)

				// save coordinates from mmx registers to memory:
					"	movq	%%mm0,	%1\n"
					"	movq	%%mm1,	8%1\n"
					"	movq	%%mm2,	%2\n"
					"	movq	%%mm3,	8%2\n"

				// shift all coords xshl positions, arithmetical
					"	psrad	%%mm6,	%%mm0\n"
					"	psrad	%%mm6,	%%mm1\n"
					"	psrad	%%mm6,	%%mm2\n"
					"	psrad	%%mm6,	%%mm3\n"
					"	movq	%%mm0,	%%mm4\n"
					"	movq	%%mm2,	%%mm5\n"
				// save 3rd and 4th x and y coordinate to work on later...
					"	movq	%%mm1,	%3\n"
					"	movq	%%mm3,	8%3\n"
				// shift the first copy additional 16 bits, arithmetical
					"	psrad	$16,	%%mm0\n"
					"	psrad	$16,	%%mm2\n"

	/* situation here:                                          ************
		mm0 = gX1&gX2 >> (16 + xshl) = Bx                 **    ****    **
		mm1 = unused & saved                             *     *  **      *
		mm2 = gY1&gY2 >> (16 + xshl) = By               *     *   **       *
		mm3 = unused & saved                            *         **       *
		mm4 = gX1&gX2 >> (xshl)                         *         **       *
		mm5 = gY1&gY2 >> (xshl)                         *         **       *
		mm6 = xshl                                       *        **      *
		mm7 = xxandmask                                   **   *******  **
                                                                    ************
	*/

					"	pslld	$16,	%%mm4\n"
					"	pslld	$16,	%%mm5\n"
					"	psrld	$17,	%%mm4\n" //mm4 &= 0xffff = FX
					"	psrld	$17,	%%mm5\n" //mm5 &= 0xffff = FY
					"	packssdw %%mm5,	%%mm4\n" //mm4 = (lo to hi) -> [ fx0 fx1 fy0 fy1 ]
					"	psllw	$1,	%%mm4\n"

					"	pand	%8,	%%mm2\n"
					"	pand	%8,	%%mm0\n" // mm0 &= xandmask
					SHIFT_LEFT_LOG2_OF_MAX_TEXTURE_SIZE_mm2
					"	psrld	%%mm6,	%%mm2\n" // mm2 = (mm2 & xandmask) << xshl
					"	paddd	%%mm2,	%%mm0\n" // mm0 = boffset[2]

				//	move the four points for the second coord in mm2 and mm3
					"	movd	%%mm0,	%%eax\n"
					"	psrlq	$32,	%%mm0\n"
					"	movd	%%mm0,	%%edx\n"
					"	movl	%7,	%%ecx\n"
					"	movl	%4,	%%esi\n"
					"	movq	(%%esi, %%eax, 4),	%%mm2\n"

					"	addl	%8,	%%eax\n"
					"	inc	%%eax\n"
					"	andl	%%ecx,	%%eax\n"
					"	movq	(%%esi, %%eax, 4),	%%mm3\n" //!!

/* situation: (low-to-hi)
	mm2 -> [ boffset		, boffset+1 		] of first coord
	mm3 -> [ boffset+xandmask+1	, boffset+xandmask+2 	] of first coord
{
	mm2(1)*=(       fx)*(0xffff-fy)
	mm2(0)*=(0xffff-fx)*(0xffff-fy)
	mm3(1)*=(       fx)*(       fy)
	mm3(0)*=(0xffff-fx)*(       fy)
}
*/

					"	pxor	%%mm7,	%%mm7\n" // nullify mm7
					"	movq	%9,	%%mm0\n" // mm0 = 0xffffffffffffffff
					"	movq	%9,	%%mm1\n" // mm1 = mm0
					"	pshufw	$0xaa, %%mm4, %%mm5\n" //extract the third word to mm5
					"	psubw	%%mm5,	%%mm0\n"
					"	pshufw	$0, %%mm4, %%mm5\n" //extract the first word to all in mm5
					"	psubw	%%mm5,	%%mm1\n"
					"	pmulhuw	%%mm0,	%%mm5\n"
					"	pmulhuw	%%mm1,	%%mm0\n"
					"	movq	%%mm2,	%%mm1\n" // save mm2 to mm1
					"	punpcklbw %%mm7,%%mm2\n"
					"	psrlq	$32,	%%mm1\n"
					"	punpcklbw %%mm7,%%mm1\n" // boffset in mm1, boffset+1 in mm2

					"	pmulhuw %%mm5,	%%mm1\n"
					"	pmulhuw %%mm0,	%%mm2\n"
					"	paddusw	%%mm1,	%%mm2\n"

					"	movq	%9,	%%mm0\n"
					"	pshufw	$0xaa, %%mm4,	%%mm5\n" // extract y0 to mm5
					"	pshufw	$0, %%mm4, %%mm1\n" // extract x0 to mm5
					"	psubw	%%mm1,	%%mm0\n"
					"	pmulhuw	%%mm5,	%%mm0\n"
					"	pmulhuw	%%mm1,	%%mm5\n"
					"	movq	%%mm3,	%%mm1\n"
					"	punpcklbw %%mm7, %%mm3\n"
					"	psrlq	$32,	%%mm1\n"
					"	punpcklbw %%mm7, %%mm1\n"

					"	pmulhuw	%%mm5,	%%mm1\n"
					"	pmulhuw	%%mm0,	%%mm3\n"
					"	paddusw	%%mm1,	%%mm2\n"
					"	paddusw	%%mm3,	%%mm2\n"
				// first point is done. Color in word fmt is in mm2.
					"	movq	%%mm2,	%%mm5\n"
					"	punpcklwd %%mm7,	%%mm2\n" // unpack words to mm2 (low)
					"	punpckhwd %%mm7,	%%mm5\n" // and mm5 (high)
					"	cvtpi2ps %%mm2,	%%xmm4\n"	 // convert mm2 to xmm4
					"	cvtpi2ps %%mm5,	%%xmm5\n"	 // convert mm5 to xmm5
					"	movlhps	%%xmm5,	%%xmm4\n"	 // get in xmm4 all components of color
					"	movaps	%%xmm6,	%%xmm5\n"
					"	shufps	$0, %%xmm6, %%xmm5\n"	// fill all xmm5 with xmm6[0]
					"	mulps	%%xmm5,	%%xmm4\n"	// multiply xmm4
				// repack to RGB entity using MMX:
					"	cvtps2pi %%xmm4,	%%mm2\n"
					"	movhlps	%%xmm4,	%%xmm5\n"
					"	cvtps2pi %%xmm5,	%%mm5\n"
					"	packssdw %%mm5,	%%mm2\n"
					"	packuswb %%mm7,	%%mm2\n"
					"	movd	%%mm2,	%0\n"
/*
						    ************	.    ************
						  **            **	.  **            **
						 *    *****       *	. *    *****       *
						*    *     *       *	.*    *     *       *
						*          *       *	.*          *       *
						*         *        *	.*         *        *
						*       **         *	.*       **         *
						 *    **     *    *	. *    **     *    *
						  **  ********  **	.  **  ********  **
						    ************	.    ************
	*/


// next point baddress is in edx... repeat the whole procedure...
					"	movl	%%edx,	%%eax\n"
					"	movq	(%%esi, %%eax, 4),	%%mm2\n"

					"	addl	%8,	%%eax\n"
					"	inc	%%eax\n"
					"	andl	%%ecx,	%%eax\n"
					"	movq	(%%esi, %%eax, 4),	%%mm3\n"

					"	movq	%9,	%%mm0\n" // mm0 = 0xffffffffffffffff
					"	movq	%9,	%%mm1\n" // mm1 = mm0
					"	pshufw	$0xff, %%mm4, %%mm5\n" //extract the third word to mm5
					"	psubw	%%mm5,	%%mm0\n"
					"	pshufw	$0x55, %%mm4, %%mm5\n" //extract the first word to all in mm5
					"	psubw	%%mm5,	%%mm1\n"
					"	pmulhuw	%%mm0,	%%mm5\n"
					"	pmulhuw	%%mm1,	%%mm0\n"
					"	movq	%%mm2,	%%mm1\n" // save mm2 to mm1
					"	punpcklbw %%mm7,%%mm2\n"
					"	psrlq	$32,	%%mm1\n"
					"	punpcklbw %%mm7,%%mm1\n" // boffset in mm1, boffset+1 in mm2

					"	pmulhuw %%mm5,	%%mm1\n"
					"	pmulhuw %%mm0,	%%mm2\n"
					"	paddusw	%%mm1,	%%mm2\n"

					"	movq	%9,	%%mm0\n"
					"	pshufw	$0xff, %%mm4,	%%mm5\n" // extract y0 to mm5
					"	pshufw	$0x55, %%mm4, %%mm1\n" // extract x0 to mm5
					"	psubw	%%mm1,	%%mm0\n"
					"	pmulhuw	%%mm5,	%%mm0\n"
					"	pmulhuw	%%mm1,	%%mm5\n"
					"	movq	%%mm3,	%%mm1\n"
					"	punpcklbw %%mm7, %%mm3\n"
					"	psrlq	$32,	%%mm1\n"
					"	punpcklbw %%mm7, %%mm1\n"

					"	pmulhuw	%%mm5,	%%mm1\n"
					"	pmulhuw	%%mm0,	%%mm3\n"
					"	paddusw	%%mm1,	%%mm2\n"
					"	paddusw	%%mm3,	%%mm2\n"
				// second point is done. Color in word fmt is in mm2.
					"	movq	%%mm2,	%%mm5\n"
					"	punpcklwd %%mm7,	%%mm2\n" // unpack words to mm2 (low)
					"	punpckhwd %%mm7,	%%mm5\n" // and mm5 (high)
					"	cvtpi2ps %%mm2,	%%xmm4\n"	 // convert mm2 to xmm4
					"	cvtpi2ps %%mm5,	%%xmm5\n"	 // convert mm5 to xmm5
					"	movlhps	%%xmm5,	%%xmm4\n"	 // get in xmm4 all components of color
					"	movaps	%%xmm6,	%%xmm5\n"
					"	shufps	$0x55,%%xmm6, %%xmm5\n"	// fill all xmm5 with xmm6[1]
					"	mulps	%%xmm5,	%%xmm4\n"	// multiply xmm4
				// repack to RGB entity using MMX:
					"	cvtps2pi %%xmm4,	%%mm2\n"
					"	movhlps	%%xmm4,	%%xmm5\n"
					"	cvtps2pi %%xmm5,	%%mm5\n"
					"	packssdw %%mm5,	%%mm2\n"
					"	packuswb %%mm7,	%%mm2\n"
					"	movd	%%mm2,	4%0\n"

/*
	    ************	    ************	    ************
	  **   ****     **	  **   ****     **	  **   ****     **
	 *    *    *      *	 *    *    *      *	 *    *    *      *
	*          *       *	*          *       *	*          *       *
	*       ***        *	*       ***        *	*       ***        *
	*          *       *	*          *       *	*          *       *
	*    *     *       *	*    *     *       *	*    *     *       *
	 *    *    *      *	 *    *    *      *	 *    *    *      *
	  **   ****     **	  **   ****     **	  **   ****     **
	    ************	    ************	    ************
	*/

// third coord:
				// restore 3rd and 4th x and y coordinate...
					"	movq	%3,	%%mm0\n"
					"	movq	8%3,	%%mm2\n"
					"	movq	%%mm0,	%%mm4\n"
					"	movq	%%mm2,	%%mm5\n"
				// shift the first copy additional 16 bits, arithmetical
					"	psrad	$16,	%%mm0\n"
					"	psrad	$16,	%%mm2\n"

					"	pslld	$16,	%%mm4\n"
					"	pslld	$16,	%%mm5\n"
					"	psrld	$17,	%%mm4\n" //mm4 &= 0xffff = FX
					"	psrld	$17,	%%mm5\n" //mm5 &= 0xffff = FY
					"	packssdw %%mm5,	%%mm4\n" //mm4 = (lo to hi) -> [ fx0 fx1 fy0 fy1 ]
					"	psllw	$1,	%%mm4\n"

					"	pand	%8,	%%mm2\n"
					"	pand	%8,	%%mm0\n" // mm0 &= xandmask
					SHIFT_LEFT_LOG2_OF_MAX_TEXTURE_SIZE_mm2
					"	psrld	%%mm6,	%%mm2\n" // mm2 = (mm2 & xandmask) << xshl
					" 	paddd	%%mm2,	%%mm0\n" // mm0 = boffset[2]

				//	move the four points for the second coord in mm2 and mm3
					"	movd	%%mm0,	%%eax\n"
					"	psrlq	$32,	%%mm0\n"
					"	movd	%%mm0,	%%edx\n"
					"	movl	%7,	%%ecx\n"
					"	movl	%4,	%%esi\n"
					"	movq	(%%esi, %%eax, 4),	%%mm2\n"

					"	addl	%8,	%%eax\n"
					"	inc	%%eax\n"
					"	andl	%%ecx,	%%eax\n"
					"	movq	(%%esi, %%eax, 4),	%%mm3\n"

					"	movq	%9,	%%mm0\n" // mm0 = 0xffffffffffffffff
					"	movq	%9,	%%mm1\n" // mm1 = mm0
					"	pshufw	$0xaa, %%mm4, %%mm5\n" //extract the third word to mm5
					"	psubw	%%mm5,	%%mm0\n"
					"	pshufw	$0, %%mm4, %%mm5\n" //extract the first word to all in mm5
					"	psubw	%%mm5,	%%mm1\n"
					"	pmulhuw	%%mm0,	%%mm5\n"
					"	pmulhuw	%%mm1,	%%mm0\n"
					"	movq	%%mm2,	%%mm1\n" // save mm2 to mm1
					"	punpcklbw %%mm7,%%mm2\n"
					"	psrlq	$32,	%%mm1\n"
					"	punpcklbw %%mm7,%%mm1\n" // boffset in mm1, boffset+1 in mm2

					"	pmulhuw %%mm5,	%%mm1\n"
					"	pmulhuw %%mm0,	%%mm2\n"
					"	paddusw	%%mm1,	%%mm2\n"

					"	movq	%9,	%%mm0\n"
					"	pshufw	$0xaa, %%mm4,	%%mm5\n" // extract y0 to mm5
					"	pshufw	$0, %%mm4, %%mm1\n" // extract x0 to mm5
					"	psubw	%%mm1,	%%mm0\n"
					"	pmulhuw	%%mm5,	%%mm0\n"
					"	pmulhuw	%%mm1,	%%mm5\n"
					"	movq	%%mm3,	%%mm1\n"
					"	punpcklbw %%mm7, %%mm3\n"
					"	psrlq	$32,	%%mm1\n"
					"	punpcklbw %%mm7, %%mm1\n"

					"	pmulhuw	%%mm5,	%%mm1\n"
					"	pmulhuw	%%mm0,	%%mm3\n"
					"	paddusw	%%mm1,	%%mm2\n"
					"	paddusw	%%mm3,	%%mm2\n"
				// third point is done. Color in word fmt is in mm2.
					"	movq	%%mm2,	%%mm5\n"
					"	punpcklwd %%mm7,	%%mm2\n" // unpack words to mm2 (low)
					"	punpckhwd %%mm7,	%%mm5\n" // and mm5 (high)
					"	cvtpi2ps %%mm2,	%%xmm4\n"	 // convert mm2 to xmm4
					"	cvtpi2ps %%mm5,	%%xmm5\n"	 // convert mm5 to xmm5
					"	movlhps	%%xmm5,	%%xmm4\n"	 // get in xmm4 all components of color
					"	movaps	%%xmm6,	%%xmm5\n"
					"	shufps	$0xaa,%%xmm6, %%xmm5\n"	// fill all xmm5 with xmm6[2]
					"	mulps	%%xmm5,	%%xmm4\n"	// multiply xmm4
				// repack to RGB entity using MMX:
					"	cvtps2pi %%xmm4,	%%mm2\n"
					"	movhlps	%%xmm4,	%%xmm5\n"
					"	cvtps2pi %%xmm5,	%%mm5\n"
					"	packssdw %%mm5,	%%mm2\n"
					"	packuswb %%mm7,	%%mm2\n"
					"	movd	%%mm2,	8%0\n"

/*
							    ************
							  **            **
							 *   ***  **      *
							*     *   *        *
							*     *   *        *
							*     *****        *
							*         *        *
							 *        *       *
							  **      **    **
							    ************
******************************************************************************
********************************        **************************************
***************************                  *********************************
********                                                          ************
********                  *****   ****   ****                     ************
********                    *       *     *                       ************
********                    *        *   *                        ************
********                    *         * *                         ************
********                  *****        *                          ************
********                                                          ************
********************************        **************************************
******************************************************************************
	*/
// FOURTH point!
// next point baddress is in edx... repeat the whole procedure...
					"	movl	%%edx,	%%eax\n"
					"	movq	(%%esi, %%eax, 4),	%%mm2\n"

					"	addl	%8,	%%eax\n"
					"	inc	%%eax\n"
					"	andl	%%ecx,	%%eax\n"
					"	movq	(%%esi, %%eax, 4),	%%mm3\n"

					"	movq	%9,	%%mm0\n" // mm0 = 0xffffffffffffffff
					"	movq	%9,	%%mm1\n" // mm1 = mm0
					"	pshufw	$0xff, %%mm4, %%mm5\n" //extract the third word to mm5
					"	psubw	%%mm5,	%%mm0\n"
					"	pshufw	$0x55, %%mm4, %%mm5\n" //extract the first word to all in mm5
					"	psubw	%%mm5,	%%mm1\n"
					"	pmulhuw	%%mm0,	%%mm5\n"
					"	pmulhuw	%%mm1,	%%mm0\n"
					"	movq	%%mm2,	%%mm1\n" // save mm2 to mm1
					"	punpcklbw %%mm7,%%mm2\n"
					"	psrlq	$32,	%%mm1\n"
					"	punpcklbw %%mm7,%%mm1\n" // boffset in mm1, boffset+1 in mm2

					"	pmulhuw %%mm5,	%%mm1\n"
					"	pmulhuw %%mm0,	%%mm2\n"
					"	paddusw	%%mm1,	%%mm2\n"

					"	movq	%9,	%%mm0\n"
					"	pshufw	$0xff, %%mm4,	%%mm5\n" // extract y0 to mm5
					"	pshufw	$0x55, %%mm4, %%mm1\n" // extract x0 to mm5
					"	psubw	%%mm1,	%%mm0\n"
					"	pmulhuw	%%mm5,	%%mm0\n"
					"	pmulhuw	%%mm1,	%%mm5\n"
					"	movq	%%mm3,	%%mm1\n"
					"	punpcklbw %%mm7, %%mm3\n"
					"	psrlq	$32,	%%mm1\n"
					"	punpcklbw %%mm7, %%mm1\n"

					"	pmulhuw	%%mm5,	%%mm1\n"
					"	pmulhuw	%%mm0,	%%mm3\n"
					"	paddusw	%%mm1,	%%mm2\n"
					"	paddusw	%%mm3,	%%mm2\n"
				// last point is done. Color in word fmt is in mm2.
					"	movq	%%mm2,	%%mm5\n"
					"	punpcklwd %%mm7,	%%mm2\n" // unpack words to mm2 (low)
					"	punpckhwd %%mm7,	%%mm5\n" // and mm5 (high)
					"	cvtpi2ps %%mm2,	%%xmm4\n"	 // convert mm2 to xmm4
					"	cvtpi2ps %%mm5,	%%xmm5\n"	 // convert mm5 to xmm5
					"	movlhps	%%xmm5,	%%xmm4\n"	 // get in xmm4 all components of color
					"	movaps	%%xmm6,	%%xmm5\n"
					"	shufps	$0xff,%%xmm6, %%xmm5\n"	// fill all xmm5 with xmm6[3]
					"	mulps	%%xmm5,	%%xmm4\n"	// multiply xmm4
				// repack to RGB entity using MMX:
					"	cvtps2pi %%xmm4,	%%mm2\n"
					"	movhlps	%%xmm4,	%%xmm5\n"
					"	cvtps2pi %%xmm5,	%%mm5\n"
					"	packssdw %%mm5,	%%mm2\n"
					"	packuswb %%mm7,	%%mm2\n"
					"	movd	%%mm2,	12%0\n"

				// get integer coordinates from memory:
					"	movq	%1,	%%mm0\n"
					"	movq	8%1,	%%mm1\n"
					"	movq	%2,	%%mm2\n"
					"	movq	8%2,	%%mm3\n"

				// continue interpolation:
					"	paddd	%6,	%%mm0\n"
					"	paddd	8%6,	%%mm1\n"
					"	paddd	16%6,	%%mm2\n"
					"	paddd	24%6,	%%mm3\n"
					"	addps	%%xmm2,	%%xmm0\n"
					"	addps	%%xmm3,	%%xmm1\n"
					"   pop		%%esi\n"
					:"=m"(*dptr), "=m"(*gx), "=m"(*gy), "=m"(*save)
					:"m"(ptex),"m"(*lightxy1),"m"(*gxx), "m"(texandmask),"m"(*xxandmask),
					 "m"(*my_ffs)
					:"memory", "eax", "ecx", "edx"
					);
#endif
// originaly taken from 1174
#ifdef render3
				__asm __volatile (
				// calculate light degradation:
				    "	push	%%esi\n"
					"	movaps	%5,	%%xmm4\n" // load light x in xmm4
					"	movaps	0x10%5,	%%xmm5\n" // load light y in xmm5
					"	movaps	%%xmm7,	%%xmm6\n"
					"	subps	%%xmm0,	%%xmm4\n" // xmm4 = [ -current x + lightx ]   x 4
					"	subps	%%xmm1,	%%xmm5\n" // xmm5 = [ -current y + lighty ]   x 4
					"	mulps	%%xmm4,	%%xmm4\n" // xmm4 = [(-current x + lightx)^2] x 4
					"	mulps	%%xmm5,	%%xmm5\n" // xmm5 = [(-current y + lighty)^2] x 4
					"	addps	0x20%5,	%%xmm4\n" // xmm4 = xmm4 + (1, 1, 1, 1)
					"	addps	0x30%5,	%%xmm5\n" // xmm5 = xmm5 + [ysqrd] x 4
					"	addps	%%xmm5,	%%xmm4\n" // xmm4 = xmm4 + xmm5 = dist^2

					"	rsqrtps	%%xmm4,	%%xmm4\n" // xmm4 = 1 / dist
					"	mulps	%%xmm4,	%%xmm6\n" // xmm6 = const / dist
					"	rsqrtps %%xmm4, %%xmm4\n" // xmm4 = sqrt(dist)
					"	mulps	%%xmm4,	%%xmm6\n" // xmm6 = const / sqrt(dist)

				// save coordinates from mmx registers to memory:
					"	movq	%%mm0,	%1\n"
					"	movq	%%mm1,	8%1\n"
					"	movq	%%mm2,	%2\n"
					"	movq	%%mm3,	8%2\n"

				// shift all coords 16 positions, arithmetical
					"	psrad	$16,	%%mm0\n"
					"	psrad	$16,	%%mm1\n"
					"	psrad	$16,	%%mm2\n"
					"	psrad	$16,	%%mm3\n"
					"	movq	%7,	%%mm7\n"
				// then shift xtex positions, logical
					"	psrld	%%mm6,	%%mm0\n"
					"	psrld	%%mm6,	%%mm1\n"
					"	psrld	%%mm6,	%%mm2\n"
					"	psrld	%%mm6,	%%mm3\n"

					"	pand	%%mm7,	%%mm0\n" // mm0 &= xandmask; (mm0 -> x0 x1)
					"	pand	%%mm7,	%%mm1\n" // mm1 &= xandmask; (mm1 -> x2 x3)
					"	pand	%%mm7,	%%mm2\n" // mm2 &= xandmask; (mm2 -> y0 y1)
					"	pand	%%mm7,	%%mm3\n" // mm3 &= xandmask; (mm3 -> y2 y3)

					SHIFT_LEFT_LOG2_OF_MAX_TEXTURE_SIZE_mm2
					SHIFT_LEFT_LOG2_OF_MAX_TEXTURE_SIZE_mm3
					"	psrld	%%mm6,	%%mm2\n" // mm2 <<= xshl
					"	psrld	%%mm6,	%%mm3\n" // mm3 <<= xshl

					"	paddd	%%mm2,	%%mm0\n" // mm0 = index0 index1
					"	paddd	%%mm3,	%%mm1\n" // mm1 = index2 index3

					"	pxor	%%mm7,	%%mm7\n" // nullify mm7
					"	movl	%4,	%%esi\n"

					"	movd	%%mm0,	%%eax\n"
					"	psrlq	$32,	%%mm0\n"
					"	movd	%%mm0,	%%edx\n"

					"	movd	(%%esi, %%eax, 4),	%%mm2\n" // get first pixel in mm2
					"	movd	(%%esi,	%%edx, 4),	%%mm3\n" // get second pixel in mm3

					"	punpcklbw %%mm7,	%%mm2\n"
					"	punpcklbw %%mm7,	%%mm3\n"

					"	movq	%%mm2,	%%mm4\n"
					"	movq	%%mm3,	%%mm5\n"

					"	punpcklwd %%mm7,	%%mm2\n"
					"	punpcklwd %%mm7,	%%mm3\n"
					"	punpckhwd %%mm7,	%%mm4\n"
					"	punpckhwd %%mm7,	%%mm5\n"

					"	cvtpi2ps %%mm2,	%%xmm2\n"
					"	cvtpi2ps %%mm3,	%%xmm3\n"
					"	cvtpi2ps %%mm4,	%%xmm4\n"
					"	cvtpi2ps %%mm5,	%%xmm5\n"
					"	movlhps %%xmm4,	%%xmm2\n"
					"	movlhps %%xmm5,	%%xmm3\n"
					"	movaps	%%xmm6,	%%xmm4\n"
					"	movaps	%%xmm6,	%%xmm5\n"
					"	shufps $0, %%xmm6, %%xmm4\n"
					"	shufps $0x55, %%xmm6, %%xmm5\n"
					"	mulps	%%xmm4,	%%xmm2\n"
					"	mulps	%%xmm5,	%%xmm3\n"
					"	movhlps	%%xmm2,	%%xmm4\n"
					"	movhlps	%%xmm3,	%%xmm5\n"
					"	cvtps2pi %%xmm2, %%mm2\n"
					"	cvtps2pi %%xmm3, %%mm3\n"
					"	cvtps2pi %%xmm4, %%mm4\n"
					"	cvtps2pi %%xmm5, %%mm5\n"
					"	packssdw %%mm4,	%%mm2\n"
					"	packssdw %%mm5,	%%mm3\n"
					"	packuswb %%mm7,	%%mm2\n"
					"	packuswb %%mm7,	%%mm3\n"
					// points 0 and 1 are ready, push to framebuffer:
					"	movd	%%mm2,	%0\n"
					"	movd	%%mm3,	4%0\n"
// proceed with points 2 and 3...
					"	movd	%%mm1,	%%eax\n"
					"	psrlq	$32,	%%mm1\n"
					"	movd	%%mm1,	%%edx\n"

					"	movd	(%%esi, %%eax, 4),	%%mm2\n" // get first pixel in mm2
					"	movd	(%%esi,	%%edx, 4),	%%mm3\n" // get second pixel in mm3

					"	punpcklbw %%mm7,	%%mm2\n"
					"	punpcklbw %%mm7,	%%mm3\n"

					"	movq	%%mm2,	%%mm4\n"
					"	movq	%%mm3,	%%mm5\n"

					"	punpcklwd %%mm7,	%%mm2\n"
					"	punpcklwd %%mm7,	%%mm3\n"
					"	punpckhwd %%mm7,	%%mm4\n"
					"	punpckhwd %%mm7,	%%mm5\n"

					"	cvtpi2ps %%mm2,	%%xmm2\n"
					"	cvtpi2ps %%mm3,	%%xmm3\n"
					"	cvtpi2ps %%mm4,	%%xmm4\n"
					"	cvtpi2ps %%mm5,	%%xmm5\n"
					"	movlhps %%xmm4,	%%xmm2\n"
					"	movlhps %%xmm5,	%%xmm3\n"
					"	movaps	%%xmm6,	%%xmm4\n"
					"	movaps	%%xmm6,	%%xmm5\n"
					"	shufps $0xaa, %%xmm6, %%xmm4\n"
					"	shufps $0xff, %%xmm6, %%xmm5\n"
					"	mulps	%%xmm4,	%%xmm2\n"
					"	mulps	%%xmm5,	%%xmm3\n"
					"	movhlps	%%xmm2,	%%xmm4\n"
					"	movhlps	%%xmm3,	%%xmm5\n"
					"	cvtps2pi %%xmm2, %%mm2\n"
					"	cvtps2pi %%xmm3, %%mm3\n"
					"	cvtps2pi %%xmm4, %%mm4\n"
					"	cvtps2pi %%xmm5, %%mm5\n"
					"	packssdw %%mm4,	%%mm2\n"
					"	packssdw %%mm5,	%%mm3\n"
					"	packuswb %%mm7,	%%mm2\n"
					"	packuswb %%mm7,	%%mm3\n"
					// points 2 and 3 are ready, push to framebuffer:
					"	movd	%%mm2,	8%0\n"
					"	movd	%%mm3,	12%0\n"

				// get integer coordinates from memory:
					"	movq	%1,	%%mm0\n"
					"	movq	8%1,	%%mm1\n"
					"	movq	%2,	%%mm2\n"
					"	movq	8%2,	%%mm3\n"

				// continue interpolation:
					"	paddd	%6,	%%mm0\n"
					"	paddd	8%6,	%%mm1\n"
					"	paddd	16%6,	%%mm2\n"
					"	paddd	24%6,	%%mm3\n"
					"	movaps	%8,	%%xmm2\n"
					"	movaps	16%8,	%%xmm3\n"
					"	addps	%%xmm2,	%%xmm0\n"
					"	addps	%%xmm3,	%%xmm1\n"
					"	pop		%%esi\n"

					:"=m"(*dptr), "=m"(*gx), "=m"(*gy), "=m"(*save)
					:"m"(ptex),"m"(*lightxy1),"m"(*gxx),"m"(*xxandmask), "m"(*fxyi)
					:"memory", "eax", "edx");
#endif
// originaly taken from antialias.cpp:75
#ifdef antialias_asm
// MMX2 version of the antialias folding routine... does two output pixels per loop, gets 8 points and does them in
// parallel.

// it is low fidelity, because before adding the four points, two LSB of every point are lost, resulting in
// e.g., (7+7+7+7)/4 = 4...
static void antialias_4x_mmx2_lo_fi(Uint32 *fb)
{
	int xr = xres(), yr = yres();
	unsigned pp[2] = {0x3f3f3f3f, 0x3f3f3f3f};
	__asm __volatile(
	"	movl	%2,	%%edx\n"
	"	movl	%0,	%%esi\n"
	"	movl	%0,	%%edi\n"
	"	movl	%1,	%%ecx\n"
	"	shl	$3,	%%ecx\n"
	"	movq	%3,	%%mm7\n"

	"rowcycle:\n"
	"	movl	%1,	%%eax\n"
	"	shr	$1,	%%eax\n"
	"pixelcycle:\n"
	"	movq	 (%%esi),	%%mm0\n"
	"	movq	8(%%esi),	%%mm1\n"
	"	movq	 (%%esi, %%ecx, 1),%%mm2\n"
	"	movq	8(%%esi, %%ecx, 1),%%mm3\n"

	"	psrlq	$2,	%%mm0\n"
	"	psrlq	$2,	%%mm1\n"
	"	psrlq	$2,	%%mm2\n"
	"	psrlq	$2,	%%mm3\n"

	"	pand	%%mm7,	%%mm0\n"
	"	pand	%%mm7,	%%mm1\n"
	"	pand	%%mm7,	%%mm2\n"
	"	pand	%%mm7,	%%mm3\n"

	"	paddusb	%%mm2,	%%mm0\n"
	"	paddusb	%%mm3,	%%mm1\n"

	"	pshufw	$0x0e,	%%mm0,	%%mm2\n"
	"	pshufw	$0x0e,	%%mm1,	%%mm3\n"
	"	paddusb	%%mm2,	%%mm0\n"
	"	paddusb	%%mm3,	%%mm1\n"

	"	movd	%%mm0,	(%%edi)\n"
	"	movd	%%mm1,	4(%%edi)\n"

	"	add	$8,	%%edi\n"
	"	add	$16,	%%esi\n"

	"	dec	%%eax\n"
	"	jnz	pixelcycle\n"
	"	add	%%ecx,	%%esi\n"
	"	dec	%%edx\n"
	"	jnz	rowcycle\n"
	"	emms\n"
	:"=m"(fb)
	:"m"(xr),"m"(yr), "m"(*pp)
	:"memory", "eax", "edx", "esi", "edi"
	);
}

/***********************************************************
 * High quality version of the antialias folding routine   *
 * Uses MMX. Does all arithmetics in full precision	   *
 ***********************************************************/

static void antialias_4x_mmx_hi_fi(Uint32 *fb)
{
	int xr = xres(), yr = yres();
	__asm __volatile(
	"	movl	%2,	%%edx\n"
	"	movl	%0,	%%esi\n"
	"	movl	%0,	%%edi\n"
	"	movl	%1,	%%ecx\n"
	"	shl	$3,	%%ecx\n"
	"	pxor	%%mm7,	%%mm7\n"

	"rowcycle2:\n"
	"	movl	%1,	%%eax\n"
	"	shr	$1,	%%eax\n"
	"pixelcycle2:\n"
	"	movq	 (%%esi),	%%mm0\n"
	"	movq	8(%%esi),	%%mm1\n"
	"	movq	 (%%esi, %%ecx),%%mm2\n"
	"	movq	8(%%esi, %%ecx),%%mm3\n"

	"	movq	%%mm0,	%%mm4\n"
	"	movq	%%mm1,	%%mm5\n"
	"	movq	%%mm2,	%%mm6\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpckhbw	%%mm7,	%%mm4\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
	"	punpckhbw	%%mm7,	%%mm5\n"
	"	punpcklbw	%%mm7,	%%mm2\n"
	"	punpckhbw	%%mm7,	%%mm6\n"

	"	paddusw	%%mm4,	%%mm0\n"
	"	movq	%%mm3,	%%mm4\n"

	"	paddusw	%%mm5,	%%mm1\n"

	"	punpcklbw	%%mm7,	%%mm3\n"
	"	punpckhbw	%%mm7,	%%mm4\n"

	"	paddusw	%%mm6,	%%mm2\n"
	"	paddusw	%%mm4,	%%mm3\n"

	"	paddusw	%%mm2,	%%mm0\n"
	"	paddusw	%%mm3,	%%mm1\n"

	"	psrlw	$2,	%%mm0\n"
	"	psrlw	$2,	%%mm1\n"

	"	packuswb	%%mm7,	%%mm0\n"
	"	packuswb	%%mm7,	%%mm1\n"

	"	movd	%%mm0,	(%%edi)\n"
	"	movd	%%mm1,	4(%%edi)\n"

	"	add	$8,	%%edi\n"
	"	add	$16,	%%esi\n"

	"	dec	%%eax\n"
	"	jnz	pixelcycle2\n"
	"	add	%%ecx,	%%esi\n"
	"	dec	%%edx\n"
	"	jnz	rowcycle2\n"
	"	emms\n"
	:"=m"(fb)
	:"m"(xr),"m"(yr)
	:"memory", "eax", "ecx", "edx", "esi", "edi"
	);
}
#endif
// originaly taken from gfx.cpp:279:
#ifdef gfx_asm
// WITH SSE
Uint32 blend_sse(Uint32 f, Uint32 b, float ff)
{Uint32 rezult;
 __asm __volatile (
 "	pxor	%%mm3,	%%mm3	\n"
 "	movd	%1,	%%mm0	\n"
 "	movd	%2,	%%mm1	\n"
 "	punpcklbw	%%mm3,	%%mm0	\n"
 "	movq	%%mm0,	%%mm2	\n"
 "	punpcklwd	%%mm3,	%%mm0	\n"
 "	punpckhwd	%%mm3,	%%mm2	\n"

 "	cvtpi2ps	%%mm0,	%%xmm0	\n"
 "	cvtpi2ps	%%mm2,	%%xmm1	\n"
 "	movlhps	%%xmm1,	%%xmm0	\n"

 "	punpcklbw	%%mm3,	%%mm1	\n"
 "	movq	%%mm1,	%%mm2	\n"
 "	punpcklwd	%%mm3,	%%mm1	\n"
 "	punpckhwd	%%mm3,	%%mm2	\n"

 "	cvtpi2ps	%%mm1,	%%xmm1	\n"
 "	cvtpi2ps	%%mm2,	%%xmm2	\n"
 "	movlhps	%%xmm2,	%%xmm1	\n"

 "	movss	%3,	%%xmm2	\n"
 "	shufps	$0,	%%xmm2,	%%xmm2	\n"

 "	movaps	%4,	%%xmm3	\n"
 "	subps	%%xmm2,	%%xmm3	\n"
 "	mulps	%%xmm2,	%%xmm0	\n"
 "	mulps	%%xmm3,	%%xmm1	\n"
 "	addps	%%xmm1,	%%xmm0	\n"

 "	movhlps	%%xmm0,	%%xmm1	\n"
 "	cvtps2pi	%%xmm0,	%%mm0	\n"
 "	cvtps2pi	%%xmm1,	%%mm1	\n"

 "	packssdw	%%mm1,	%%mm0	\n"
 "	packuswb	%%mm1,	%%mm0	\n"
 "	movd	%%mm0,	%0	\n"
 "	emms	\n"

 :"=m"(rezult)
 :"m"(f), "m"(b), "m"(ff), "m"(*w2_flones)
 :"memory"
 );
 return rezult;
}

Uint32 blend_sse0(Uint32 f, Uint32 b, float ff)
{
	Uint32 result;
	Uint32 mul = (int)(ff * 65535.0f);
	__asm __volatile (

	"	pxor	%%mm7,	%%mm7\n"
	"	movd	%1,	%%mm0\n"
	"	movd	%2,	%%mm1\n"
	"	movd	%3,	%%mm5\n"
	"	movq	%4,	%%mm6\n"

	"	punpcklbw %%mm7,	%%mm0\n"
	"	punpcklbw %%mm7,	%%mm1\n"
	"	pshufw	$0, %%mm5,	%%mm5\n"
	"	psubw	%%mm5,	%%mm6\n"
	"	pmulhuw %%mm5,	%%mm0\n"
	"	pmulhuw %%mm6,	%%mm1\n"
	"	paddusw	%%mm1,	%%mm0\n"

	"	packuswb %%mm7,	%%mm0\n"

	"	movd	%%mm0,	%0\n"
	"	emms\n"

	:"=m"(result)
	:"m"(f), "m"(b), "m"(mul), "m"(*ii_ones)
	:"memory"
	);
	return result;
}

#endif
//originaly taken from profile.cpp:25:
#ifdef profile_asm
long long prof_rdtsc(void)
{
	long long r;
	__asm __volatile(
		"	rdtsc				\n"
		"	movl	%%eax,	%0		\n"
		"	movl	%%edx,	4%0		\n"
		:"=m"(r)
		::"memory", "eax", "edx"
	);
	return r;
}
#endif
//originaly taken from shaders.cpp:267:
#ifdef shaders_asm
/* here comes a quick list of all functions with the needed stuff to tell gcc not to inline them */

void convolve_mmx_w_shifts_generic(Uint32 *src, Uint32 *dest, int resx, int resy,
					ConvolveMatrix *M, int shift) __attribute__ ((noinline));
void convolve_mmx_w_shifts_3(Uint32 *src, Uint32 *dest, int resx, int resy,
					ConvolveMatrix *M, int shift) __attribute__ ((noinline));
void convolve_mmx_w_shifts_5(Uint32 *src, Uint32 *dest, int resx, int resy,
					ConvolveMatrix *M, int shift) __attribute__ ((noinline));
void fft_1D_complex_sse(int dir, int m, float *x, float *y) __attribute__ ((noinline));

void shader_gamma_shl(Uint32 *src, Uint32 *dest, int resx, int resy, int shift) __attribute__ ((noinline));
void shader_gamma_shr(Uint32 *src, Uint32 *dest, int resx, int resy, int shift) __attribute__ ((noinline));
void float_copy_ij_i(float *a, float *b, complex c[], int fft_size) __attribute__ ((noinline));
void float_copy_i_ij(float *a, float *b, complex c[], int fft_size) __attribute__ ((noinline));
void float_copy_ij_j(float *a, float *b, complex c[], int fft_size) __attribute__ ((noinline));
void float_copy_j_ij(float *a, float *b, complex c[], int fft_size) __attribute__ ((noinline));
void shader_spill_mmx(Uint8*, Uint8*, int, int, float) __attribute__ ((noinline));
void shader_fbmerge_mmx2(Uint32 *, Uint8 *, int, int, float, Uint32) __attribute__((noinline));
/*
	convolve_mmx_w_shifts_generic

		DESCRIPTION:
		Does the convolution effect with the given coefficient table. The maximum size of the matrix is 8x8
		(so actually useful matrix sizes are 3x3, 5x5 and 7x7, but any numbers may be used, too).
		This version is somewhat `generic'. Almost no loop unrolling is done and thus no parallelism is exploited.
		If the matrix size is 3x3 or 5x5 you may wish to use the specially coded counterparts
		`convolve_mmx_w_shifts_3' and `convolve_mmx_w_shifts_5' which are guaranteed to be at least twice as fast.

		INPUT:
		*src - Input array of RGB values
		*dest- Output framebuffer (note that this function can't do the conversion in-place, src must be different from dest)
		resx - width of the framebuffer
		resy - height of the framebuffer
		*M   - the input convoluion matrix
		shift- if the sum of the coefficients is above 1, the convolution result must be `normalized', i.e.
		       divided by that sum. It works best when the sum is a power of two and so shifts can be used
		       instead of divides. The `shift' value here means how many bits to shift-right after addition.
		       If the shift is less than 1, no actual shifting occurs.

		OUTPUT: none (*dest contains the convolution effect result)

*/
void convolve_mmx_w_shifts_generic(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift)
{
	int toadd, n, cols, rows;
	Sint16 __attribute__ ((aligned(64))) mm[MA3X_MAX_SIZE*MA3X_MAX_SIZE][4];
	Uint16 __attribute__ ((aligned(16))) fourones[4] = {256, 256, 256, 256};
	Sint16 *pmm = mm[0];

	int i, j, k;
	for (i=0;i<M->n;i++)
		for (j=0;j<M->n;j++)
			for (k=0;k<4;k++) mm[i*M->n+j][k] = M->coeff[i][j];

	n = M->n;

	if (shift<1) shift = 0;
	toadd = (resx - M->n)*4;
	rows = resy - (n & ~1);
	cols = resx - (n & ~1);
	dest+=(resx*(M->n/2) + M->n/2);

	__asm __volatile(
	// initialize some mmx regs...
	"	push	%%ebx\n"
	"	pxor	%%mm7,	%%mm7\n"
	"	movd	%5, 	%%mm6\n"
	"	movd	%6,	%%mm5\n"
	// init the stuff

	// bx = M->coeff; si = src[...]; di = dst[...]
	"	movl	%2,	%%ebx\n"
	"	movl	%1,	%%esi\n"
	"	movl	%0,	%%edi\n"

	".rowloop:	\n"
	"	movl	%8,	%%edx\n"

	XALIGN
	".pixelloop:	\n"

	"	movl	%4,	%%ecx\n"
	"	pxor	%%mm0,	%%mm0\n"
	"	pxor	%%mm2,	%%mm2\n"

	XALIGN
	".y:	\n"

	"	movl	%4, 	%%eax\n"

	XALIGN
	".x:	\n"

	"	cmp	$2,	%%eax\n"
	"	jnb	.enough\n"

	"	movd	(%%esi),	%%mm1\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
	"	pmullw (%%ebx),	%%mm1\n"
	"	paddsw	%%mm1,	%%mm0\n"

	"	add	$4,	%%esi\n"
	"	add	$8,	%%ebx\n"

	"	jmp	.xend\n"

	XALIGN
	".enough:	\n"
	"	movd	(%%esi),	%%mm1\n"
	"	movd	4(%%esi),	%%mm3\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
	"	punpcklbw	%%mm7,	%%mm3\n"
	"	pmullw (%%ebx),	%%mm1\n"
	"	pmullw 8(%%ebx),	%%mm3\n"
	"	paddsw	%%mm1,	%%mm0\n"
	"	paddsw	%%mm3,	%%mm2\n"

	"	add	$8,	%%esi\n"
	"	add	$16,	%%ebx\n"
	"	sub	$2,	%%eax\n"
	"	jnz	.x\n"

	XALIGN
	".xend:\n"

	"	add	%3,	%%esi\n"

	"	dec	%%ecx\n"
	"	jnz	.y\n"

	"	paddsw	%%mm2,	%%mm0\n"
	"	paddsw	%%mm5,	%%mm0\n"
	"	psubusw	%%mm5,	%%mm0\n"
	"	psraw	%%mm6,	%%mm0\n"
	"	packuswb %%mm7,	%%mm0\n"
	"	movd	%%mm0,	(%%edi)\n"

	"	movl	%2,	%%ebx\n"

	"	movl	%4,	%%eax\n"
	"	xchg	%%edx,	%%ecx\n"
	"	mull	%3\n"
	"	sub	%%eax,	%%esi\n"
	"	movl	%4,	%%eax\n"
	"	mull	%4\n"
	"	shl	$2,	%%eax\n"
	"	sub	%%eax,	%%esi\n"

	"	xchg	%%edx,	%%ecx\n"

	"	add	$4,	%%edi\n"
	"	add	$4, 	%%esi\n"

	"	dec	%%edx\n"
	"	jnz	.pixelloop\n"

	"	movl	%4,	%%eax\n"
	"	andl	$0xfffffffe,	%%eax\n"
	"	shl	$2,	%%eax\n"
	"	add	%%eax,	%%esi\n"
	"	add	%%eax,	%%edi\n"

	"	decl	%7\n"
	"	jnz	.rowloop\n"
	"	pop	%%ebx\n"

	:"=m"(dest)
	:"m"(src), "m"(pmm), "m"(toadd), "m"(n), "m"(shift), "m"(*fourones), "m"(rows), "m"(cols)
	:"memory", "eax", "ecx", "edx", "esi", "edi"
	);

	__asm __volatile("emms"); // empty multimedia state
}

/*
	convolve_mmx_w_shifts_3

		DESCRIPTION:
		Does the convolution effect with the given coefficient table. This routine uses a hardcoded assumption, that
		the matrix size is 3 (and thus it is above twice as fast than the `generic version). If the convolution
		matrix is of any other size, error occurs.

		This routine maximizes parallelism utilization.

		INPUT:
		*src - Input array of RGB values
		*dest- Output framebuffer (note that this function can't do the conversion in-place, src must be different from dest)
		resx - width of the framebuffer
		resy - height of the framebuffer
		*M   - the input convoluion matrix
		shift- if the sum of the coefficients is above 1, the convolution result must be `normalized', i.e.
		       divided by that sum. It works best when the sum is a power of two and so shifts can be used
		       instead of divides. The `shift' value here means how many bits to shift-right after addition.
		       If the shift is less than 1, no actual shifting occurs.

		OUTPUT: none (*dest contains the convolution effect result)
*/

void convolve_mmx_w_shifts_3(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift)
{
	int toadd, cols, rows, n=3;
	Sint16 __attribute__ ((aligned(64))) mm[9][4];
	Uint16 __attribute__ ((aligned(16))) fourones[4] = {256, 256, 256, 256};
	Sint16 *pmm = mm[0];

	if (M->n!=3) {
		printf("convolve_mmx_w_shifts_3: expecting matrix size of 3, but size=%d received!\n", M->n);
		return;
	}

	int i, j, k;
	for (i=0;i<3;i++)
		for (j=0;j<3;j++)
			for (k=0;k<4;k++) mm[i*3+j][k] = M->coeff[i][j];

	if (shift<1) shift = 0;
	toadd = resx*4;
	rows = resy - 2;
	cols = resx - 2;
	dest+=(resx*1 + 1);

	__asm __volatile(
	"	push	%%ebx\n"
	// initialize some mmx regs...
	"	pxor	%%mm7,	%%mm7\n"
	"	movd	%5, 	%%mm6\n"
	// init the stuff

	// bx = M->coeff; si = src[...]; di = dst[...]
	"	movl	%2,	%%ebx\n"
	"	movl	%1,	%%esi\n"
	"	movl	%0,	%%edi\n"

	".rowloop_3:	\n"
	"	movl	%8,	%%edx\n"

	XALIGN
	".pixelloop_3:	\n"

	"	movd	(%%esi),	%%mm0\n"
	"	movd	4(%%esi),	%%mm1\n"
	"	movd	8(%%esi),	%%mm2\n"
	"	addl	%3,	%%esi\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
	"	punpcklbw	%%mm7,	%%mm2\n"

	"	pmullw	(%%ebx),	%%mm0\n"
	"	pmullw	8(%%ebx),	%%mm1\n"
	"	pmullw	16(%%ebx),	%%mm2\n"

	"	movd	(%%esi),	%%mm3\n"
	"	movd	4(%%esi),	%%mm4\n"
	"	movd	8(%%esi),	%%mm5\n"
	"	addl	%3,	%%esi\n"

	"	punpcklbw	%%mm7,	%%mm3\n"
	"	punpcklbw	%%mm7,	%%mm4\n"
	"	punpcklbw	%%mm7,	%%mm5\n"

	"	pmullw	24(%%ebx),	%%mm3\n"
	"	pmullw	32(%%ebx),	%%mm4\n"
	"	pmullw	40(%%ebx),	%%mm5\n"

	"	paddsw	%%mm0,	%%mm3\n"
	"	paddsw	%%mm1,	%%mm4\n"
	"	paddsw	%%mm2,	%%mm5\n"

	"	movd	(%%esi),	%%mm0\n"
	"	movd	4(%%esi),	%%mm1\n"
	"	movd	8(%%esi),	%%mm2\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
	"	punpcklbw	%%mm7,	%%mm2\n"

	"	pmullw	48(%%ebx),	%%mm0\n"
	"	pmullw	56(%%ebx),	%%mm1\n"
	"	pmullw	64(%%ebx),	%%mm2\n"


	"	paddsw	%%mm3,	%%mm0\n"
	"	paddsw	%%mm4,	%%mm1\n"
	"	paddsw	%%mm5,	%%mm2\n"

	"	paddsw	%%mm1,	%%mm0\n"
	"	paddsw	%6,	%%mm3\n"
	"	paddsw	%%mm3,	%%mm0\n"

	"	psubusw	%6,	%%mm0\n"
	"	psraw	%%mm6,	%%mm0\n"
	"	packuswb %%mm7,	%%mm0\n"
	"	movd	%%mm0,	(%%edi)\n"

	"	subl	%3,	%%esi\n"
	"	subl	%3,	%%esi\n"

	"	add	$4,	%%edi\n"
	"	add	$4, 	%%esi\n"

	"	dec	%%edx\n"
	"	jnz	.pixelloop_3\n"

	"	movl	%4,	%%eax\n"
	"	andl	$0xfffffffe,	%%eax\n"
	"	shl	$2,	%%eax\n"
	"	add	%%eax,	%%esi\n"
	"	add	%%eax,	%%edi\n"

	"	decl	%7\n"
	"	jnz	.rowloop_3\n"

	"	pop		%%ebx\n"
	:"=m"(dest)
	:"m"(src), "m"(pmm), "m"(toadd), "m"(n), "m"(shift), "m"(*fourones), "m"(rows), "m"(cols)
	:"memory", "eax", "ecx", "edx", "esi", "edi"
	);

	__asm __volatile("emms"); // empty multimedia state
}

// this function assumes that M->n is 5.
/*
	convolve_mmx_w_shifts_5

		DESCRIPTION: see the info for convolve_mmx_w_shifts_3, this function does the same,
	but is hardcoded and loop-unrolled for matrix size of 5, attempting to maximize parallelism utilization
*/
void convolve_mmx_w_shifts_5(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift)
{
	static int toadd;
	int cols, rows, n=5;
	Sint16 __attribute__ ((aligned(16))) mm[25][4];
	Uint16 __attribute__ ((aligned(16))) fourones[4] = {256, 256, 256, 256};
	Sint16 *pmm = mm[0];

	if (M->n!=5) {
		printf("convolve_mmx_w_shifts_5: expecting matrix size of 5, but size=%d received!\n", M->n);
		return;
	}

	int i, j, k;
	for (i=0;i<5;i++)
		for (j=0;j<5;j++)
			for (k=0;k<4;k++) mm[i*5+j][k] = M->coeff[i][j];

	if (shift<1) shift = 0;
	toadd = resx*4;
	rows = resy - 4;
	cols = resx - 4;
	dest+=(resx*2 + 2);

	__asm __volatile(
	// initialize some mmx regs...
	"	push	%%ebx\n"
	"	pxor	%%mm7,	%%mm7\n"
//	"	movd	%5, 	%%mm6\n"
	// init the stuff

	// bx = M->coeff; si = src[...]; di = dst[...]
	"	movl	%2,	%%ebx\n"
	"	movl	%1,	%%esi\n"
	"	movl	%0,	%%edi\n"

	".rowloop_5:	\n"
	"	movl	%8,	%%edx\n"

	XALIGN
	".pixelloop_5:	\n"

	// row 1:
	"	movd	  (%%esi),	%%mm2\n"
	"	movd	 4(%%esi),	%%mm3\n"
	"	movd	 8(%%esi),	%%mm4\n"
	"	movd	12(%%esi),	%%mm5\n"
	"	movd	16(%%esi),	%%mm6\n"
	"	addl	%3,	%%esi\n"

	"	punpcklbw	%%mm7,	%%mm2\n"
	"	punpcklbw	%%mm7,	%%mm3\n"
	"	punpcklbw	%%mm7,	%%mm4\n"
	"	punpcklbw	%%mm7,	%%mm5\n"
	"	punpcklbw	%%mm7,	%%mm6\n"

	"	pmullw	  (%%ebx),	%%mm2\n"
	"	pmullw	 8(%%ebx),	%%mm3\n"
	"	pmullw	16(%%ebx),	%%mm4\n"
	"	pmullw	24(%%ebx),	%%mm5\n"
	"	pmullw	32(%%ebx),	%%mm6\n"

	"	paddsw	%%mm2,	%%mm4\n"
	"	paddsw	%%mm3,	%%mm5\n"

	// row 2:
	"	movd	  (%%esi),	%%mm0\n"
	"	movd	 4(%%esi),	%%mm1\n"

	"	paddsw	%%mm4,	%%mm6\n"

	"	movd	 8(%%esi),	%%mm2\n"
	"	movd	12(%%esi),	%%mm3\n"
	"	movd	16(%%esi),	%%mm4\n"
	"	addl	%3,	%%esi\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
	"	punpcklbw	%%mm7,	%%mm2\n"
	"	punpcklbw	%%mm7,	%%mm3\n"
	"	punpcklbw	%%mm7,	%%mm4\n"

	"	pmullw	 40(%%ebx),	%%mm0\n"
	"	pmullw	 48(%%ebx),	%%mm1\n"
	"	pmullw	 56(%%ebx),	%%mm2\n"
	"	pmullw	 64(%%ebx),	%%mm3\n"
	"	pmullw	 72(%%ebx),	%%mm4\n"

	"	paddsw	%%mm0,	%%mm2\n"
	"	paddsw	%%mm1,	%%mm3\n"
	"	paddsw	%%mm4,	%%mm5\n"
	"	paddsw	%%mm2,	%%mm6\n"

	// row 3:
	"	movd	  (%%esi),	%%mm0\n"
	"	movd	 4(%%esi),	%%mm1\n"

	"	paddsw	%%mm3,	%%mm5\n"

	"	movd	 8(%%esi),	%%mm2\n"
	"	movd	12(%%esi),	%%mm3\n"
	"	movd	16(%%esi),	%%mm4\n"
	"	addl	%3,	%%esi\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
	"	punpcklbw	%%mm7,	%%mm2\n"
	"	punpcklbw	%%mm7,	%%mm3\n"
	"	punpcklbw	%%mm7,	%%mm4\n"

	"	pmullw	 80(%%ebx),	%%mm0\n"
	"	pmullw	 88(%%ebx),	%%mm1\n"
	"	pmullw	 96(%%ebx),	%%mm2\n"
	"	pmullw	104(%%ebx),	%%mm3\n"
	"	pmullw	112(%%ebx),	%%mm4\n"

	"	paddsw	%%mm0,	%%mm2\n"
	"	paddsw	%%mm1,	%%mm3\n"
	"	paddsw	%%mm4,	%%mm5\n"
	"	paddsw	%%mm2,	%%mm6\n"

	// row 4:
	"	movd	  (%%esi),	%%mm0\n"
	"	movd	 4(%%esi),	%%mm1\n"

	"	paddsw	%%mm3,	%%mm5\n"

	"	movd	 8(%%esi),	%%mm2\n"
	"	movd	12(%%esi),	%%mm3\n"
	"	movd	16(%%esi),	%%mm4\n"
	"	addl	%3,	%%esi\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
	"	punpcklbw	%%mm7,	%%mm2\n"
	"	punpcklbw	%%mm7,	%%mm3\n"
	"	punpcklbw	%%mm7,	%%mm4\n"

	"	pmullw	120(%%ebx),	%%mm0\n"
	"	pmullw	128(%%ebx),	%%mm1\n"
	"	pmullw	136(%%ebx),	%%mm2\n"
	"	pmullw	144(%%ebx),	%%mm3\n"
	"	pmullw	152(%%ebx),	%%mm4\n"

	"	paddsw	%%mm0,	%%mm2\n"
	"	paddsw	%%mm1,	%%mm3\n"
	"	paddsw	%%mm4,	%%mm5\n"
	"	paddsw	%%mm2,	%%mm6\n"

	// row 5:
	"	movd	  (%%esi),	%%mm0\n"
	"	movd	 4(%%esi),	%%mm1\n"

	"	paddsw	%%mm3,	%%mm5\n"

	"	movd	 8(%%esi),	%%mm2\n"
	"	movd	12(%%esi),	%%mm3\n"
	"	movd	16(%%esi),	%%mm4\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
	"	punpcklbw	%%mm7,	%%mm2\n"
	"	punpcklbw	%%mm7,	%%mm3\n"
	"	punpcklbw	%%mm7,	%%mm4\n"

	"	pmullw	160(%%ebx),	%%mm0\n"
	"	pmullw	168(%%ebx),	%%mm1\n"
	"	pmullw	176(%%ebx),	%%mm2\n"
	"	pmullw	184(%%ebx),	%%mm3\n"
	"	pmullw	192(%%ebx),	%%mm4\n"

// situation here: we have data all over the first 7 registers.. we must first sum it up...

	"	paddsw	%%mm5,	%%mm6\n"
	"	paddsw	%%mm0,	%%mm1\n"
	"	paddsw	%%mm2,	%%mm3\n"
	"	paddsw	%%mm4,	%%mm6\n"
	"	paddsw	%6,	%%mm1\n"
	"	movd	%5,	%%mm0\n"

	"	paddsw	%%mm6,	%%mm3\n"
	"	paddsw	%%mm3,	%%mm1\n"


	"	psubusw	%6,	%%mm1\n"
	"	psraw	%%mm0,	%%mm1\n"
	"	packuswb %%mm7,	%%mm1\n"
	"	movd	%%mm1,	(%%edi)\n"

	"	movl	%3,	%%eax\n"
	"	shl	$2,	%%eax\n"
	"	subl	%%eax,	%%esi\n"

	"	add	$4,	%%edi\n"
	"	add	$4, 	%%esi\n"

	"	dec	%%edx\n"
	"	jnz	.pixelloop_5\n"

	"	movl	%4,	%%eax\n"
	"	andl	$0xfffffffe,	%%eax\n"
	"	shl	$2,	%%eax\n"
	"	add	%%eax,	%%esi\n"
	"	add	%%eax,	%%edi\n"

	"	decl	%7\n"
	"	jnz	.rowloop_5\n"
	
	"	pop		%%ebx\n"

	:"=m"(dest)
	:"m"(src), "m"(pmm), "m"(toadd), "m"(n), "m"(shift), "m"(*fourones), "m"(rows), "m"(cols)
	:"memory", "eax", "ecx", "edx", "esi", "edi"
	);

	__asm __volatile("emms"); // empty multimedia state
}

/*
	convolve_mmx_w_shifts:

		DESCRIPTION: does the convolution effect on a given coefficien matrix (up to 8x8) with sum normalization
	using bitwize right shifts (note that this can only be done when the sum of the coeficients of the convolution matrix
	is either zero, or a power of two).
		Runs very fast in the special case of 3x3 or 5x5 matrices, as a special hardcoded and tightly optimized
	(loop unrolled) versions of the effect are called.
		All functions use the "original" MMX instruction set (it is OK on PentiumII and MMX-enabled Pentium-Is)

		INPUT:
		*src - Input array of RGB values
		*dest- Output framebuffer (note that this function can't do the conversion in-place, src must be different from dest)
		resx - width of the framebuffer
		resy - height of the framebuffer
		*M   - the input convoluion matrix
		shift- if the sum of the coefficients is above 1, the convolution result must be `normalized', i.e.
		       divided by that sum. It works best when the sum is a power of two and so shifts can be used
		       instead of divides. The `shift' value here means how many bits to shift-right after addition.
		       If the shift is less than 1, no actual shifting occurs.

		OUTPUT: none (*dest contains the convolution effect result)

*/
static inline void convolve_mmx_w_shifts(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift)
{
	//printf("called convolve_mmx_w_shifts(...,...,....,...,...,%d)\n", shift);
	switch (M->n) { // if we have a match for that matrix size, we should use it - it would be the fastest function
		case 3:  convolve_mmx_w_shifts_3(src, dest, resx, resy, M, shift); break;
		case 5:  convolve_mmx_w_shifts_5(src, dest, resx, resy, M, shift); break;
		default: convolve_mmx_w_shifts_generic(src, dest, resx, resy, M, shift);
	}
}

// shifts left all pixel data by the specified shift. This is the same as raising the gamma by 2^shift
// NOTE: uses MMX and requires the number of pixels % 4 == 0
void shader_gamma_shl(Uint32 *src, Uint32 *dest, int resx, int resy, int shift)
{
	int count = (resx * resy) / 4;

	// the innermost loop is 4 times unrolled
	__asm __volatile(
// prepare the loop:
	//"	ulaw\n"
	"	pxor	%%mm7,	%%mm7\n"
	"	movd	%2,	%%mm6\n"
	"	movl	%3,	%%eax\n"
	"	movl	%0,	%%edi\n"
	"	movl	%1,	%%esi\n"

	XALIGN
	".shader_gamma_shl_loop:"

	"	movd	(%%esi),	%%mm0\n"
	"	movd	4(%%esi),	%%mm1\n"
	"	movd	8(%%esi),	%%mm2\n"
	"	movd	12(%%esi),	%%mm3\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
	"	punpcklbw	%%mm7,	%%mm2\n"
	"	punpcklbw	%%mm7,	%%mm3\n"

	"	psllw	%%mm6,	%%mm0\n"
	"	psllw	%%mm6,	%%mm1\n"
	"	psllw	%%mm6,	%%mm2\n"
	"	psllw	%%mm6,	%%mm3\n"

	"	packuswb	%%mm7,	%%mm0\n"
	"	packuswb	%%mm7,	%%mm1\n"
	"	packuswb	%%mm7,	%%mm2\n"
	"	packuswb	%%mm7,	%%mm3\n"

	"	movd	%%mm0,	(%%edi)\n"
	"	movd	%%mm1,	4(%%edi)\n"
	"	movd	%%mm2,	8(%%edi)\n"
	"	movd	%%mm3,	12(%%edi)\n"

	"	add	$16,	%%esi\n"
	"	add	$16,	%%edi\n"
	"	dec	%%eax\n"
	"	jnz	.shader_gamma_shl_loop\n"
	"	emms\n"
	:"=m"(dest)
	:"m"(src), "m"(shift), "m"(count)
	:"memory", "eax", "esi", "edi");
}

// shifts right all pixel data by the specified shift. This is the same as lowering the gamma by 2^shift
// NOTE: uses MMX and requires the number of pixels % 4 == 0
void shader_gamma_shr(Uint32 *src, Uint32 *dest, int resx, int resy, int shift)
{
	int count = (resx * resy) / 4;

	// the innermost loop is 4 times unrolled
	__asm __volatile(
// prepare the loop:
	//"	ulaw\n"
	"	pxor	%%mm7,	%%mm7\n"
	"	movd	%2,	%%mm6\n"
	"	movl	%3,	%%eax\n"
	"	movl	%0,	%%edi\n"
	"	movl	%1,	%%esi\n"

	XALIGN
	".shader_gamma_shr_loop:"

	"	movd	(%%esi),	%%mm0\n"
	"	movd	4(%%esi),	%%mm1\n"
	"	movd	8(%%esi),	%%mm2\n"
	"	movd	12(%%esi),	%%mm3\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
	"	punpcklbw	%%mm7,	%%mm2\n"
	"	punpcklbw	%%mm7,	%%mm3\n"

	"	psrlw	%%mm6,	%%mm0\n"
	"	psrlw	%%mm6,	%%mm1\n"
	"	psrlw	%%mm6,	%%mm2\n"
	"	psrlw	%%mm6,	%%mm3\n"

	"	packuswb	%%mm7,	%%mm0\n"
	"	packuswb	%%mm7,	%%mm1\n"
	"	packuswb	%%mm7,	%%mm2\n"
	"	packuswb	%%mm7,	%%mm3\n"

	"	movd	%%mm0,	(%%edi)\n"
	"	movd	%%mm1,	4(%%edi)\n"
	"	movd	%%mm2,	8(%%edi)\n"
	"	movd	%%mm3,	12(%%edi)\n"

	"	add	$16,	%%esi\n"
	"	add	$16,	%%edi\n"
	"	dec	%%eax\n"
	"	jnz	.shader_gamma_shr_loop\n"
	"	emms\n"
	:"=m"(dest)
	:"m"(src), "m"(shift), "m"(count)
	:"memory", "eax", "esi", "edi");
}


/*
	fft_1D_complex_sse

		DESCRIPTION: SSE version of the 1-dimensional 2^m-point FFT, operating in the field of complex numbers.

			The code does four 2^m-point FFTs in parallel. The first FFT is performed on the multiple-of-four indicies
		of the real[] and imag[] arrays. The second FFT is on those indicies, which, divided by 4, yield 1 as a remainder, and
		so on. So if you want to make a 1024-point FFT, you must pass two 4096-element arrays, the first with real data
		and the second with imaginary data.
			There's no way to prevent making four ffts in parallel, but a simple workaround is (persume my_fft_func does
		a n-point fft:)

		void my_fft_func(complex *a, int n, int dir)
		{
			int i;
			float real[MAX_FFT_SIZE], imag[MAX_FFT_SIZE];
			for (i=0;i<n;i++) {
				real[4*i] = a[i].real;
				imag[4*i] = a[i].real;
			}
			FFT_1D_complex_sse(dir, log2(n), real, imag);
			for (i=0;i<n;i++) {
				a[i].real = real[4*i];
				a[i].imag = imag[4*i];
			}
		}

		This would be marginally slower than a dedicated scalar version of the FFT, since scalar arithmetic is not faster
		on the latest CPU's

		INPUT:
			dir - direction of the FFT 1 = forward(decimation in frequency), -1 = backward (decimation in time)
			m   - log2 of number of points
			x   - pointer to real floatingpoint input data, containing 4 * 2^m points
			y   - pointer to imaginary floatingpoint data, containing 4 * 2^m points

		OUTPUT:
			the conversion is done in-place
*/

void fft_1D_complex_sse(int dir, int m, float *x, float *y)
{
   long i,nn,j,l;
   float __attribute__ ((aligned(16))) c1[4],c2[4];
float __attribute__ ((aligned(16))) fftconsts[20] =	{ 1.0,  1.0,  1.0,  1.0,
							  0.0,  0.0,  0.0,  0.0,
							 -1.0, -1.0, -1.0, -1.0,
							  0.5,  0.5,  0.5,  0.5,
							 -2.0, -2.0, -2.0, -2.0
							};

   volatile int msv = m;
   /* Calculate the number of points */
   nn = 1;
   for (i=0;i<m;i++)
      nn *= 2;
   __asm __volatile(
   
	"	push	%%ebx\n"

   	"	movl	%2,	%%edx\n" //edx=i2
	"	decl	%2\n"
	"	shr	%%edx\n"
	"	xor	%%ebx,	%%ebx\n" //ebx=j
	"	movl	%0,	%%esi\n"
	"	movl	%1,	%%edi\n"
	"	xor	%%eax,	%%eax\n" //eax=i

	XALIGN
	".fft_l1:\n"

	"	cmp	%%ebx,	%%eax\n"
	"	jge	.fftb1\n"

	"	shl	$4,	%%eax\n"
	"	shl	$4,	%%ebx\n"

	"	movaps	(%%esi, %%eax),	%%xmm0\n" // the fastest xchg known to man!
	"	movaps	(%%edi, %%eax),	%%xmm1\n"
	"	movaps	(%%esi, %%ebx),	%%xmm2\n"
	"	movaps	(%%edi, %%ebx),	%%xmm3\n"

	"	movaps	%%xmm0,	(%%esi, %%ebx)\n"
	"	movaps	%%xmm1,	(%%edi, %%ebx)\n"
	"	movaps	%%xmm2,	(%%esi, %%eax)\n"
	"	movaps	%%xmm3,	(%%edi, %%eax)\n"

	"	shr	$4,	%%eax\n"
	"	shr	$4,	%%ebx\n"


	XALIGN
	".fftb1:\n"

	"	movl	%%edx,	%%ecx\n" // ecx=k
	XALIGN
	".fftwhile1:\n"
	"	cmp	%%ebx,	%%ecx\n"
	"	jg	.fftb2\n"

	"	sub	%%ecx,	%%ebx\n"
	"	shr	%%ecx\n"
	"	jmp	.fftwhile1\n"

	XALIGN
	".fftb2:\n"

	"	addl	%%ecx,	%%ebx\n"


	"	inc	%%eax\n"
	"	cmpl	%2,	%%eax\n"
	"	jb	.fft_l1\n"
	"	incl	%2\n"
	"	pop	%%ebx\n"

   :"=m"(x), "=m"(y)
   :"m"(nn)
   :"memory", "eax", "ecx", "edx", "esi", "edi"
   );

   /* Compute the FFT */

	l = m = msv;

__asm __volatile (// mapping: eax = i, ebx = i1, ecx = l1, edx = l2. esi = x, edi = y

//initialize:
	"	push	%%ebx\n"
	"	movl	%0,	%%esi\n"
	"	movl	%1,	%%edi\n"

	"	movaps	32%7,	%%xmm0\n"
	"	movaps	16%7,	%%xmm1\n"
	"	movaps	%%xmm0, %2\n" // move -1 to c1
	"	movaps	%%xmm1,	%3\n" // move  0 to c2

	"	movl	$1,	%%edx\n"

/* XMM mappings
   0, 1 - accumulators
   2, 3 - u1, u2
   4, 5 - t1, t2
*/
	XALIGN
	".fft_log2:\n"

	"	mov	%%edx,	%%ecx\n"
	"	shl	%%edx\n"

	"	movaps	%7,	%%xmm2\n" // move 1 to u1
	"	movaps	16%7,	%%xmm3\n" // move 0 to u2
	"	movl	$0,	%5\n"     // j = 0

	XALIGN
	".fft_c1:\n"
	"	movl	%5,	%%eax\n" // i= j

	XALIGN
	".fft_c2:\n"

	"	mov	%%eax,	%%ebx\n"
	"	add	%%ecx,	%%ebx\n" // ebx = i+l1

	"	shl	$4,	%%ebx\n"
	"	shl	$4,	%%eax\n"

	"	movaps	(%%esi, %%ebx),	%%xmm0\n" // xmm0 = x[i1]
	"	movaps	(%%edi,	%%ebx),	%%xmm1\n" // xmm1 = y[i1]
	"	movaps	%%xmm0,	%%xmm4\n"
	"	movaps	%%xmm1,	%%xmm5\n"

	"	mulps	%%xmm3,	%%xmm0\n"	// xmm0 = x[i1]*u2
	"	mulps	%%xmm3,	%%xmm1\n"	// xmm1 = y[i1]*u2
	"	mulps	%%xmm2,	%%xmm4\n"	// t1 = x[i1]*u1
	"	mulps	%%xmm2,	%%xmm5\n"	// t2 = y[i1]*u1

	"	subps	%%xmm1,	%%xmm4\n"	// t1 = x[i1]*u1 - y[i1]*u2
	"	addps	%%xmm0,	%%xmm5\n"	// t2 = x[i1]*u2 + y[i1]*u1

	"	movaps	(%%esi, %%eax),	%%xmm6\n" // xmm6 = x[i]
	"	movaps	(%%edi, %%eax),	%%xmm7\n" // xmm7 = y[i]

	"	movaps	%%xmm6,	%%xmm0\n"  // clone to xmm0 and xmm1
	"	movaps	%%xmm7,	%%xmm1\n"

	"	subps	%%xmm4,	%%xmm6\n" // xmm6 = x[i] - t1
	"	subps	%%xmm5,	%%xmm7\n" // xmm7 = x[i] - t2
	"	addps	%%xmm4,	%%xmm0\n" // xmm0 = x[i] + t1
	"	addps	%%xmm5,	%%xmm1\n" // xmm1 = x[i] + t2

	"	movaps	%%xmm6,	(%%esi,	%%ebx)\n" // x[i1] = x[i] - t1
	"	movaps	%%xmm7,	(%%edi,	%%ebx)\n" // y[i1] = y[i] - t2
	"	movaps	%%xmm0,	(%%esi,	%%eax)\n" // x[i] += t1
	"	movaps	%%xmm1,	(%%edi,	%%eax)\n" // y[i] += t2

	"	shr	$4,	%%ebx\n"
	"	shr	$4,	%%eax\n"

	"	addl	%%edx,	%%eax\n"
	"	cmpl	%6,	%%eax\n"
	"	jl	.fft_c2\n"

	"	movaps	%%xmm2,	%%xmm0\n" // xmm0 == xmm2 == u1
	"	movaps	%%xmm3,	%%xmm1\n" // xmm1 == xmm3 == u2

/*
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z; (c1 = %2, c2 = %3)
*/
	"	mulps	%2,	%%xmm2\n" // xmm2 = u1 * c1
	"	mulps	%2,	%%xmm3\n" // xmm3 = u2 * c1
	"	mulps	%3,	%%xmm0\n" // xmm0 = u1 * c2
	"	mulps	%3,	%%xmm1\n" // xmm1 = u2 * c2
	"	addps	%%xmm0,	%%xmm3\n" // new_u2 = u2 * c1 + u1 * c2
	"	subps	%%xmm1,	%%xmm2\n" // new_u1 = u1 * c1 - u2 * c2

	"	incl	%5\n"
	"	cmpl	%%ecx,	%5\n"
	"	jl	.fft_c1\n"
/*
      c2 = sqrt((1.0 - c1) * 0.5);
      if (dir == 1)
         c2 = -c2;
      c1 = sqrt((1.0 + c1) * 0.5);
*/
	"	movaps	%7,	%%xmm0\n" // xmm0 = 1.0
	"	movaps	%%xmm0,	%%xmm1\n" // xmm1 = 1.0
	"	subps	%2,	%%xmm0\n" // xmm0 = 1.0 - c1
	"	addps	%2,	%%xmm1\n" // xmm1 = 1.0 + c1
	"	mulps	48%7,	%%xmm0\n" // xmm0 = (1.0 - c1) * 0.5
	"	mulps	48%7,	%%xmm1\n" // xmm1 = (1.0 + c1) * 0.5
	"	sqrtps	%%xmm0,	%%xmm0\n" // xmm0 ^= 0.5
	"	sqrtps	%%xmm1,	%%xmm1\n" // xmm1 ^= 0.5

	"	movaps	%%xmm0,	%3\n"	// c2 = sqrt((1.0 - c1) * 0.5)
	"	movaps	%%xmm1,	%2\n"   // c1 = sqrt((1.0 + c1) * 0.5)

	"	cmpl	$1,	%8\n"
	"	jne	.fft_invb\n"

	"	mulps	64%7,	%%xmm0\n" // if (dir == 1) c2 = -c2
	"	addps	%3,	%%xmm0\n"
	"	movaps	%%xmm0,	%3\n"

	XALIGN
	".fft_invb:\n"

	"	decl	%4\n" // l--
	"	jnz	.fft_log2\n" // if l ... jump

   /* Scaling for forward transform */

	"	cmpl	$1,	%8\n"
	"	jne	.fft_nscale\n"

	"	cvtsi2ss	%6,	%%xmm0\n"
	"	movaps	%%xmm0,	%%xmm1\n"
	"	shufps	$0,	%%xmm0,	%%xmm1\n" //xmm1 = nn nn nn nn

	"	movaps	%7,	%%xmm0\n" // xmm0 = 1 1 1 1
	"	divps	%%xmm1,	%%xmm0\n" // xmm0 = 1/nn 1/nn 1/nn 1/nn
	//"	addps	%%xmm0,	%%xmm0\n" // xmm0 = 2/nn 2/nn 2/nn 2/nn

	"	movl	%6,	%%ecx\n"
	XALIGN
	".fft_scale:\n"

	"	movaps	%%xmm0,	%%xmm1\n"
	"	movaps	%%xmm0,	%%xmm2\n"
	"	mulps	(%%esi),	%%xmm1\n"
	"	mulps	(%%edi),	%%xmm2\n"
	"	movaps	%%xmm1,	(%%esi)\n"
	"	movaps	%%xmm2,	(%%edi)\n"

	"	addl	$16,	%%esi\n"
	"	addl	$16,	%%edi\n"

	"	decl	%%ecx\n"
	"	jnz	.fft_scale\n"

	XALIGN
	".fft_nscale:\n"

	"	emms\n"
	"	pop	%%ebx\n"

/*
	FFT Constants:
	offset		0	16	32	48	64
	constant	+1	 0	-1	0.5	-2
*/
:"=m"(x), "=m"(y), "=m"(*c1), "=m"(*c2), "=m"(l), "=m"(j)
:"m"(nn), "m"(*fftconsts) , "m"(dir)
:"memory", "eax", "ecx", "edx", "esi", "edi"
);

}

/*
	this function replaces the following code:
      		for (i=0;i<fft_size;i++) {
         		real[4*i  ] = c[i][4*j  ].re;
         		imag[4*i  ] = c[i][4*j  ].im;
         		real[4*i+1] = c[i][4*j+1].re;
         		imag[4*i+1] = c[i][4*j+1].im;
         		real[4*i+2] = c[i][4*j+2].re;
         		imag[4*i+2] = c[i][4*j+2].im;
         		real[4*i+3] = c[i][4*j+3].re;
         		imag[4*i+3] = c[i][4*j+3].im;
      		}
*/
void float_copy_ij_i(float *a, float *b, complex c[], int fft_size)
{
	__asm __volatile(
	"	movl	%3,	%%eax\n"
	//"	movl	%%eax,	%%edx\n"
	//"	shl	$3,	%%edx\n"
	"	movl	%0,	%%esi\n"
	"	movl	%1,	%%edi\n"
	"	movl	%2,	%%ecx\n"

	XALIGN
	".copy_ij_i_loop:\n"

	"	movaps	(%%ecx),	%%xmm0\n" // xmm0 = re[0] im[0] re[1] im[1]
	"	movaps	16(%%ecx),	%%xmm1\n" // xmm1 = re[2] im[2] re[3] im[3]
	"	movaps	%%xmm0,	%%xmm2\n"	  // xmm2 = re[0] im[0] re[1] im[1]
	"	movaps	%%xmm1,	%%xmm3\n"	  // xmm3 = re[2] im[2] re[3] im[3]
	"	shufps	$0x88,	%%xmm1,	%%xmm0\n" // 10 00 10 00 // xmm0 = re[0] re[1] re[2] re[3]
	"	shufps	$0xdd,	%%xmm3,	%%xmm2\n" // 11 01 11 01 // xmm2 = im[0] im[1] im[2] im[3]
	"	movaps	%%xmm0,	(%%esi)\n"
	"	movaps	%%xmm2,	(%%edi)\n"

	"	add	$8192,	%%ecx\n"
	"	add	$16,	%%esi\n"
	"	add	$16,	%%edi\n"

	"	decl	%%eax\n"
	"	jnz	.copy_ij_i_loop\n"

	"	emms\n"
	:"=m"(a), "=m"(b)
	:"m"(c), "m"(fft_size)
	:"memory", "eax", "ecx", "edx", "esi", "edi"
	);

}

/*
	this function replaces the following code:
      		for (i=0;i<fft_size;i++) {
         		 c[i][4*j  ].re = real[4*i  ];
         		 c[i][4*j  ].im = imag[4*i  ];
         		 c[i][4*j+1].re = real[4*i+1];
         		 c[i][4*j+1].im = imag[4*i+1];
         		 c[i][4*j+2].re = real[4*i+2];
         		 c[i][4*j+2].im = imag[4*i+2];
         		 c[i][4*j+3].re = real[4*i+3];
         		 c[i][4*j+3].im = imag[4*i+3];
      		}
*/
void float_copy_i_ij(float *a, float *b, complex c[], int fft_size)
{
	__asm __volatile(
	"	movl	%3,	%%eax\n"
	//"	movl	%%eax,	%%edx\n"
	//"	shl	$3,	%%edx\n"
	"	movl	%0,	%%esi\n"
	"	movl	%1,	%%edi\n"
	"	movl	%2,	%%ecx\n"

	XALIGN
	".copy_i_ij_loop:\n"

	"	movaps	(%%esi),	%%xmm0\n" // xmm0 = re[0] re[1] re[2] re[3]
	"	movaps	(%%edi),	%%xmm1\n" // xmm1 = im[0] im[1] im[2] im[3]
	"	movaps	%%xmm0,	%%xmm2\n"	  // xmm2 = re[0] re[1] re[2] re[3]
	"	movaps	%%xmm1,	%%xmm3\n"	  // xmm3 = im[0] im[1] im[2] im[3]

	"	unpcklps	%%xmm1,	%%xmm0\n" // xmm0 = re[0] im[0] re[1] im[1]
	"	unpckhps	%%xmm3,	%%xmm2\n" // xmm2 = re[2] im[2] re[3] im[3]

	"	movaps	%%xmm0,	(%%ecx)\n"
	"	movaps	%%xmm2,	16(%%ecx)\n"

	"	add	$8192,	%%ecx\n"
	"	add	$16,	%%esi\n"
	"	add	$16,	%%edi\n"

	"	decl	%%eax\n"
	"	jnz	.copy_i_ij_loop\n"

	"	emms\n"
	:"=m"(a), "=m"(b)
	:"m"(c), "m"(fft_size)
	:"memory", "eax", "ecx", "edx", "esi", "edi"
	);

}


/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/


/*
	this function replaces the following code:
      		for (j=0;j<fft_size;k++) {
         		real[4*j  ] = c[4*i  ][j].re;
         		imag[4*j  ] = c[4*i  ][j].im;
         		real[4*j+1] = c[4*i+1][j].re;
         		imag[4*j+1] = c[4*i+1][j].im;
         		real[4*j+2] = c[4*i+2][j].re;
         		imag[4*j+2] = c[4*i+2][j].im;
         		real[4*j+3] = c[4*i+3][j].re;
         		imag[4*j+3] = c[4*i+3][j].im;
      		}
*/
void float_copy_ij_j(float *a, float *b, complex c[], int fft_size)
{
	__asm __volatile(
	"	movl	%3,	%%eax\n"
	"	movl	%0,	%%esi\n"
	"	movl	%1,	%%edi\n"
	"	movl	%2,	%%ecx\n"

	XALIGN
	".copy_ij_j_loop:\n"

	"	movlps	(%%ecx),	%%xmm0\n"
	"	movhps	 8192(%%ecx),	%%xmm0\n" // xmm0 = re[0] im[0] re[1] im[1]
	"	movlps	16384(%%ecx),	%%xmm1\n"
	"	movhps	24576(%%ecx),	%%xmm1\n" // xmm1 = re[2] im[2] re[3] im[3]
	"	movaps	%%xmm0,	%%xmm2\n"	  // xmm2 = re[0] im[0] re[1] im[1]
	"	movaps	%%xmm1,	%%xmm3\n"	  // xmm3 = re[2] im[2] re[3] im[3]
	"	shufps	$0x88,	%%xmm1,	%%xmm0\n" // 10 00 10 00 // xmm0 = re[0] re[1] re[2] re[3]
	"	shufps	$0xdd,	%%xmm3,	%%xmm2\n" // 11 01 11 01 // xmm2 = im[0] im[1] im[2] im[3]
	"	movaps	%%xmm0,	(%%esi)\n"
	"	movaps	%%xmm2,	(%%edi)\n"

	"	add	$8,	%%ecx\n"
	"	add	$16,	%%esi\n"
	"	add	$16,	%%edi\n"

	"	decl	%%eax\n"
	"	jnz	.copy_ij_j_loop\n"

	"	emms\n"
	:"=m"(a), "=m"(b)
	:"m"(c), "m"(fft_size)
	:"memory", "eax", "ecx", "esi", "edi"
	);

}


/*
	this function replaces the following code:
      		for (j=0;j<fft_size;k++) {
         		 c[4*i  ][j].re = real[4*j  ];
         		 c[4*i  ][j].im = imag[4*j  ];
         		 c[4*i+1][j].re = real[4*j+1];
         		 c[4*i+1][j].im = imag[4*j+1];
         		 c[4*i+2][j].re = real[4*j+2];
         		 c[4*i+2][j].im = imag[4*j+2];
         		 c[4*i+3][j].re = real[4*j+3];
         		 c[4*i+3][j].im = imag[4*j+3];
      		}
*/

void float_copy_j_ij(float *a, float *b, complex c[], int fft_size)
{
	__asm __volatile(
	"	movl	%3,	%%eax\n"
	"	movl	%0,	%%esi\n"
	"	movl	%1,	%%edi\n"
	"	movl	%2,	%%ecx\n"

	XALIGN
	".copy_j_ij_loop:\n"

	"	movaps	(%%esi),	%%xmm0\n" // xmm0 = re[0] re[1] re[2] re[3]
	"	movaps	(%%edi),	%%xmm1\n" // xmm1 = im[0] im[1] im[2] im[3]
	"	movaps	%%xmm0,	%%xmm2\n"	  // xmm2 = re[0] re[1] re[2] re[3]
	"	movaps	%%xmm1,	%%xmm3\n"	  // xmm3 = im[0] im[1] im[2] im[3]

	"	unpcklps	%%xmm1,	%%xmm0\n" // xmm0 = re[0] im[0] re[1] im[1]
	"	unpckhps	%%xmm3,	%%xmm2\n" // xmm2 = re[2] im[2] re[3] im[3]

	"	movlps	%%xmm0,	(%%ecx)\n"
	"	movhps	%%xmm0,	 8192(%%ecx)\n"
	"	movlps	%%xmm2,	16384(%%ecx)\n"
	"	movhps	%%xmm2,	24576(%%ecx)\n"

	"	add	$8,	%%ecx\n"
	"	add	$16,	%%esi\n"
	"	add	$16,	%%edi\n"

	"	decl	%%eax\n"
	"	jnz	.copy_j_ij_loop\n"

	"	emms\n"
	:"=m"(a), "=m"(b)
	:"m"(c), "m"(fft_size)
	:"memory", "eax", "ecx", "esi", "edi"
	);

}

void shader_spill_mmx(Uint8 *src, Uint8 * dest, int resx, int resy, float coeff)
{
	SSE_ALIGN(unsigned short vcoeff[4]);
	SSE_ALIGN(unsigned short vncoeff[4]);
	SSE_ALIGN(unsigned short firstok[4]) = {0xffff,0,0,0};
	SSE_ALIGN(unsigned short lastok[4]) = {0,0,0,0xffff};
	vcoeff[0] = (unsigned short) (255 * coeff);
	vncoeff[0] = (unsigned short) (255 * (1-coeff));
	for (int i = 1; i < 4; i++) {
		vcoeff[i] = vcoeff[0];
		vncoeff[i] = vncoeff[0];
	}
	__asm __volatile (
//mmx init:
		"pxor	%%mm5,	%%mm5\n"
		"movq	%4,	%%mm6\n"
		"movq	%5,	%%mm7\n"


/* HORIZONTAL PASSES (IA32) */
		"mov	%1,	%%esi\n"
		"mov	%0,	%%edi\n"
		"mov	%3,	%%edx\n"
"f_y_0:\n"
		"mov	%2,	%%ecx\n"
		"shr	$2,	%%ecx\n"
		"pxor	%%mm0,	%%mm0\n"
XALIGN
"f_x_0:\n"
		"movd	(%%esi),	%%mm1\n"
		"punpcklbw	%%mm5,	%%mm1\n"
		"mov	$4,	%%eax\n"
XALIGN
"quad_loop_0:\n"
		"movq	%6,	%%mm3\n"
		"pand	%%mm1,	%%mm3\n"
		"pmullw	%%mm7,	%%mm0\n"
		"pmullw	%%mm6,	%%mm3\n"
		"paddusw	%%mm3,	%%mm0\n"
		"psllq	$48,	%%mm0\n"
		"psrlq	$16,	%%mm1\n"
		"por		%%mm0,	%%mm1\n"
		"psrlq	$56,	%%mm0\n"
		"dec	%%eax\n"
		"jnz	quad_loop_0\n"
		//
		"psrlw	$8,	%%mm1\n"
		"packuswb	%%mm5,	%%mm1\n"
		"movd	%%mm1,	(%%edi)\n"
	 	"add	$4,	%%esi\n"
		"add	$4,	%%edi\n"
		"dec	%%ecx\n"
		"jnz	f_x_0\n"
// REVERSE:
		"mov	%2,	%%ecx\n"
		"shr	$2,	%%ecx\n"
		"pxor	%%mm0,	%%mm0\n"
XALIGN
"f_x_1:\n"
		"sub	$4,	%%edi\n"
		"movd	(%%edi),	%%mm1\n"
		"punpcklbw	%%mm5,	%%mm1\n"
		"mov	$4,	%%eax\n"
XALIGN
"quad_loop_1:\n"
		"movq	%7,	%%mm3\n"
		"pand	%%mm1,	%%mm3\n"
		"pmullw	%%mm7,	%%mm0\n"
		"pmullw	%%mm6,	%%mm3\n"
		"paddusw	%%mm3,	%%mm0\n"
		"psrlq	$48,	%%mm0\n"
		"psllq	$16,	%%mm1\n"
		"por		%%mm0,	%%mm1\n"
		"psllq	$40,	%%mm0\n"
		"dec	%%eax\n"
		"jnz	quad_loop_1\n"
		//
		"psrlw	$8,	%%mm1\n"
		"packuswb	%%mm5,	%%mm1\n"
		"movd	%%mm1,	(%%edi)\n"
		"dec	%%ecx\n"
		"jnz	f_x_1\n"

		//
		"add	%2,	%%edi\n"
		"dec	%%edx\n"
		"jnz	f_y_0\n"


/* VERTICAL PASSES (%%mmX) */

//addr init:
		"mov	%0,	%%edi\n"
		"mov	%2,	%%ecx\n"
		"shr	$2,	%%ecx\n"
		"mov	%2,	%%esi\n"
"f_x_2:\n"
		"mov	$2,	%%edx\n"
XALIGN
"negloop:\n"
		"movd	(%%edi),	%%mm0\n"
		"punpcklbw	%%mm5,	%%mm0\n"
		"mov	%3,	%%eax\n"
		"dec	%%eax\n"
XALIGN
"f_y_1:\n"
		"add	%%esi,	%%edi\n"
		"movd	(%%edi),	%%mm1\n"
		"pmullw	%%mm7,	%%mm0\n"
		"punpcklbw	%%mm5,	%%mm1\n"
		"pmullw	%%mm6,	%%mm1\n"
		"paddusw	%%mm1,	%%mm0\n"
		"psrlw	$8,	%%mm0\n"
		"movq	%%mm0,	%%mm2\n"
		"packuswb	%%mm5,	%%mm2\n"
		"movd	%%mm2,	(%%edi)\n"
		//
		"dec	%%eax\n"
		"jnz	f_y_1\n"
		//
		"neg	%%esi\n"
		"dec	%%edx\n"
		"jnz	negloop\n"
		//
		"add	$4,	%%edi\n"
		"dec	%%ecx\n"
		"jnz f_x_2\n"

		"emms\n"
		:
		:"m"(dest), "m"(src), "m"(resx), "m"(resy), "m"(*vcoeff), 
		"m"(*vncoeff), "m"(*firstok), "m"(*lastok)
		:"memory", "eax", "ecx", "edx", "esi", "edi"
	);
}

void shader_fbmerge_mmx2(Uint32 *dest, Uint8 * src, int resx, int resy, float intensity, Uint32 glow_color)
{
	SSE_ALIGN(unsigned short vint[4]);
	SSE_ALIGN(unsigned short vnint[4]);
	SSE_ALIGN(unsigned short glowcol[4]);
	vint[0] = (int)(65535* intensity);
	vnint[0] = 65535;
	glowcol[0] = (glow_color<<8) & 0xff00;
	glowcol[1] = (glow_color   ) & 0xff00;
	glowcol[2] = (glow_color>>8) & 0xff00;
	for (int i = 1; i < 4; i++) {
		vint[i] = vint[0];
		vnint[i] = vnint[0];
	}
	__asm __volatile (
		"pxor	%%mm7,	%%mm7\n"
		"movq	%6,	%%mm4\n"
		"movq	%4,	%%mm5\n"
		"movq	%5,	%%mm6\n"
		
		"mov	%2,	%%eax\n"
		"mov	%3,	%%edx\n"
		"mul	%%edx\n"
		"shr	$2,	%%eax\n"
		"mov	%1,	%%esi\n"
		"mov	%0,	%%edi\n"

/*op:
		int mult1 = (int) (((src[i] & 0xff000000) >> 16)*intensity);
		dest[i] = multiplycolor(dest[i], 65535 - mult1)  + multiplycolor(glow_color, mult1);
*/
	XALIGN
	"pix_loop_1:\n"
		"movd	(%%esi),	%%mm3\n"
		
		"punpcklbw	%%mm7,	%%mm3\n"
		"psllw	$8,	%%mm3\n"
		"pmulhuw	%%mm5,	%%mm3\n"

//0:
		"movd	(%%edi),	%%mm1\n"
		"punpcklbw	%%mm7,	%%mm1\n"
		"pshufw	$0x00,	%%mm3,	%%mm0\n"
		"psllw	$8,	%%mm1\n"
		"movq	%%mm6,	%%mm2\n"
		"psubw	%%mm0,	%%mm2\n"
		"pmulhuw	%%mm2,	%%mm1\n"
		"pmulhuw	%%mm4 ,	%%mm0\n"
		"paddusw	%%mm1,	%%mm0\n"
		"psrlw	$8,	%%mm0\n"
		"packuswb	%%mm7,	%%mm0\n"
		"movd	%%mm0,	(%%edi)\n"
//1:
		"movd	4(%%edi),	%%mm1\n"
		"punpcklbw	%%mm7,	%%mm1\n"
		"pshufw	$0x55,	%%mm3,	%%mm0\n"
		"psllw	$8,	%%mm1\n"
		"movq	%%mm6,	%%mm2\n"
		"psubw	%%mm0,	%%mm2\n"
		"pmulhuw	%%mm2,	%%mm1\n"
		"pmulhuw	%%mm4 ,	%%mm0\n"
		"paddusw	%%mm1,	%%mm0\n"
		"psrlw	$8,	%%mm0\n"
		"packuswb	%%mm7,	%%mm0\n"
		"movd	%%mm0,	4(%%edi)\n"
//2:
		"movd	8(%%edi),	%%mm1\n"
		"punpcklbw	%%mm7,	%%mm1\n"
		"pshufw	$0xaa,	%%mm3,	%%mm0\n"
		"psllw	$8,	%%mm1\n"
		"movq	%%mm6,	%%mm2\n"
		"psubw	%%mm0,	%%mm2\n"
		"pmulhuw	%%mm2,	%%mm1\n"
		"pmulhuw	%%mm4 ,	%%mm0\n"
		"paddusw	%%mm1,	%%mm0\n"
		"psrlw	$8,	%%mm0\n"
		"packuswb	%%mm7,	%%mm0\n"
		"movd	%%mm0,	8(%%edi)\n"
//3:
		"movd	12(%%edi),	%%mm1\n"
		"punpcklbw	%%mm7,	%%mm1\n"
		"pshufw	$0xcc,	%%mm3,	%%mm0\n"
		"psllw	$8,	%%mm1\n"
		"movq	%%mm6,	%%mm2\n"
		"psubw	%%mm0,	%%mm2\n"
		"pmulhuw	%%mm2,	%%mm1\n"
		"pmulhuw	%%mm4 ,	%%mm0\n"
		"paddusw	%%mm1,	%%mm0\n"
		"psrlw	$8,	%%mm0\n"
		"packuswb	%%mm7,	%%mm0\n"
		"movd	%%mm0,	12(%%edi)\n"

		//
		"add	$4,	%%esi\n"
		"add	$16,	%%edi\n"

		"dec	%%eax\n"
		"jnz	pix_loop_1\n"

		"emms\n"
		:
		:"m"(dest), "m"(src), "m"(resx), "m"(resy), 
		 "m"(*vint), "m"(*vnint), "m"(*glowcol)
		:"memory", "eax", "ecx", "edx", "esi", "edi"
	);
}

void shader_sobel_sse(Uint32 *src, Uint32 *dest, int resx, int resy, const int hk[], const int vk[]) __attribute__((noinline));
void shader_sobel_sse(Uint32 *src, Uint32 *dest, int resx, int resy, const int hk[], const int vk[])
{
	SSE_ALIGN(short int h_kernel[9][4]);
	SSE_ALIGN(short int v_kernel[9][4]);
	SSE_ALIGN(float two_pow_m16[4]) = {
		0.0000152587890625,0.0000152587890625,0.0000152587890625,0.0000152587890625
	};

	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 4; j++) {
			h_kernel[i][j] = (short int) hk[i];
			v_kernel[i][j] = (short int) vk[i];
		}
	}
	short int *fh_kernel = &h_kernel[0][0];
	short int *fv_kernel = &v_kernel[0][0];
	for (int i = 0; i < resx; i++) dest[i] = dest[(resy-1)*resx+i] = 0;
	src += resx;
	dest += resx;
	resy -= 2;
	//
	__asm __volatile
	(
"               pxor    %%mm7,  %%mm7\n"
"               mov     %0,     %%esi\n"
"               mov     %1,     %%edi\n"
"               xor     %%edx,  %%edx\n"
"               movups  %2,     %%xmm7\n"

XALIGN
"       yloop:\n"
"               mov     $2,     %%eax\n"
"               movl    $0,     (%%edi)\n"
"               add     $4,     %%esi\n"
"               add     $4,     %%edi\n"

XALIGN
"       xloop:\n"

"               push    %%eax\n"

"               mov     $3,     %%ecx\n"
"               mov     %3,     %%eax\n"
"               neg     %%eax\n"
"               lea     -4(%%esi,%%eax,4),      %%esi\n"
"               pxor    %%mm0,  %%mm0\n"
"               pxor    %%mm1,  %%mm1\n"

XALIGN
"       yiloop:\n"

"               lea     -8(,%%ecx, 8),        %%eax\n"
"               lea     (%%eax, %%eax, 2),      %%eax\n"
"               add	%4,     %%eax\n"

                /* 1 */
"               movq    (%%eax),        %%mm4\n"
"               movq    -80(%%eax),      %%mm5\n"
"               movd    (%%esi),        %%mm6\n"

"               punpcklbw       %%mm7,  %%mm6\n"
"               pmullw  %%mm6,  %%mm4\n"
"               pmullw  %%mm6,  %%mm5\n"
"               paddw   %%mm4,  %%mm0\n"
"               paddw   %%mm5,  %%mm1\n"
                /* 2 */
"               movq    8(%%eax),       %%mm4\n"
"               movq    -72(%%eax),      %%mm5\n"
"               movd    4(%%esi),       %%mm6\n"

"               punpcklbw       %%mm7,  %%mm6\n"
"               pmullw  %%mm6,  %%mm4\n"
"               pmullw  %%mm6,  %%mm5\n"
"               paddw   %%mm4,  %%mm0\n"
"               paddw   %%mm5,  %%mm1\n"
                /* 3 */
"               movq    16(%%eax),      %%mm4\n"
"               movq    -64(%%eax),      %%mm5\n"
"               movd    8(%%esi),       %%mm6\n"

"               punpcklbw       %%mm7,  %%mm6\n"
"               pmullw  %%mm6,  %%mm4\n"
"               pmullw  %%mm6,  %%mm5\n"
"               paddw   %%mm4,  %%mm0\n"
"               paddw   %%mm5,  %%mm1\n"
                // row ready

"               mov     %3,     %%eax\n"
"               lea     (%%esi, %%eax, 4), %%esi\n"
"               dec     %%ecx\n"
"               jnz     yiloop\n"
                // result ready in mm0 and mm1
"               movq    %%mm0,  %%mm2\n"
"               movq    %%mm1,  %%mm3\n"

                // convert to dwords
"               punpcklwd       %%mm7,  %%mm0\n"
"               punpcklwd       %%mm7,  %%mm1\n"
"               punpckhwd       %%mm7,  %%mm2\n"
"               punpckhwd       %%mm7,  %%mm3\n"

                // shift 'em, so that sign is preserved:
"               pslld   $16,    %%mm0\n"
"               pslld   $16,    %%mm1\n"
"               pslld   $16,    %%mm2\n"
"               pslld   $16,    %%mm3\n"

                //transfer to SSE
"               cvtpi2ps        %%mm0,  %%xmm0\n"
"               cvtpi2ps        %%mm1,  %%xmm1\n"
"               cvtpi2ps        %%mm2,  %%xmm2\n"
"               cvtpi2ps        %%mm3,  %%xmm3\n"

                // repackage in xmm. Xmm0 + Xmm2, Xmm1 + Xmm3
"               movlhps %%xmm2, %%xmm0\n"
"               movlhps %%xmm3, %%xmm1\n"

                // perform ops
"               mulps   %%xmm0, %%xmm0\n"
"               mulps   %%xmm1, %%xmm1\n"
"               addps   %%xmm1, %%xmm0\n"

                // Square root

                //sqrtps        %%xmm0, %%xmm0
                //using these instead of sqrtps
"               addps   %%xmm7, %%xmm0\n"
"               rsqrtps %%xmm0, %%xmm0\n"
"               rcpps   %%xmm0, %%xmm0\n"

                // Negate the effect of the MMX shifting by 16 position: multiply by 2^-16
"               mulps   %%xmm7, %%xmm0\n"

                // unpackage
"               movhlps %%xmm0, %%xmm1\n"

                // transfer back to MMX
"               cvtps2pi        %%xmm0, %%mm0\n"
"               cvtps2pi        %%xmm1, %%mm1\n"

                // truncate to 16bit with saturation
"               packssdw        %%mm1,  %%mm0\n"

                // truncate to 8bit with saturation
"               packuswb        %%mm7,  %%mm0\n"

                // push the result
"               movd    %%mm0,  (%%edi)\n"

"               mov     %3,     %%eax\n"
"               neg     %%eax\n"
"               lea     4(%%esi, %%eax, 8), %%esi\n"
"               pop     %%eax\n"
                // end of x loop
"               add     $4,     %%esi\n"
"               add     $4,     %%edi\n"
"               inc     %%eax\n"
"               cmp     %3,     %%eax\n"
"               jb      xloop\n"

                // end of y loop
"               movl    $0,     (%%edi)\n"
"               add     $4,     %%edi\n"
"               add     $4,     %%esi\n"
"               inc     %%edx\n"
"               cmp     %6,    %%edx\n"
"               jb      yloop\n"
"               emms\n"
::"m"(src), "m"(dest), "m"(*two_pow_m16), "m"(resx), "m"(fh_kernel), "m"(fv_kernel), "m"(resy)
:"memory", "eax", "ecx", "edx", "esi", "edi"
	);

}

#endif

#ifdef apply_fft_filter_asm
void apply_fft_filter_sse(complex *dest, float *filter, int fft_size)
{
	if (!filter) return;
	fft_size = fft_size * fft_size / 4;
	do {
	//	dest->re *= (*filter);
	//	dest->im *= (*filter);
	//	dest++; filter++;
		__asm __volatile(

		"	movups	%1,	%%xmm0\n" // f1 f2 f3 f4
		"	movaps	%%xmm0,	%%xmm1\n" // f1 f2 f3 f4
		"	movaps	%%xmm0,	%%xmm2\n" // f1 f2 f3 f4

		"	shufps  $0x50,	%%xmm0,	%%xmm1\n" // 01 01 00 00 = f1 f1 f2 f2
		"	shufps	$0xfa,	%%xmm0,	%%xmm2\n" // 11 11 10 10 = f3 f3 f4 f4

		"	mulps	%0,	%%xmm1\n"
		"	mulps	16%0,	%%xmm2\n"

		"	movaps	%%xmm1,	%0\n"
		"	movaps	%%xmm2,	16%0\n"

		:"=m"(*dest)
		:"m"(*filter)
		:"memory"
		);
		dest+=4; filter+=4;
	}while (--fft_size);
}

#endif

#ifdef bilinea_p5_asm
	__asm __volatile (
    "   push    %%ebx\n"
	"	pxor	%%mm7,	%%mm7\n"
	"	movd	%1,	%%mm2\n"
	"	movl	%5,	%%eax\n"
	"	movl	%6,	%%ebx\n"

	"	punpcklbw	%%mm7,	%%mm2\n"

	"	movb	$255,	%%cl\n"
	"	movb	$255,	%%dl\n"

	"	movd	%2,	%%mm3\n"

	"	subb	%%al,	%%cl\n"
	"	subb	%%bl,	%%dl\n"

	"	punpcklbw	%%mm7,	%%mm3\n"

// ax = Fx, bx = Fy. cx = 1-Fx, dx = 1-Fy

// ax = 	Fx	1-Fx	  Fx	1-Fx
// bx = 	Fy	  Fy	1-Fy	1-Fy

	"	mov	%%al,	%%ch\n"
	"	mov	%%bl,	%%bh\n"

	"	movd	%3,	%%mm4\n"

	"	mov	%%cx,	%%ax\n"
	"	shl	$16,	%%ebx\n"

	"	punpcklbw	%%mm7,	%%mm4\n"

	"	mov	%%dl,	%%dh\n"
	"	shl	$16,	%%eax\n"
	"	movd	%4,	%%mm5\n"
	"	mov	%%dx,	%%bx\n" // bx ready
	"	mov	%%cx,	%%ax\n" // ax ready

	"	punpcklbw	%%mm7,	%%mm5\n"

	"	movd	%%ebx,	%%mm1\n"
	"	movd	%%eax,	%%mm0\n"
	"	movl	$0xffff,	%%edx\n"

	"	punpcklbw	%%mm7,	%%mm1\n"
	"	punpcklbw	%%mm7,	%%mm0\n"

	"	movd	%%edx,	%%mm6\n" //mm6 = 0xffff

	"	pmullw	%%mm1,	%%mm0\n"
	"	psrlw	$8,	%%mm0\n"
//1:
	"	movq	%%mm6,	%%mm7\n"
	"	movq	%%mm6,	%%mm1\n"
	"	psllq	$16,	%%mm7\n"

	"	pand	%%mm0,	%%mm6\n"
	"	pand	%%mm0,	%%mm1\n"

	"	psllq	$16,	%%mm6\n"
	"	por	%%mm6,	%%mm1\n"
	"	psllq	$16,	%%mm6\n"
	"	por	%%mm6,	%%mm1\n"

	"	pmullw	%%mm1,	%%mm2\n"
//2:
	"	movq	%%mm7,	%%mm6\n"
	"	movq	%%mm7,	%%mm1\n"
	"	psllq	$16,	%%mm7\n"

	"	pand	%%mm0,	%%mm6\n"
	"	pand	%%mm0,	%%mm1\n"
	"	psllq	$16,	%%mm6\n"
	"	por	%%mm6,	%%mm1\n"
	"	psrlq	$32,	%%mm6\n"
	"	por	%%mm6,	%%mm1\n"

	"	pmullw	%%mm1,	%%mm3\n"
//3:
	"	movq	%%mm7,	%%mm6\n"
	"	movq	%%mm7,	%%mm1\n"
	"	psllq	$16,	%%mm7\n"

	"	pand	%%mm0,	%%mm6\n"
	"	pand	%%mm0,	%%mm1\n"
	"	psrlq	$16,	%%mm6\n"
	"	por	%%mm6,	%%mm1\n"
	"	psrlq	$16,	%%mm6\n"
	"	por	%%mm6,	%%mm1\n"

	"	pmullw	%%mm1,	%%mm4\n"
//4:
	"	movq	%%mm7,	%%mm6\n"
	"	movq	%%mm7,	%%mm1\n"
	"	pxor	%%mm7,	%%mm7\n"

	"	pand	%%mm0,	%%mm6\n"
	"	pand	%%mm0,	%%mm1\n"
	"	psrlq	$16,	%%mm6\n"
	"	psrlq	$48,	%%mm1\n"
	"	por	%%mm6,	%%mm1\n"
	"	psrlq	$16,	%%mm6\n"
	"	por	%%mm6,	%%mm1\n"

	"	pmullw	%%mm1,	%%mm5\n"

	"	paddusw	%%mm2,	%%mm3\n"
	"	paddusw	%%mm4,	%%mm5\n"
	"	paddusw	%%mm3,	%%mm5\n"
	"	psrlw	$8,	%%mm5\n"

	"	packuswb	%%mm7,	%%mm5\n"
	"	movd	%%mm5,	%0\n"

	"	emms\n"
	"   pop %%ebx\n"
	:"=m"(rez)
	:"m"(x0y0),"m"(x1y0),"m"(x0y1),"m"(x1y1),"m"(Fx),"m"(Fy)
	:"memory", "eax", "ecx", "edx"
	);
#endif // bilinea_p5_asm

#ifdef common_asm
/* THIS VERSION IS DUE TO barney. */

/*
(
|------|------| ^
|      |      | |
| x0y0 | x1y0 | |
|      |      | |
|------|------| |  y
|      |      | |
| x0y1 | x1y1 | |
|      |      | |
|------|------| V

<------------->
       x

The Formula is identical to:
result = MultiplyColor(x0y0, (1-x)*(1-y)) + MultiplyColor(x0y0, (  x)*(1-y)) +
	 MultiplyColor(x0y1, (1-x)*(  y)) + MultiplyColor(x1y1, (  x)*(  y));
)
Requires SSE. Very fast :)
*/

Uint32 bilinea4(Uint32 x0y0, Uint32 x1y0, Uint32 x0y1, Uint32 x1y1, int x, int y)
{	Uint32 xx_result;
//    __m64 mm;
	__asm (
	" 	pxor		%%mm6,	%%mm6		\n"
	"	movd		%1,	%%mm0		\n"
	"	movl		$65535,	%%eax		\n"
	"	movl		%6,	%%edi		\n"
	"	movd		%2,	%%mm1		\n"
	"	movl		$65535,	%%ecx		\n"
	"	subl		%%edi,	%%ecx		\n"
	"	movd		%3,	%%mm2		\n"
	"	shll		$16,	%%edi		\n"
	"	movl		%5,	%%esi		\n"
	"	movd		%4,	%%mm3		\n"
	"	subl		%%esi,	%%eax		\n"
	"	shll		$16,	%%esi		\n"
	"	punpcklbw	%%mm6,	%%mm0		\n"
	"	orl		%%edi,	%%ecx		\n"
	"	orl		%%esi,	%%eax		\n"
	"	punpcklbw	%%mm6,	%%mm1		\n"
	"	movd		%%eax,	%%mm5		\n"
	"	movd		%%ecx,	%%mm7		\n"
	"	punpcklbw	%%mm6,	%%mm2		\n"
	"	punpcklbw	%%mm6,	%%mm3		\n"
	"	pshufw		$80,	%%mm7,	%%mm7	\n"
	"	pshufw		$68,	%%mm5,	%%mm5	\n"
	"	pmulhuw		%%mm5,	%%mm7		\n"
	"	pshufw		$0,	%%mm7,	%%mm4	\n"
	"	pshufw		$85,	%%mm7,	%%mm5	\n"
	"	pmulhuw		%%mm4,	%%mm0		\n"
	"	pshufw		$170,	%%mm7,	%%mm6	\n"
	"	pmulhuw		%%mm5,	%%mm1		\n"
	"	pshufw		$255,	%%mm7,	%%mm7	\n"
	"	pmulhuw		%%mm6,	%%mm2		\n"
	"	paddw		%%mm1,	%%mm0		\n"
	"	pmulhuw		%%mm7,	%%mm3		\n"
	"	paddw		%%mm3,	%%mm2		\n"
	"	paddw		%%mm2,	%%mm0		\n"
	"	packuswb	%%mm0,	%%mm0		\n"
	"	movd		%%mm0,	%0		\n"
	"	emms					\n"
	 :"=m"(xx_result)
	 :"m"(x0y0), "m"(x1y0), "m"(x0y1), "m"(x1y1), "m"(x), "m"(y)
	 :"memory", "eax", "edi", "ecx", "esi"
	);
	return xx_result;
}
#endif

#ifdef blur_asm

/*********************************************************
 * MMX blur routines:                                    *
 * blur_forward_mmx, blur_backward_mmx, buffer_plus_mmx, *
 * and buffer_minus_mmx works just like their non-mmx    *
 * counterparts, except for saturation arithmetic        *
 *********************************************************/

// note: count must be divisable by 8
void blur_forward_mmx(Uint32 *dest, Uint32 *src, int count)
{
	int andval[6] __attribute__((aligned(8))) = {
			0x000000ff, 0x000000ff,
			0x0000ff00, 0x0000ff00,
			0x00f80000, 0x00f80000
	};
	// load this useful value:
	__asm __volatile(
	"	movq	8%0,	%%mm6\n"
	"	movq	16%0,	%%mm7\n"
	::"m"(*andval):"memory");
	count /= 8; // we are doing 8 pixels per cycle :)
	while (count--) {
		__asm __volatile(

		"	movq	%1,	%%mm0\n"
		"	movq	8%1,	%%mm3\n"
		"	movq	%%mm6,	%%mm4\n"
		"	movq	%%mm7,	%%mm5\n"
		"	movq	%%mm0,	%%mm1\n"
		"	movq	%%mm0,	%%mm2\n"

		"	pand	%2,	%%mm0\n"
		"	pand	%%mm3,	%%mm4\n"
		"	pand	%%mm3,	%%mm5\n"
		"	pand	%2,	%%mm3\n"
		"	pand	%%mm6,	%%mm1\n"
		"	pand	%%mm7,	%%mm2\n"

		"	pslld	$3,	%%mm4\n"
		"	pslld	$5,	%%mm5\n"

		"	pslld	$3,	%%mm1\n"
		"	pslld	$5,	%%mm2\n"

		"	por	%%mm5,	%%mm4\n"
		"	movq	24%1,	%%mm5\n"
		"	por	%%mm2,	%%mm1\n"
		"	movq	16%1,	%%mm2\n"
/* REGISTER ALLOCATION:
	mm0	- blue  of point0 + orig of point0
	mm1	- R+G   of point0
	mm2	- third point
	mm3	- blue  of point1 + orig of point1
	mm4	- R+G   of point1
	mm5	- fourth point
*/
		"	paddd	%%mm4,	%%mm3\n"
		"	movq	%%mm7,	%%mm4\n"
		"	paddd	%%mm1,	%%mm0\n"
		"	movq	%%mm7,	%%mm1\n"
		"	movq	%%mm3,	8%0\n"
		"	movq	%%mm0,	%0\n"
// 1 & 2 done;

		"	pand	%%mm5,	%%mm4\n"
		"	movq	%%mm5,	%%mm3\n"
		"	pand	%2,	%%mm5\n"
		"	movq	%%mm2,	%%mm0\n"
		"	pand	%%mm2,	%%mm1\n"
		"	pand	%2,	%%mm2\n"
		"	pand	%%mm6,	%%mm3\n"
		"	pand	%%mm6,	%%mm0\n"
		"	pslld	$5,	%%mm4\n"
		"	pslld	$3,	%%mm3\n"
		"	pslld	$5,	%%mm1\n"
		"	pslld	$3,	%%mm0\n"
		"	paddd	%%mm4,	%%mm3\n"
		"	paddd	%%mm1,	%%mm0\n"
		"	paddd	%%mm3,	%%mm5\n"
		"	paddd	%%mm0,	%%mm2\n"
		"	movq	%%mm5,	24%0\n"
		"	movq	%%mm2,	16%0\n"


		:"=m"(*dest)
		:"m"(*src), "m"(*andval)
		:"memory");
		src+=8; dest+=8;
	}
	__asm __volatile("emms");
}

// note: count must be divisable by 2
void blur_backward_mmx(Uint32 *dest, Uint32 *src, int count)
{
	int shifted_8bits[6] __attribute__((aligned(8))) = {
			0x000007f8, 0x000007f8,
			0x003fc000, 0x003fc000,
			0xff000000, 0xff000000
	};
	count /= 2;
	__asm __volatile(
		"	pxor	%%mm4,	%%mm4\n"
		"	movq	%0,	%%mm5\n"
		"	movq	8%0,	%%mm6\n"
		"	movq	16%0,	%%mm7\n"
		::"m"(*shifted_8bits):"memory");


	while (count--) {
		__asm __volatile(

		"	movq	%1,	%%mm0\n"
		"	movq	%%mm6,	%%mm1\n"
		"	movq	%%mm7,	%%mm2\n"
		"	pand	%%mm0,	%%mm1\n"
		"	pand	%%mm0,	%%mm2\n"
		"	pand	%%mm5,	%%mm0\n"

		"	psrld	$6,	%%mm1\n"
		"	psrld	$8,	%%mm2\n"
		"	psrld	$3,	%%mm0\n"
		"	por	%%mm1,	%%mm0\n"
		"	por	%%mm2,	%%mm0\n"

		"	movq	%%mm0,	%0\n"

		:"=m"(*dest)
		:"m"(*src)
		:"memory");
		dest+=2; src+=2;
	}
	__asm __volatile("emms");
}

// note: count must be divisable by 8
void buffer_minus_mmx(Uint32 *dest, Uint32 *src, int count)
{
	count /= 8;
	do {
		__asm __volatile(
		"	movq	  %0,	%%mm0\n"
		"	movq	 8%0,	%%mm1\n"
		"	movq	16%0,	%%mm2\n"
		"	movq	24%0,	%%mm3\n"
		"	psubd	  %1,	%%mm0\n"
		"	psubd	 8%1,	%%mm1\n"
		"	psubd	16%1,	%%mm2\n"
		"	psubd	24%1,	%%mm3\n"
		"	movq	%%mm0,	  %0\n"
		"	movq	%%mm1,	 8%0\n"
		"	movq	%%mm2,	16%0\n"
		"	movq	%%mm3,	24%0\n"

		:"=m"(*dest)
		:"m"(*src)
		:"memory"
		);
		dest += 8; src += 8;
	} while (--count);
	__asm __volatile("emms");
}

// note: count must be divisable by 8
void buffer_plus_mmx(Uint32 *dest, Uint32 *src, int count)
{
	count /= 8;
	do {
		__asm __volatile(
		"	movq	  %0,	%%mm0\n"
		"	movq	 8%0,	%%mm1\n"
		"	movq	16%0,	%%mm2\n"
		"	movq	24%0,	%%mm3\n"
		"	paddd	  %1,	%%mm0\n"
		"	paddd	 8%1,	%%mm1\n"
		"	paddd	16%1,	%%mm2\n"
		"	paddd	24%1,	%%mm3\n"
		"	movq	%%mm0,	  %0\n"
		"	movq	%%mm1,	 8%0\n"
		"	movq	%%mm2,	16%0\n"
		"	movq	%%mm3,	24%0\n"

		:"=m"(*dest)
		:"m"(*src)
		:"memory"
		);
		dest += 8; src += 8;
	} while (--count);
	__asm __volatile("emms");
}

#endif

#ifdef fract_asm
// contains some of fract.cpp's source code (merely all that is in assembly)

float w2_22 = BRIGHTNESS_C;
float w2_shl16 = 65536.0;
float w2_22_16=w2_22*65536.0;
float w2_22_8 =w2_22*256.0;

// this is the integrated version of multiplycolor and lform2 - does the whole thing in a single routine
// input:  raw color from the texture. The x and y coord of the destination (where the ray hit the floor/ceiling).
//         The Lx, Ly coordinates of the light source
// output: the calculated color that should be printed to the screen
// The routine requires SSE instruction set. Supported CPUs: Pentium III and above (Intel); Athlon XP/Duron Morgan or above (AMD)
// This one uses some ideas and optimizations by barnie.

static inline Uint32	igrated(Uint32 color, int x, int y, int lx, int ly, int ysq)
{
	register Uint32 rezult asm ("eax"); // the assembly routine calculates the result in the eax register

 __asm __volatile(
  // perform the light degradation math -> color multiplyer = const * ((x-lx)^2+(y-ly)^2)^-0.25
 "	cvtsi2ss %6, 	%%xmm4	\n"
 "	subl	%1,	%0	\n"
 "	subl	%3,	%1	\n"

 "	movss	%4,	%%xmm2	\n" // move the constant to xmm2
 "	cvtsi2ss %0,	%%xmm0	\n" // ...try to exploit parallelism
 "	cvtsi2ss %1,	%%xmm1	\n"
 "	mulss	%%xmm0,	%%xmm0	\n"
 "	mulss	%%xmm1,	%%xmm1	\n"
 "	addps	%%xmm0,	%%xmm4	\n" // xmm0 = dist^2
 "	addps	%%xmm1,	%%xmm4	\n"

// the following is the barnie's idea of obtaining  const / dist^0.5
 "	rsqrtss	%%xmm4,	%%xmm4	\n" // xmm0 = 1/dist
 "	mulss	%%xmm4,	%%xmm2	\n" // xmm2 = const / dist
 "	rsqrtss	%%xmm4,	%%xmm4	\n" // xmm0 = sqrt(dist)
 "	mulss	%%xmm4,	%%xmm2	\n" // xmm2 = const / sqrt(dist)
 //	load color in mm0: (via mmx):

 // note: we switch to MMX here:
 "	pxor	%%mm0,	%%mm0	\n"
 "	movd	%5,	%%mm1	\n"
 "	punpcklbw	%%mm1,	%%mm0	\n"

 "	shufps	$0,	%%xmm2,	%%xmm2	\n"
 "	cvtps2pi	%%xmm2, %%mm2	\n"
 "	pxor 	%%mm4,	%%mm4	\n"
 "	movq	%%mm2,	%%mm3	\n"
 "	packssdw	%%mm3,	%%mm2	\n"

 "	pmulhuw	%%mm2,	%%mm0	\n"

 "	packuswb	%%mm4,	%%mm0	\n"
 "	movd	%%mm0,	%%eax	\n"
 "	emms	\n"
 :
 :"r"(x), "r"(y), "r"(lx), "r"(ly), "m"(w2_22_8), "m"(color), "m"(ysq)
 :"memory", "eax"
 );
 return rezult; // the assembly equivallent to this is "do nothing",
 // since Uint32 results are returned in eax and the result is already there.
}
#endif


#ifdef genrender_asm
/*********************************************************************
 *  MMX2 version of the merging function, which works on a single row*
 *-------------------------------------------------------------------*
 * INPUT:  `flr' is the pixel buffer from the floor engine           *
 *         `sph' is the pixel buffer from the sphere engine          *
 *         `multi' is an array of multipliers in 16.16 fixedpoint    *
 *             format. A value of 16384 for example, means, to blend *
 *             1/4 of the sphere pixel with 3/4 of the floor pixel   *
 *         `count' : how many pixels, must be divisable by 2.        *
 *********************************************************************/
	const Uint16 ones[4] __attribute__ ((aligned (8))) = {0xffff, 0xffff, 0xffff, 0xffff};
	const Uint16  bla[4] __attribute__ ((aligned (8))) = {0x0001, 0x0001, 0x0001, 0x0001};

void merge_rows(Uint32 *flr, Uint32 *sph, unsigned short *multi, int count)
{
	// load one useful value:
	__asm __volatile ("	movq	%0,	%%mm7\n"::"m"(*ones):"memory");
	count /= 2;
	while (count--) {
		__asm __volatile (
		"	pxor	%%mm6,	%%mm6\n"
		"	movq	%0,	%%mm0\n" // mm0 = color from floor (2 pixels)
		"	movq	%1,	%%mm2\n" // mm2 = color from sphere (2 pixels)
		"	movd	%2,	%%mm4\n" // low mm4 = alpha (16bit) (2 pixels)
		"	movq	%%mm0,	%%mm1\n"
		"	movq	%%mm2,	%%mm3\n"
		"	punpcklbw %%mm6, %%mm0\n"
		"	punpckhbw %%mm6, %%mm1\n"
		"	punpcklbw %%mm6, %%mm2\n"
		"	punpckhbw %%mm6, %%mm3\n"
// mm0, mm1 - unpacked pixels from floor engine
// mm2, mm3 - unpacked pixels from sphere engine
// mm4 - packed multipliers

		"	movq	%%mm7,	%%mm6\n"
		"	pshufw	$0,	%%mm4,	%%mm5\n"
		"	psubw	%%mm5,	%%mm6\n"
		"	pmulhuw	%%mm5,	%%mm2\n"
		"	pmulhuw	%%mm6,	%%mm0\n"
		"	pshufw	$0x55,	%%mm4,	%%mm5\n"
		"	movq	%%mm7,	%%mm6\n"
		"	pmulhuw	%%mm5,	%%mm3\n"
		"	paddusw	%3,	%%mm0\n"
		"	psubw	%%mm5,	%%mm6\n"
		"	pmulhuw	%%mm6,	%%mm1\n"
		"	paddusw	%3,	%%mm3\n"
		"	paddusw	%%mm2,	%%mm0\n"
		"	paddusw	%%mm3,	%%mm1\n"
		"	packuswb %%mm7,	%%mm0\n"
		"	packuswb %%mm7,	%%mm1\n"
		"	movd	%%mm0,	%0\n"
		"	movd	%%mm1,	4%0\n"

		:"=m"(*flr)
		:"m"(*sph),"m"(*multi), "m"(*bla)
		);
		flr+=2;
		sph+=2;
		multi+=2;
	}
	__asm __volatile ("emms");
}
#endif

#ifdef shadows_related
/***********************************************************************
	shadows_merge_mmx2: MMX2 version of the shadow merging routine

	multiplies every pixel of dst by the inverted low byte of the
	src "shadow-buffer" array.

	the formula is:
	for (i=0;i<xr*yr;i++)
		floorbuffer[i] = multiplycolor(floorbuffer[i], 255 * (255 - (255&sbuffer[i])));

	note: count must be divisable by 4!
 ***********************************************************************/

void __attribute__((noinline)) shadows_merge_mmx2(Uint32 *dst, Uint16 *src, int count)
{
	unsigned short bandmask[4] = {255, 255, 255, 255};
	count /= 4;
	__asm __volatile (

	"	mov	%2,	%%ecx\n"
	"	mov	%0,	%%edi\n"
	"	mov	%1,	%%esi\n"

	"	movq	%3,	%%mm6\n"
	"	pxor	%%mm7,	%%mm7\n"

	XALIGN
	"s_merge_loop:	\n"

	"	movq	%%mm6,		%%mm5\n"
	"	pshufw	$0xe4,	%%mm6,	%%mm4\n" // essentialy, movq %%mm6, %%mm4

	"	movq	(%%edi),	%%mm0\n"
	"	movq	8(%%edi),	%%mm2\n"
	"	pand	(%%esi),	%%mm5\n"

	"	pshufw	$0xee,	%%mm0,	%%mm1\n"
	"	pshufw	$0xee,	%%mm2,	%%mm3\n"

	"	psubw	%%mm5,		%%mm4\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"

	"	pshufw	$0x00,	%%mm4,	%%mm5\n"

	"	punpcklbw	%%mm7,	%%mm2\n"
	"	punpcklbw	%%mm7,	%%mm3\n"

	"	pmullw	%%mm5,		%%mm0\n"
	"	pshufw	$0x55,	%%mm4,	%%mm5\n"

	"	pmullw	%%mm5,		%%mm1\n"

	"	pshufw	$0xaa,	%%mm4,	%%mm5\n"
	"	pshufw	$0xff,	%%mm4,	%%mm4\n"
	"	pmullw	%%mm5,		%%mm2\n"
	"	pmullw	%%mm4,		%%mm3\n"

	"	psrlw	$8,		%%mm0\n"
	"	psrlw	$8,		%%mm1\n"
	"	psrlw	$8,		%%mm2\n"
	"	psrlw	$8,		%%mm3\n"

	"	packuswb	%%mm1,	%%mm0\n"
	"	packuswb	%%mm3,	%%mm2\n"

	"	movq	%%mm0,		(%%edi)\n"
	"	movq	%%mm2,		8(%%edi)\n"

	"	add	$8,		%%esi\n"
	"	add	$16,		%%edi\n"

	"	dec			%%ecx\n"
	"	jnz	s_merge_loop\n"

	"	emms\n"


	::"m"(dst),"m"(src),"m"(count), "m"(*bandmask)
	:"memory", "ecx", "esi", "edi"
	);
}

/*
	Perform triangle shadow and sphere shadow buffers merge
	The `dst` buffer is in 16bit format with lower 7 bits
	designating the amount of shadow.
	The `src' buffer is in 8bit format with lower 7 bits
	designating the amount of shadow.
*/
void triangle_sp_merge(Uint16 *dst, Uint8 *src, int count)
{
	for (int i = 0; i < count; i++) {
		dst[i] |= src[i];
	}
}

void fast_line_fill(Uint16 *p, int size, Uint16 fill)
{
	SSE_ALIGN(Uint16 bigfill[4]) = {fill, fill, fill, fill};
	int k = size / 4;
	__asm __volatile("movq	%0,	%%mm7\n"
	: 
	:"m"(*bigfill)
	:"memory"
			);
	while (k--) {
		__asm __volatile(
				"	movq	%0,	%%mm0\n"
				"	por	%%mm7,	%%mm0\n"
				"	movq	%%mm0,	%0\n"
			:"=m"(*p)
			::"memory"
				);
		p += 4;
	}
	__asm __volatile ("emms");
	size %= 4;
	for (int i = 0; i < size; i++)
		p[i] |= fill;
}

static void fast_reblend_mmx(int x1, int y1, int x2, int y2, Uint16 *sbuff, int xr, Uint16 sintensity, int cpui, int cpuc)
{
	sintensity = 255 - sintensity;
	SSE_ALIGN(Uint16 x_mor[4]) = { 0xff, 0xff, 0xff, 0xff} ;
	SSE_ALIGN(Uint16 x_add[4]) = { sintensity, sintensity, sintensity, sintensity };
	x2 = ((x2/4)+1)*4;
	x1 &= ~3;
	int xsize = (x2 - x1)/4;
	int ysize = y2 - y1+1;
	if (xsize <= 0 || ysize <= 0) return;
	Uint16 *p = &sbuff[y1 * xr + x1];
	xr *= 2;
	
	cpui = ((y1 % cpuc) + cpuc - cpui) % cpuc + 1;
	//
	__asm __volatile (// 0 - p, 1 - ysize, 2 - xsize, 3 - xr
			"	mov	%0,	%%esi\n"
			"	mov	%1,	%%edx\n"
			"	mov	%7,	%%ecx\n"
			"	movq	%4,	%%mm7\n"
			"	movq	%5,	%%mm6\n"
			
			".balign	16\n"
			"yyloop:\n"
			"	mov	%%esi,	%%edi\n"
			"	add	%3,	%%esi\n"
			
			"	dec	%%ecx\n"
			"	jnz	skip_row\n"
			"	mov	%6,	%%ecx\n"
			"	mov	%2,	%%eax\n"
			
			".balign	16\n"
			"xxloop:\n"
			"	movq	(%%edi),	%%mm0\n"
			"	movq	%%mm0,	%%mm1\n"
			"	pand	%%mm7,	%%mm0\n"
			"	psrlw	$8,	%%mm1\n"
			"	paddusb	%%mm1,	%%mm0\n"
			"	paddusb	%%mm6,	%%mm0\n"
			"	psubusb	%%mm6,	%%mm0\n"
			"	movq	%%mm0,	(%%edi)\n"
			
			"	add	$8,	%%edi\n"
			"	dec	%%eax\n"
			"	jnz	xxloop\n"
			
			".balign	16\n"
			"skip_row:\n"
			"	dec	%%edx\n"
			"	jnz	yyloop\n"
			
			"	emms\n"
	::"m"(p), "m"(ysize), "m"(xsize), "m"(xr), "m"(*x_mor), "m"(*x_add), "m"(cpuc), "m"(cpui)
	:"memory", "eax", "edx", "esi", "edi");
}

#endif

#ifdef rgb2yuv_asm

// ConvertRGB2YUV_ASM - convers [count] pixels from [*src] to [*dest]
// transformation is FROM RGB colorspace TO YUY2 packed format
// -- Assembly version of ConvertRGB2YUV_X86. Uses i386 intructions only. No precision loss also.
// count must be even
void ConvertRGB2YUV_X86_ASM(Uint32 *dest, Uint32 *src, size_t count)
{
#ifdef __APPLE__
	ConvertRGB2YUV_X86(dest, src, count);
	return;
#endif
 int s[9] = {IM11, IM12, IM13, IM21, IM22, IM23, IM31, IM32, IM33};
 count /= 2;
 while (count--) {
 	__asm __volatile(
	//  Y0 ::
	"mov		%1,		%%ecx\n"
	"movzbl		%%cl,		%%eax\n"
	"imull		8%2\n"
	"xchg		%%eax,		%%esi\n"
	"movzbl		%%ch,		%%eax\n"
	"ror		$16,		%%ecx\n"
	"imull		4%2\n"
	"add		%%eax,		%%esi\n"
	"movzbl		%%cl,		%%eax\n"
	"ror		$16,		%%ecx\n"
	"add		$0x100000,	%%esi\n"
	"imull		%2\n"
	"add		%%esi,		%%eax\n"
	"shr		$16,		%%eax\n"
	"and		$0xff,		%%eax\n"
	"mov		%%eax,		%0\n"
	//  U0 ::
	"movzbl		%%cl,		%%eax\n"
	"imull		32%2\n"
	"xchg		%%eax,		%%esi\n"
	"movzbl		%%ch,		%%eax\n"
	"ror		$16,		%%ecx\n"
	"imull		28%2\n"
	"add		%%eax,		%%esi\n"
	"movzbl		%%cl,		%%eax\n"
	"ror		$16,		%%ecx\n"
	"add		$0x800000,	%%esi\n"
	"imull		24%2\n"
	"add		%%esi,		%%eax\n"
	"shr		$8,		%%eax\n"
	"and		$0xff00,	%%eax\n"
	"or 		%%eax,		%0\n"
	//  V0 ::
	"movzbl		%%cl,		%%eax\n"
	"imull		20%2\n"
	"xchg		%%eax,		%%esi\n"
	"movzbl		%%ch,		%%eax\n"
	"ror		$16,		%%ecx\n"
	"imull		16%2\n"
	"add		%%eax,		%%esi\n"
	"movzbl		%%cl,		%%eax\n"
	"ror		$16,		%%ecx\n"
	"add		$0x800000,	%%esi\n"
	"imull		12%2\n"
	"add		%%esi,		%%eax\n"
	"shl		$8,		%%eax\n"
	"and		$0xff000000,	%%eax\n"
	"or 		%%eax,		%0\n"
	//  Y1 ::
	"mov		4%1,		%%ecx\n"
	"movzbl		%%cl,		%%eax\n"
	"imull		8%2\n"
	"xchg		%%eax,		%%esi\n"
	"movzbl		%%ch,		%%eax\n"
	"ror		$16,		%%ecx\n"
	"imull		4%2\n"
	"add		%%eax,		%%esi\n"
	"movzbl		%%cl,		%%eax\n"
	"ror		$16,		%%ecx\n"
	"add		$0x100000,	%%esi\n"
	"imull		%2\n"
	"add		%%esi,		%%eax\n"
	"and		$0xff0000,	%%eax\n"
	"or 		%%eax,		%0\n"
	:"=m"(*dest)
	:"m"(*src), "m"(*s)
	:"memory", "eax", "ecx", "edx", "esi");
	dest++;
	src+=2;
 	}
}

// ConvertRGB2YUV_X86_FPU - convers [count] pixels from [*src] to [*dest] using floating point math
// transformation is FROM RGB colorspace TO YUY2 packed format
// count must be even.
void ConvertRGB2YUV_X86_FPU(Uint32 *dest, Uint32 *src, size_t count)
{
#ifdef __APPLE__
	ConvertRGB2YUV_X86(dest, src, count);
	return;
#endif
	//float M[9] __attribute__ ((aligned(16))) = {M11, M12, M13, M21, M22, M23, M31, M32, M33};
	float M[9] __attribute__ ((aligned(16))) = {M13, M33, M23, M11, M31, M21, M12, M32, M22};
	int   a[2] __attribute__ ((aligned(16))) = {16, 128};
	Uint16 cwsave, mycw;

	count /= 2;
	// lower FPU precision to single:
	__asm __volatile("	fstcw	%0\n":"=m"(cwsave)::"memory"); // get conrol word
	mycw = cwsave & (~0x0300); // set single precision
	__asm __volatile("	fldcw	%0\n"::"m"(mycw):"memory"); // put control word
	while (count--) {
		__asm __volatile (

//get B0, R0, G0:
		"movl	%1,		%%eax\n"
		"andl	$0xff00ff,	%1\n"
		"mov	$0x00ff00,	%%edx\n"
		"filds	%1\n"
		"and	%%eax,	%%edx\n"
		"shrl	$16,	%1\n"
		"shrl	$8,	%%edx\n"
		"filds	%1\n"
		"mov	%%edx,	%1\n"
		"filds	%1\n"
// st(0) = g0; st(1) = r0; st(2) = b0

// results will be: st -> V0, st(1) -> U0, st(2) -> Y0

// B0:
		"fld	%%st(2)\n"
		"fmul	%2\n"
		"fld	%%st(3)\n"
		"fmul	4%2\n"
		"fld	%%st(4)\n"
		"ffree	%%st(5)\n"
		"fmul	8%2\n"

// R0:
		"fld	%%st(4)\n"
		"fmul	12%2\n"
		"fld	%%st(5)\n"
		"fmul	16%2\n"
		"fld	%%st(6)\n"
		"fmul	20%2\n"
		"ffree	%%st(7)\n"

		"faddp	%%st,	%%st(3)\n"
		"faddp	%%st,	%%st(3)\n"
		"faddp	%%st,	%%st(3)\n"
// G0:
		"fld	%%st(3)\n"
		"fmul	24%2\n"
		"fld	%%st(4)\n"
		"fmul	28%2\n"
		"fld	%%st(5)\n"
		"fmul	32%2\n"
		"ffree	%%st(6)\n"

		"faddp	%%st,	%%st(3)\n"
		"faddp	%%st,	%%st(3)\n"
		"faddp	%%st,	%%st(3)\n"

		"fiaddl 4%3\n"

// ST(0) = V0; ST(1) = U0; ST(2) = Y0
		"fistps	%0\n"
		"fiaddl	4%3\n"
		"fistps	%1\n"
		"fiaddl	%3\n"
		"shll	$24,	%0\n"

		"fistps	%0\n"
		"movzx	%1,	%%edx\n"
		"shl	$8,	%%edx\n"
		"orl	%%edx,	%0\n"

// %0 = V0 00 U0 Y0
		"movl	%%eax,	%1\n"

		"fildl	%3\n"
		"flds	24%2\n"
		"flds	12%2\n"
		"flds	%2\n"

		"movl	4%1,	%%eax\n"
		"movl	$0x00ff00,	%%edx\n"
		"andl	$0xff00ff,	4%1\n"
		"andl	%%eax,	%%edx\n"
		"fimuls 4%1\n"
		"shrl	$16,	4%1\n"
		"faddp	%%st,	%%st(3)\n"
		"shrl	$8,	%%edx\n"
		"fimuls	4%1\n"
		"faddp	%%st,	%%st(2)\n"
		"movl	%%edx,	4%1\n"
		"fimuls 4%1\n"

		"faddp\n"

		"fistpl	4%1\n"
		"movl	4%1,	%%edx\n"
		"shll	$16,	%%edx\n"
		"orl	%%edx,	%0\n"
		"movl	%%eax,	4%1\n"

		:"=m"(*dest), "=m"(*src)
		:"m"(*M), "m"(*a)
		:"memory", "eax", "edx"
		);
		dest++; src+=2;
	}
	__asm __volatile ("	fldcw	%0\n"::"m"(cwsave):"memory");
}

// ConvertRGB2YUV_MMX - convers [count] pixels from [*src] to [*dest] using MMX
// transformation is FROM RGB colorspace TO YUY2 packed format
// uses tightly optimized (pairable) MMX code
// should work significantly faster on PIII & P4. (And of course AthlonXP). It's not such a big deal on a PI and PII
// there is a small precision cost, which should be unnoticable
// count must be even.

void ConvertRGB2YUV_MMX(Uint32 *dest, Uint32 *src, size_t count)
{Sint16 pmm3[4]={SIM13, SIM33, SIM13, SIM23};
 Sint16 pmm4[4]={SIM12, SIM32, SIM12, SIM22};
 Sint16 pmm5[4]={SIM11, SIM31, SIM11, SIM21};
 Uint16 pmm6[4]={4096, 32768, 4096, 32768};
 Sint16 pmm7[4]={0, 0, 0, 0};


 count /= 2;
 // load some useful values
 __asm __volatile(
 "movq %0, %%mm3\n"
 "movq %1, %%mm4\n"
 "movq %2, %%mm5\n"
 "movq %3, %%mm7\n"
 "movq %4, %%mm6\n"
 ::"m"(*pmm3), "m"(*pmm4), "m"(*pmm5), "m"(*pmm7), "m"(*pmm6):"memory"
 );
 while (count--) {
	__asm __volatile(
	// get the current pixel in ecx, the next in edx
	"mov	%1, 	%%ecx\n"
	"mov	4%1,	%%edx\n"
	// prepare mm0 - fill with (b0, b0, b1, b0)
	"movb	%%cl,	%%ah\n"
	"movb	%%dl, 	%%al\n"
	"shl	$16,	%%eax\n"
	"movb	%%cl,	%%al\n"
	"movb	%%cl,	%%ah\n"
	"movd	%%eax,	%%mm0\n"

	"shr	$8,	%%ecx\n"
	"shr	$8,	%%edx\n"

	"punpcklbw 	%%mm7,	%%mm0\n"
	// prepare mm1 - fill with (g0, g0, g1, g0)
	"movb	%%cl,	%%ah\n"
	"movb	%%dl, 	%%al\n"
	"shl	$16,	%%eax\n"
	"movb	%%cl,	%%al\n"
	"movb	%%cl,	%%ah\n"
	"movd	%%eax,	%%mm1\n"

	"shr	$8,	%%ecx\n"
	"shr	$8,	%%edx\n"

	"punpcklbw 	%%mm7,	%%mm1\n"
	// prepare mm2 - fill with (r0, r0, r1, r0)
	"movb	%%cl,	%%ah\n"
	"movb	%%dl, 	%%al\n"
	"shl	$16,	%%eax\n"
	"movb	%%cl,	%%al\n"
	"movb	%%cl,	%%ah\n"
	"movd	%%eax,	%%mm2\n"

	// do the actual math:

	"punpcklbw 	%%mm7,	%%mm2\n"

	"pmullw 	%%mm3,	%%mm0\n"
	"pmullw 	%%mm4,	%%mm1\n"
	"pmullw 	%%mm5,	%%mm2\n"

	"paddw		%%mm1,	%%mm0\n"
	"paddw		%%mm6,	%%mm2\n"
	"paddw		%%mm2,	%%mm0\n"
	"psrlw		$8,	%%mm0\n"

	"packuswb	%%mm7,	%%mm0\n"

	"movd		%%mm0,	%0\n"
	:"=m"(*dest)
	:"m"(*src)
	:"memory", "eax", "ecx", "edx"
	);
 	dest++;
	src+=2;
 	}
 __asm __volatile("emms");
}

// ConvertRGB2YUV_MMX2 - convers [count] pixels from [*src] to [*dest] using MMX2
// transformation is FROM RGB colorspace TO YUY2 packed format
// uses tightly optimized (pairable) MMX code
// count must be even.

void ConvertRGB2YUV_MMX2(Uint32 *dest, Uint32 *src, size_t count)
{
 SSE_ALIGN(Sint16 pmm3[4])={SIM13, SIM33, SIM13, SIM23};
 SSE_ALIGN(Sint16 pmm4[4])={SIM12, SIM32, SIM12, SIM22};
 SSE_ALIGN(Sint16 pmm5[4])={SIM11, SIM31, SIM11, SIM21};
 SSE_ALIGN(Uint16 pmm6[4])={4096, 32768, 4096, 32768};
 SSE_ALIGN(Uint16 pmm7[4])={0xffff, 0x00ff, 0xffff, 0x00ff};
 //Sint16 pmm7[4]={0, 0, 0, 0};

 count /= 2;

 // load some constants:
 __asm __volatile (
 "	pxor	%%mm7,	%%mm7\n"
 "	movq	%0,	%%mm6\n"
 ::"m"(*pmm7):"memory"
 );

 while (count--) {
 	__asm __volatile (
	"	movq	%1,	%%mm0\n"
	"	pand	%%mm6,	%%mm0\n"
	"	pshufw	$0x0e,	%%mm0,	%%mm1\n"
// mm0 contains pixel 1 and pixel 2
// low dword of mm1 contains pixel 2

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"

// mm0 = (b0 g0 r0 00) (words)
// mm1 = (b1 g1 r1 00) (words)

// SOME REAALY WEIRD SHUFFLES::: :)
	"	pshufw	$0x30,	%%mm0,	%%mm2\n" // 00110000
	"	pshufw	$0xcf,	%%mm1,	%%mm3\n" // 11001111
	"	pshufw	$0x75,	%%mm0,	%%mm4\n" // 01110101
	"	pshufw	$0xdf,	%%mm1,	%%mm5\n" // 11011111
	"	pshufw	$0xba,	%%mm0,	%%mm0\n" // 10111010
	"	pshufw	$0xef,	%%mm1,	%%mm1\n" // 11101111
// ends up like this:
	"	por	%%mm3,	%%mm2\n" // mm2 - b0 b0 b1 b0
	"	por	%%mm5,	%%mm4\n" // mm4 - g0 g0 g1 g0
	"	por	%%mm1,	%%mm0\n" // mm0 - r0 r0 r1 r0

	"	pmullw	%2,	%%mm2\n" // mm2 *= M13 M33 M13 M23
	"	pmullw	%3,	%%mm4\n" // mm4 *= M12 M32 M12 M22
	"	pmullw	%4,	%%mm0\n" // mm0 *= M11 M31 M11 M21

	"	paddw	%5,	%%mm2\n" // mm2 += 16  128  16 128
	"	paddw	%%mm4,	%%mm0\n"
	"	paddw	%%mm2,	%%mm0\n"
	"	psrlw	$8,	%%mm0\n" // mm0 = Y0 U0 Y1 V0

	"	packuswb	%%mm7,	%%mm0\n"
	"	movd	%%mm0,	%0\n"


	:"=m"(*dest)
	:"m"(*src), "m"(*pmm3), "m"(*pmm4), "m"(*pmm5), "m"(*pmm6)
	:"memory"
	);
 	src+=2;
	dest++;
 }

 __asm __volatile("emms");
}


// ConvertRGB2YUV_SSE - convers [count] pixels from [*src] to [*dest] using MMX2 and SSE
// transformation is FROM RGB colorspace TO YUY2 packed format
// count must be divisable by four! (twice unrolled loop)

void ConvertRGB2YUV_SSE(Uint32 *dest, Uint32 *src, size_t count)
{
	float __attribute__ ((aligned (16))) cxmm4[4] = { M13, M33, M13, M23 };
	float __attribute__ ((aligned (16))) cxmm5[4] = { M12, M32, M12, M22 };
	float __attribute__ ((aligned (16))) cxmm6[4] = { M11, M31, M11, M21 };
	float __attribute__ ((aligned (16))) cxmm7[4] = {  16, 128,  16, 128 };
	Uint16 cmm6[4] = {0xffff, 0x00ff, 0xffff, 0x00ff};
	count /= 4;
	__asm __volatile ("pxor	%%mm7,	%%mm7\n":::"memory");
	while (count -- ) {

/*
	Assembly Implementation:
	get 16 bytes per cycle (4 points) to produce 8 bytes of YUV results
	i.e. the conversion loop is unrolled once. Proved to be 15% faster than
	the non-unrolled variant on an Athlon XP
*/
		__asm __volatile (
		"	movq	8%1,	%%mm0\n"
		"	movq	%1,	%%mm4\n"
		"	pand	%6,	%%mm0\n"
		"	pand	%6,	%%mm4\n"
		"	pshufw	$0x0e,	%%mm0,	%%mm1\n"
		"	pshufw	$0x0e,	%%mm4,	%%mm5\n"
// hint hardware to prefetch next 4 points:
		"	prefetchnta	16%1\n"

		"	punpcklbw	%%mm7,	%%mm0\n"
		"	punpcklbw	%%mm7,	%%mm4\n"
		"	punpcklbw	%%mm7,	%%mm1\n"
		"	punpcklbw	%%mm7,	%%mm5\n"

// mm0 = (b0, g0, r0, 00) (words)
// mm1 = (b1, g1, r1, 00) (words)
///---------------------------------------
// mm4 = (b2, g2, r2, 00) (words)
// mm5 = (b3, g3, r3, 00) (words)

		"	pshufw	$0xcc,	%%mm0,	%%mm2\n"
		"	pshufw	$0xcc,	%%mm4,	%%mm6\n"
		"	pshufw	$0xcc,	%%mm1,	%%mm3\n"
		"	pshufw	$0xcc,	%%mm5,	%%mm7\n"

		"	cvtpi2ps	%%mm2,	%%xmm1\n"
		"	cvtpi2ps	%%mm6,	%%xmm5\n"
		"	cvtpi2ps	%%mm3,	%%xmm0\n"
		"	cvtpi2ps	%%mm7,	%%xmm4\n"

		"	pshufw	$0xdd,	%%mm0,	%%mm2\n"
		"	pshufw	$0xdd,	%%mm1,	%%mm3\n"
		"	pshufw	$0xdd,	%%mm4,	%%mm6\n"
		"	pshufw	$0xdd,	%%mm5,	%%mm7\n"

		"	movlhps	%%xmm0,	%%xmm1\n"
		"	movlhps	%%xmm4,	%%xmm5\n"

		"	pshufw	$0xee,	%%mm0,	%%mm0\n"
		"	pshufw	$0xee,	%%mm1,	%%mm1\n"
		"	pshufw	$0xee,	%%mm4,	%%mm4\n"
		"	pshufw	$0xee,	%%mm5,	%%mm5\n"

		"	mulps	%2,	%%xmm1\n"
		"	mulps	%2,	%%xmm5\n"

		"	cvtpi2ps	%%mm2,	%%xmm2\n"
		"	cvtpi2ps	%%mm3,	%%xmm3\n"
		"	cvtpi2ps	%%mm6,	%%xmm6\n"
		"	cvtpi2ps	%%mm7,	%%xmm7\n"

		"	movlhps	%%xmm3,	%%xmm2\n"
		"	movlhps	%%xmm7,	%%xmm6\n"

		"	cvtpi2ps	%%mm0,	%%xmm3\n"
		"	cvtpi2ps	%%mm1,	%%xmm0\n"
		"	cvtpi2ps	%%mm4,	%%xmm7\n"
		"	cvtpi2ps	%%mm5,	%%xmm4\n"

		"	mulps	%3,	%%xmm2\n"
		"	mulps	%3,	%%xmm6\n"

		"	movlhps	%%xmm0,	%%xmm3\n"
		"	movlhps	%%xmm4,	%%xmm7\n"

		"	movaps	%5,	%%xmm0\n"
		"	movaps	%%xmm0,	%%xmm4\n"

		"	mulps	%4,	%%xmm3\n"
		"	mulps	%4,	%%xmm7\n"

		"	pxor	%%mm7,	%%mm7\n"

		"	addps	%%xmm1,	%%xmm0\n"
		"	addps	%%xmm5,	%%xmm4\n"
		"	addps	%%xmm3,	%%xmm2\n"
		"	addps	%%xmm7,	%%xmm6\n"
		"	addps	%%xmm2,	%%xmm0\n"
		"	addps	%%xmm6,	%%xmm4\n"

		"	movhlps	%%xmm0,	%%xmm1\n"
		"	movhlps	%%xmm4,	%%xmm5\n"

		"	cvtps2pi	%%xmm0,	%%mm0\n"
		"	cvtps2pi	%%xmm4,	%%mm4\n"
		"	cvtps2pi	%%xmm1,	%%mm1\n"
		"	cvtps2pi	%%xmm5,	%%mm5\n"

		"	packssdw	%%mm1,	%%mm0\n"
		"	packssdw	%%mm5,	%%mm4\n"
		"	packuswb	%%mm7,	%%mm0\n"
		"	packuswb	%%mm7,	%%mm4\n"

		//"	movd	%%mm0,	%0\n"
		"	psllq	$32,	%%mm0\n"
		"	por	%%mm0,	%%mm4\n"
		//"	movd	%%mm4,	4%0\n"
// we prefer movntq while it minimizes cache pollution:
		"	movntq	%%mm4,	%0\n"

		:"=m"(*dest)
		:"m"(*src), "m"(*cxmm4), "m"(*cxmm5), "m"(*cxmm6), "m"(*cxmm7), "m"(*cmm6)
		:"memory");
		dest+=2;
		src+=4;
	}
	__asm __volatile ("emms");
}

#endif

#ifdef rgbhacks1
/***************************************************************
 * gets some flags about the processor's capabilities          *
 *-------------------------------------------------------------*
 * This is the Windows version, requires CPUID. Should work on *
 * both AMD and Intel CPUs. Code taken from the internet:      *
 *  (looks very ugly :)                                        *
 * http://www.tommesani.com/DetectingMMXSSE.html               *
 ***************************************************************/
int CPUZ=0, CPUEXT=0, ECS=0;
 __asm __volatile (
 "          push	%%ebx\n"
 "          mov     $1,    %%eax \n"
 "          cpuid \n"
 "          mov     %%edx,  %0\n"
 "	    mov     $0x80000000, %%eax\n"
 "          cpuid\n"
 "          cmp     $0x80000001, %%eax\n"
 "          jb      not_supported\n"

 "          mov     $0x80000001, %%eax\n"
 "          cpuid\n"
 "          mov     %%edx,       %1\n"
 "          movl     $1,          %2\n"
 "          jmp     ende\n"

 "          not_supported:\n"
 "          movl     $0,          %2\n"
 "          ende:\n"
 "          pop	%%ebx\n"
 :"=m"(CPUZ), "=m"(CPUEXT), "=m"(ECS)
 :
 :"eax", "ecx", "edx"
 );

 hasmmx = (CPUZ&MMX_MASK?1:0);
 hassse = (CPUZ&SSE_MASK?1:0);
 hassse2= (CPUZ&SSE2_MASK?1:0);
 if (hassse) hasmmx2 = 1;
     else {
     if (ECS && (CPUEXT&MMX2_MASK)) hasmmx2=1;
                               else hasmmx2=0;
     }

#endif

#ifdef rgbhacks2
static void get_vendor_string(char *name) // for benchmarking only:
{

	__asm __volatile(
		"mov	%0,	%%esi\n"
		"push	%%ebx\n"
		"xorl	%%eax, 	%%eax\n"
		"cpuid\n"
		"mov	%%ebx,	(%%esi)\n"
		"mov	%%edx,	4(%%esi)\n"
		"mov	%%ecx,	8(%%esi)\n"
		"pop	%%ebx\n"
	::"m"(name)
	:"eax", "ecx", "edx", "esi"
	);
}
#endif

#ifdef threads1 
int atomic_add(volatile int *addr, int val) 
{
	__asm__ __volatile__(
		"lock; xadd	%0,	%1\n"
		:"=r"(val), "=m"(*addr)
		:"0"(val), "m"(*addr)
	);
	return val;
}
#endif

#ifdef VECTOR3_ASM
#define sse_rcp(x) \
({\
	register float _rezultf;\
	__asm__ (\
	"	rcpss	%1,	%%xmm0\n"\
	"	movss	%1,	%%xmm2\n"\
	"	movss	%%xmm0,	%%xmm1\n"\
	"	addss	%%xmm0,	%%xmm0\n"\
	"	mulss	%%xmm1,	%%xmm1\n"\
	"	mulss	%%xmm2,	%%xmm1\n"\
	"	subss	%%xmm1,	%%xmm0\n"\
	"	movss	%%xmm0,	%0\n"\
	:"=x"(_rezultf)\
	:"x"(x)\
	:"%xmm0","%xmm1","%xmm2"\
	);\
	_rezultf;\
})
#endif // VECTOR3_ASM

#else // __GNUC__
#ifdef _MSC_VER

#ifdef emit_emms
__asm {
	emms
}
#endif
#ifdef render1
			__asm {

				movups		xmm0,		[fx]
				movups		xmm1,		[fy]
				movups		xmm2,		[fxi]
				movups		xmm3,		[fyi]
				movups		xmm7,		[c64k]

				movq		mm0,		[gx]
				movq		mm1,		[gx + 8]
				movq		mm2,		[gy]
				movq		mm3,		[gy + 8]
				pxor		mm6,		mm6
				movd		mm6,		xtex

			}
#endif
#ifdef render2
					__asm {
						push		esi
						push		edi
				// calculate light degradation:
						movaps		xmm4,		[lightxy1] // load light x in xmm4
						movaps		xmm5,		[lightxy1 + 0x10] // load light y in xmm5
						movaps		xmm6,		xmm7
						subps		xmm4,		xmm0 // xmm4 = [ -current x + lightx ]   x 4
						subps		xmm5,		xmm1 // xmm5 = [ -current y + lighty ]   x 4
						mulps		xmm4,		xmm4 // xmm4 = [(-current x + lightx)^2] x 4
						mulps		xmm5,		xmm5 // xmm5 = [(-current y + lighty)^2] x 4
						addps		xmm4,		[lightxy1 + 0x20] // xmm4 = xmm4 + (1, 1, 1, 1)
						addps		xmm5,		[lightxy1 + 0x30] // xmm5 = xmm5 + [ysqrd] x 4
						addps		xmm4,		xmm5 // xmm4 = xmm4 + xmm5 = dist^2

						rsqrtps		xmm4,		xmm4 // xmm4 = 1 / dist
						mulps		xmm6,		xmm4 // xmm6 = const / dist
						rsqrtps		xmm4,		xmm4 // xmm4 = sqrt(dist)
						mulps		xmm6,		xmm4 // xmm6 = const / sqrt(dist)

				// save coordinates from mmx registers to memory:
						movq		[gx],		mm0
						movq		[gx + 8],		mm1
						movq		[gy],		mm2
						movq		[gy + 8],		mm3

				// shift all coords xshl positions, arithmetical
						psrad		mm0,		mm6
						psrad		mm1,		mm6
						psrad		mm2,		mm6
						psrad		mm3,		mm6
						movq		mm4,		mm0
						movq		mm5,		mm2
				// save 3rd and 4th x and y coordinate to work on later...
						movq		[save],		mm1
						movq		[save + 8],		mm3
				// shift the first copy additional 16 bits, arithmetical
						psrad		mm0,		16
						psrad		mm2,		16

	/* situation here:                                          ************
		mm0 = gX1&gX2 >> (16 + xshl) = Bx                 **    ****    **
		mm1 = unused & saved                             *     *  **      *
		mm2 = gY1&gY2 >> (16 + xshl) = By               *     *   **       *
		mm3 = unused & saved                            *         **       *
		mm4 = gX1&gX2 >> (xshl)                         *         **       *
		mm5 = gY1&gY2 >> (xshl)                         *         **       *
		mm6 = xshl                                       *        **      *
		mm7 = xxandmask                                   **   *******  **
                                                                    ************
	*/

						pslld		mm4,		16
						pslld		mm5,		16
						psrld		mm4,		17 //mm4 &= 0xffff = FX
						psrld		mm5,		17 //mm5 &= 0xffff = FY
						packssdw		mm4,		mm5 //mm4 = (lo to hi) -> [ fx0 fx1 fy0 fy1 ]
						psllw		mm4,		1

						pand		mm2,		[xxandmask]
						pand		mm0,		[xxandmask] // mm0 &= xandmask
//					SHIFT_LEFT_LOG2_OF_MAX_TEXTURE_SIZE_mm2
						pslld		mm2,		8
						psrld		mm2,		mm6 // mm2 = (mm2 & xandmask) << xshl
						paddd		mm0,		mm2 // mm0 = boffset[2]

				//	move the four points for the second coord in mm2 and mm3
						movd		eax,		mm0
						psrlq		mm0,		32
						movd		edx,		mm0
						mov		ecx,		texandmask
						mov		edi,		ptex
						movq		mm2,		[edi + eax*0x4]

						add		eax,		[xxandmask]
						inc		eax
						and		eax,		ecx
						movq		mm3,		[edi + eax*0x4] //!!

/* situation: (low-to-hi)
	mm2 -> [ boffset		, boffset+1 		] of first coord
	mm3 -> [ boffset+xandmask+1	, boffset+xandmask+2 	] of first coord
{
	mm2(1)*=(       fx)*(0xffff-fy)
	mm2(0)*=(0xffff-fx)*(0xffff-fy)
	mm3(1)*=(       fx)*(       fy)
	mm3(0)*=(0xffff-fx)*(       fy)
}
*/

						pxor		mm7,		mm7 // nullify mm7
						movq		mm0,		[my_ffs] // mm0 = 0xffffffffffffffff
						movq		mm1,		[my_ffs] // mm1 = mm0
						pshufw		mm5,		mm4,		0xaa //extract the third word to mm5
						psubw		mm0,		mm5
						pshufw		mm5,		mm4,		0 //extract the first word to all in mm5
						psubw		mm1,		mm5
						pmulhuw		mm5,		mm0
						pmulhuw		mm0,		mm1
						movq		mm1,		mm2 // save mm2 to mm1
						punpcklbw		mm2,	mm7
						psrlq		mm1,		32
						punpcklbw		mm1,	mm7 // boffset in mm1, boffset+1 in mm2

						pmulhuw		mm1,		mm5
						pmulhuw		mm2,		mm0
						paddusw		mm2,		mm1

						movq		mm0,		[my_ffs]
						pshufw		mm5,		mm4,		0xaa // extract y0 to mm5
						pshufw		mm1,		mm4,		0 // extract x0 to mm5
						psubw		mm0,		mm1
						pmulhuw		mm0,		mm5
						pmulhuw		mm5,		mm1
						movq		mm1,		mm3
						punpcklbw		mm3,		mm7
						psrlq		mm1,		32
						punpcklbw		mm1,		mm7

						pmulhuw		mm1,		mm5
						pmulhuw		mm3,		mm0
						paddusw		mm2,		mm1
						paddusw		mm2,		mm3
				// first point is done. Color in word fmt is in mm2.
						movq		mm5,		mm2
						punpcklwd		mm2,		mm7 // unpack words to mm2 (low)
						punpckhwd		mm5,		mm7 // and mm5 (high)
						cvtpi2ps		xmm4,		mm2	 // convert mm2 to xmm4
						cvtpi2ps		xmm5,		mm5	 // convert mm5 to xmm5
						movlhps		xmm4,		xmm5	 // get in xmm4 all components of color
						movaps		xmm5,		xmm6
						shufps		xmm5,		xmm6,		0	// fill all xmm5 with xmm6[0]
						mulps		xmm4,		xmm5	// multiply xmm4
				// repack to RGB entity using MMX:
						cvtps2pi		mm2,		xmm4
						movhlps		xmm5,		xmm4
						cvtps2pi		mm5,		xmm5
						packssdw		mm2,		mm5
						packuswb		mm2,		mm7
						mov			esi,		[dptr]
						movd		[esi],		mm2
/*
						    ************	.    ************
						  **            **	.  **            **
						 *    *****       *	. *    *****       *
						*    *     *       *	.*    *     *       *
						*          *       *	.*          *       *
						*         *        *	.*         *        *
						*       **         *	.*       **         *
						 *    **     *    *	. *    **     *    *
						  **  ********  **	.  **  ********  **
						    ************	.    ************
	*/


// next point baddress is in edx... repeat the whole procedure...
						mov		eax,		edx
						movq		mm2,		[edi + eax*0x4]

						add		eax,		[xxandmask]
						inc		eax
						and		eax,		ecx
						movq		mm3,		[edi + eax*0x4]

						movq		mm0,		[my_ffs] // mm0 = 0xffffffffffffffff
						movq		mm1,		[my_ffs] // mm1 = mm0
						pshufw		mm5,		mm4,		0xff //extract the third word to mm5
						psubw		mm0,		mm5
						pshufw		mm5,		mm4,		0x55 //extract the first word to all in mm5
						psubw		mm1,		mm5
						pmulhuw		mm5,		mm0
						pmulhuw		mm0,		mm1
						movq		mm1,		mm2 // save mm2 to mm1
						punpcklbw		mm2,	mm7
						psrlq		mm1,		32
						punpcklbw		mm1,	mm7 // boffset in mm1, boffset+1 in mm2

						pmulhuw		mm1,		mm5
						pmulhuw		mm2,		mm0
						paddusw		mm2,		mm1

						movq		mm0,		[my_ffs]
						pshufw		mm5,		mm4,		0xff // extract y0 to mm5
						pshufw		mm1,		mm4,		0x55 // extract x0 to mm5
						psubw		mm0,		mm1
						pmulhuw		mm0,		mm5
						pmulhuw		mm5,		mm1
						movq		mm1,		mm3
						punpcklbw		mm3,		mm7
						psrlq		mm1,		32
						punpcklbw		mm1,		mm7

						pmulhuw		mm1,		mm5
						pmulhuw		mm3,		mm0
						paddusw		mm2,		mm1
						paddusw		mm2,		mm3
				// second point is done. Color in word fmt is in mm2.
						movq		mm5,		mm2
						punpcklwd		mm2,		mm7 // unpack words to mm2 (low)
						punpckhwd		mm5,		mm7 // and mm5 (high)
						cvtpi2ps		xmm4,		mm2	 // convert mm2 to xmm4
						cvtpi2ps		xmm5,		mm5	 // convert mm5 to xmm5
						movlhps		xmm4,		xmm5	 // get in xmm4 all components of color
						movaps		xmm5,		xmm6
						shufps		xmm5,		xmm6,	0x55	// fill all xmm5 with xmm6[1]
						mulps		xmm4,		xmm5	// multiply xmm4
				// repack to RGB entity using MMX:
						cvtps2pi		mm2,		xmm4
						movhlps		xmm5,		xmm4
						cvtps2pi		mm5,		xmm5
						packssdw		mm2,		mm5
						packuswb		mm2,		mm7
						movd		[esi + 4],		mm2

/*
	    ************	    ************	    ************
	  **   ****     **	  **   ****     **	  **   ****     **
	 *    *    *      *	 *    *    *      *	 *    *    *      *
	*          *       *	*          *       *	*          *       *
	*       ***        *	*       ***        *	*       ***        *
	*          *       *	*          *       *	*          *       *
	*    *     *       *	*    *     *       *	*    *     *       *
	 *    *    *      *	 *    *    *      *	 *    *    *      *
	  **   ****     **	  **   ****     **	  **   ****     **
	    ************	    ************	    ************
	*/

// third coord:
				// restore 3rd and 4th x and y coordinate...
						movq		mm0,		[save]
						movq		mm2,		[save + 8]
						movq		mm4,		mm0
						movq		mm5,		mm2
				// shift the first copy additional 16 bits, arithmetical
						psrad		mm0,		16
						psrad		mm2,		16

						pslld		mm4,		16
						pslld		mm5,		16
						psrld		mm4,		17 //mm4 &= 0xffff = FX
						psrld		mm5,		17 //mm5 &= 0xffff = FY
						packssdw		mm4,		mm5 //mm4 = (lo to hi) -> [ fx0 fx1 fy0 fy1 ]
						psllw		mm4,		1

						pand		mm2,		[xxandmask]
						pand		mm0,		[xxandmask] // mm0 &= xandmask
//					SHIFT_LEFT_LOG2_OF_MAX_TEXTURE_SIZE_mm2
						pslld		mm2,		8
						psrld		mm2,		mm6 // mm2 = (mm2 & xandmask) << xshl
						paddd		mm0,		mm2 // mm0 = boffset[2]

				//	move the four points for the second coord in mm2 and mm3
						movd		eax,		mm0
						psrlq		mm0,		32
						movd		edx,		mm0
						mov		ecx,		texandmask
						mov		edi,		ptex
						movq		mm2,		[edi + eax*0x4]

						add		eax,		[xxandmask]
						inc		eax
						and		eax,		ecx
						movq		mm3,		[edi + eax*0x4]

						movq		mm0,		[my_ffs] // mm0 = 0xffffffffffffffff
						movq		mm1,		[my_ffs] // mm1 = mm0
						pshufw		mm5,		mm4,		0xaa //extract the third word to mm5
						psubw		mm0,		mm5
						pshufw		mm5,		mm4,		0 //extract the first word to all in mm5
						psubw		mm1,		mm5
						pmulhuw		mm5,		mm0
						pmulhuw		mm0,		mm1
						movq		mm1,		mm2 // save mm2 to mm1
						punpcklbw		mm2,	mm7
						psrlq		mm1,		32
						punpcklbw		mm1,	mm7 // boffset in mm1, boffset+1 in mm2

						pmulhuw		mm1,		mm5
						pmulhuw		mm2,		mm0
						paddusw		mm2,		mm1

						movq		mm0,		[my_ffs]
						pshufw		mm5,		mm4,		0xaa // extract y0 to mm5
						pshufw		mm1,		mm4,		0 // extract x0 to mm5
						psubw		mm0,		mm1
						pmulhuw		mm0,		mm5
						pmulhuw		mm5,		mm1
						movq		mm1,		mm3
						punpcklbw		mm3,		mm7
						psrlq		mm1,		32
						punpcklbw		mm1,		mm7

						pmulhuw		mm1,		mm5
						pmulhuw		mm3,		mm0
						paddusw		mm2,		mm1
						paddusw		mm2,		mm3
				// third point is done. Color in word fmt is in mm2.
						movq		mm5,		mm2
						punpcklwd		mm2,		mm7 // unpack words to mm2 (low)
						punpckhwd		mm5,		mm7 // and mm5 (high)
						cvtpi2ps		xmm4,		mm2	 // convert mm2 to xmm4
						cvtpi2ps		xmm5,		mm5	 // convert mm5 to xmm5
						movlhps		xmm4,		xmm5	 // get in xmm4 all components of color
						movaps		xmm5,		xmm6
						shufps		xmm5,		xmm6,	0xaa	// fill all xmm5 with xmm6[2]
						mulps		xmm4,		xmm5	// multiply xmm4
				// repack to RGB entity using MMX:
						cvtps2pi		mm2,		xmm4
						movhlps		xmm5,		xmm4
						cvtps2pi		mm5,		xmm5
						packssdw		mm2,		mm5
						packuswb		mm2,		mm7
						movd		[esi + 8],		mm2

/*
							    ************
							  **            **
							 *   ***  **      *
							*     *   *        *
							*     *   *        *
							*     *****        *
							*         *        *
							 *        *       *
							  **      **    **
							    ************
******************************************************************************
********************************        **************************************
***************************                  *********************************
********                                                          ************
********                  *****   ****   ****                     ************
********                    *       *     *                       ************
********                    *        *   *                        ************
********                    *         * *                         ************
********                  *****        *                          ************
********                                                          ************
********************************        **************************************
******************************************************************************
	*/
// FOURTH point!
// next point baddress is in edx... repeat the whole procedure...
						mov		eax,		edx
						movq		mm2,		[edi + eax*0x4]

						add		eax,		[xxandmask]
						inc		eax
						and		eax,		ecx
						movq		mm3,		[edi + eax*0x4]

						movq		mm0,		[my_ffs] // mm0 = 0xffffffffffffffff
						movq		mm1,		[my_ffs] // mm1 = mm0
						pshufw		mm5,		mm4,		0xff //extract the third word to mm5
						psubw		mm0,		mm5
						pshufw		mm5,		mm4,		0x55 //extract the first word to all in mm5
						psubw		mm1,		mm5
						pmulhuw		mm5,		mm0
						pmulhuw		mm0,		mm1
						movq		mm1,		mm2 // save mm2 to mm1
						punpcklbw		mm2,	mm7
						psrlq		mm1,		32
						punpcklbw		mm1,	mm7 // boffset in mm1, boffset+1 in mm2

						pmulhuw		mm1,		mm5
						pmulhuw		mm2,		mm0
						paddusw		mm2,		mm1

						movq		mm0,		[my_ffs]
						pshufw		mm5,		mm4,		0xff // extract y0 to mm5
						pshufw		mm1,		mm4,		0x55 // extract x0 to mm5
						psubw		mm0,		mm1
						pmulhuw		mm0,		mm5
						pmulhuw		mm5,		mm1
						movq		mm1,		mm3
						punpcklbw		mm3,		mm7
						psrlq		mm1,		32
						punpcklbw		mm1,		mm7

						pmulhuw		mm1,		mm5
						pmulhuw		mm3,		mm0
						paddusw		mm2,		mm1
						paddusw		mm2,		mm3
				// last point is done. Color in word fmt is in mm2.
						movq		mm5,		mm2
						punpcklwd		mm2,		mm7 // unpack words to mm2 (low)
						punpckhwd		mm5,		mm7 // and mm5 (high)
						cvtpi2ps		xmm4,		mm2	 // convert mm2 to xmm4
						cvtpi2ps		xmm5,		mm5	 // convert mm5 to xmm5
						movlhps		xmm4,		xmm5	 // get in xmm4 all components of color
						movaps		xmm5,		xmm6
						shufps		xmm5,		xmm6,	0xff	// fill all xmm5 with xmm6[3]
						mulps		xmm4,		xmm5	// multiply xmm4
				// repack to RGB entity using MMX:
						cvtps2pi		mm2,		xmm4
						movhlps		xmm5,		xmm4
						cvtps2pi		mm5,		xmm5
						packssdw		mm2,		mm5
						packuswb		mm2,		mm7
						movd		[esi + 12],		mm2

				// get integer coordinates from memory:
						movq		mm0,		[gx]
						movq		mm1,		[gx + 8]
						movq		mm2,		[gy]
						movq		mm3,		[gy + 8]

				// continue interpolation:
						paddd		mm0,		[gxx]
						paddd		mm1,		[gxx + 8]
						paddd		mm2,		[gxx + 16]
						paddd		mm3,		[gxx + 24]
						addps		xmm0,		xmm2
						addps		xmm1,		xmm3
						pop			edi
						pop			esi
					}
#endif
#ifdef render3
					__asm {
						push		esi
						push		edi
				// calculate light degradation:
						movaps		xmm4,		[lightxy1] // load light x in xmm4
						movaps		xmm5,		[lightxy1 + 0x10] // load light y in xmm5
						movaps		xmm6,		xmm7
						subps		xmm4,		xmm0 // xmm4 = [ -current x + lightx ]   x 4
						subps		xmm5,		xmm1 // xmm5 = [ -current y + lighty ]   x 4
						mulps		xmm4,		xmm4 // xmm4 = [(-current x + lightx)^2] x 4
						mulps		xmm5,		xmm5 // xmm5 = [(-current y + lighty)^2] x 4
						addps		xmm4,		[lightxy1 + 0x20] // xmm4 = xmm4 + (1, 1, 1, 1)
						addps		xmm5,		[lightxy1 + 0x30] // xmm5 = xmm5 + [ysqrd] x 4
						addps		xmm4,		xmm5 // xmm4 = xmm4 + xmm5 = dist^2

						rsqrtps		xmm4,		xmm4 // xmm4 = 1 / dist
						mulps		xmm6,		xmm4 // xmm6 = const / dist
						rsqrtps		xmm4,		xmm4 // xmm4 = sqrt(dist)
						mulps		xmm6,		xmm4 // xmm6 = const / sqrt(dist)

				// save coordinates from mmx registers to memory:
						movq		[gx],		mm0
						movq		[gx + 8],		mm1
						movq		[gy],		mm2
						movq		[gy + 8],		mm3

				// shift all coords 16 positions, arithmetical
						psrad		mm0,		16
						psrad		mm1,		16
						psrad		mm2,		16
						psrad		mm3,		16
						movq		mm7,		[xxandmask]
				// then shift xtex positions, logical
						psrld		mm0,		mm6
						psrld		mm1,		mm6
						psrld		mm2,		mm6
						psrld		mm3,		mm6

						pand		mm0,		mm7 // mm0 &= xandmask; (mm0 -> x0 x1)
						pand		mm1,		mm7 // mm1 &= xandmask; (mm1 -> x2 x3)
						pand		mm2,		mm7 // mm2 &= xandmask; (mm2 -> y0 y1)
						pand		mm3,		mm7 // mm3 &= xandmask; (mm3 -> y2 y3)

//					SHIFT_LEFT_LOG2_OF_MAX_TEXTURE_SIZE_mm2
//					SHIFT_LEFT_LOG2_OF_MAX_TEXTURE_SIZE_mm3
						pslld		mm2,		8
						pslld		mm3,		8
						psrld		mm2,		mm6 // mm2 <<= xshl
						psrld		mm3,		mm6 // mm3 <<= xshl

						paddd		mm0,		mm2 // mm0 = index0 index1
						paddd		mm1,		mm3 // mm1 = index2 index3

						pxor		mm7,		mm7 // nullify mm7
						mov		edi,		[ptex]

						movd		eax,		mm0
						psrlq		mm0,		32
						movd		edx,		mm0

						movd		mm2,		[edi + eax*4] // get first pixel in mm2
						movd		mm3,		[edi + edx*4] // get second pixel in mm3

						punpcklbw		mm2,		mm7
						punpcklbw		mm3,		mm7

						movq		mm4,		mm2
						movq		mm5,		mm3

						punpcklwd		mm2,		mm7
						punpcklwd		mm3,		mm7
						punpckhwd		mm4,		mm7
						punpckhwd		mm5,		mm7

						cvtpi2ps		xmm2,		mm2
						cvtpi2ps		xmm3,		mm3
						cvtpi2ps		xmm4,		mm4
						cvtpi2ps		xmm5,		mm5
						movlhps		xmm2,		xmm4
						movlhps		xmm3,		xmm5
						movaps		xmm4,		xmm6
						movaps		xmm5,		xmm6
						shufps		xmm4,		xmm6,		0
						shufps		xmm5,		xmm6,		0x55
						mulps		xmm2,		xmm4
						mulps		xmm3,		xmm5
						movhlps		xmm4,		xmm2
						movhlps		xmm5,		xmm3
						cvtps2pi		mm2,		xmm2
						cvtps2pi		mm3,		xmm3
						cvtps2pi		mm4,		xmm4
						cvtps2pi		mm5,		xmm5
						packssdw		mm2,		mm4
						packssdw		mm3,		mm5
						packuswb		mm2,		mm7
						packuswb		mm3,		mm7
					// points 0 and 1 are ready, push to framebuffer:
						mov			esi,		[dptr]
						movd		[esi],		mm2
						movd		[esi + 4],		mm3
// proceed with points 2 and 3...
						movd		eax,		mm1
						psrlq		mm1,		32
						movd		edx,		mm1

						movd		mm2,		[edi + eax*4] // get first pixel in mm2
						movd		mm3,		[edi + edx*4] // get second pixel in mm3

						punpcklbw		mm2,		mm7
						punpcklbw		mm3,		mm7

						movq		mm4,		mm2
						movq		mm5,		mm3

						punpcklwd		mm2,		mm7
						punpcklwd		mm3,		mm7
						punpckhwd		mm4,		mm7
						punpckhwd		mm5,		mm7

						cvtpi2ps		xmm2,		mm2
						cvtpi2ps		xmm3,		mm3
						cvtpi2ps		xmm4,		mm4
						cvtpi2ps		xmm5,		mm5
						movlhps		xmm2,		xmm4
						movlhps		xmm3,		xmm5
						movaps		xmm4,		xmm6
						movaps		xmm5,		xmm6
						shufps		xmm4,		xmm6,		0xaa
						shufps		xmm5,		xmm6,		0xff
						mulps		xmm2,		xmm4
						mulps		xmm3,		xmm5
						movhlps		xmm4,		xmm2
						movhlps		xmm5,		xmm3
						cvtps2pi		mm2,		xmm2
						cvtps2pi		mm3,		xmm3
						cvtps2pi		mm4,		xmm4
						cvtps2pi		mm5,		xmm5
						packssdw		mm2,		mm4
						packssdw		mm3,		mm5
						packuswb		mm2,		mm7
						packuswb		mm3,		mm7
					// points 2 and 3 are ready, push to framebuffer:
						movd		[esi + 8],		mm2
						movd		[esi + 12],		mm3

				// get integer coordinates from memory:
						movq		mm0,		[gx]
						movq		mm1,		[gx + 8]
						movq		mm2,		[gy]
						movq		mm3,		[gy + 8]

				// continue interpolation:
						paddd		mm0,		[gxx]
						paddd		mm1,		[gxx + 8]
						paddd		mm2,		[gxx + 16]
						paddd		mm3,		[gxx + 24]
						movaps		xmm2,		[fxyi]
						movaps		xmm3,		[fxyi + 16]
						addps		xmm0,		xmm2
						addps		xmm1,		xmm3
						pop			edi
						pop			esi
					}
#endif

#ifdef antialias_asm
// MMX2 version of the antialias folding routine... does two output pixels per loop, gets 8 points and does them in
// parallel.

// it is low fidelity, because before adding the four points, two LSB of every point are lost, resulting in
// e.g., (7+7+7+7)/4 = 4...

static void antialias_4x_mmx2_lo_fi(Uint32 *fb)
{
	int xr = xres(), yr = yres();
	unsigned pp[2] = {0x3f3f3f3f, 0x3f3f3f3f};
	__asm {

		mov		edx,		yr
		mov		esi,		fb
		mov		edi,		fb
		mov		ebx,		xr
		shl		ebx,		3
		movq		mm7,		[pp]

	rowcycle:
		mov		eax,		xr
		shr		eax,		1
	pixelcycle:
		movq		mm0,		[esi]
		movq		mm1,		[esi + 8]
		movq		mm2,		[esi + ebx]
		movq		mm3,		[esi + ebx + 8]

		psrlq		mm0,		2
		psrlq		mm1,		2
		psrlq		mm2,		2
		psrlq		mm3,		2

		pand		mm0,		mm7
		pand		mm1,		mm7
		pand		mm2,		mm7
		pand		mm3,		mm7

		paddusb		mm0,		mm2
		paddusb		mm1,		mm3

		pshufw		mm2,		mm0,		0x0e
		pshufw		mm3,		mm1,		0x0e
		paddusb		mm0,		mm2
		paddusb		mm1,		mm3

		movd		[edi],		mm0
		movd		[edi + 4],		mm1

		add		edi,		8
		add		esi,		16

		dec		eax
		jnz		pixelcycle
		add		esi,		ebx
		dec		edx
		jnz		rowcycle
		emms

	}
}

/***********************************************************
 * High quality version of the antialias folding routine   *
 * Uses MMX. Does all arithmetics in full precision	   *
 ***********************************************************/

static void antialias_4x_mmx_hi_fi(Uint32 *fb)
{
	int xr = xres(), yr = yres();
	__asm {

		mov		edx,		yr
		mov		esi,		fb
		mov		edi,		fb
		mov		ebx,		xr
		shl		ebx,		3
		pxor		mm7,		mm7

	rowcycle2:
		mov		eax,		xr
		shr		eax,		1
	pixelcycle2:
		movq		mm0,		[esi]
		movq		mm1,		[esi + 8]
		movq		mm2,		[esi + ebx]
		movq		mm3,		[esi + ebx + 8]

		movq		mm4,		mm0
		movq		mm5,		mm1
		movq		mm6,		mm2

		punpcklbw		mm0,		mm7
		punpckhbw		mm4,		mm7
		punpcklbw		mm1,		mm7
		punpckhbw		mm5,		mm7
		punpcklbw		mm2,		mm7
		punpckhbw		mm6,		mm7

		paddusw		mm0,		mm4
		movq		mm4,		mm3

		paddusw		mm1,		mm5

		punpcklbw		mm3,		mm7
		punpckhbw		mm4,		mm7

		paddusw		mm2,		mm6
		paddusw		mm3,		mm4

		paddusw		mm0,		mm2
		paddusw		mm1,		mm3

		psrlw		mm0,		2
		psrlw		mm1,		2

		packuswb		mm0,		mm7
		packuswb		mm1,		mm7

		movd		[edi],		mm0
		movd		[edi + 4],		mm1

		add		edi,		8
		add		esi,		16

		dec		eax
		jnz		pixelcycle2
		add		esi,		ebx
		dec		edx
		jnz		rowcycle2
		emms

	}
}
#endif
#ifdef gfx_asm
// WITH SSE
Uint32 blend_sse(Uint32 f, Uint32 b, float ff)
{Uint32 rezult;
 __asm {

 	pxor		mm3,		mm3
 	movd		mm0,		f
 	movd		mm1,		b
 	punpcklbw		mm0,		mm3
 	movq		mm2,		mm0
 	punpcklwd		mm0,		mm3
 	punpckhwd		mm2,		mm3

 	cvtpi2ps		xmm0,		mm0
 	cvtpi2ps		xmm1,		mm2
 	movlhps		xmm0,		xmm1

 	punpcklbw		mm1,		mm3
 	movq		mm2,		mm1
 	punpcklwd		mm1,		mm3
 	punpckhwd		mm2,		mm3

 	cvtpi2ps		xmm1,		mm1
 	cvtpi2ps		xmm2,		mm2
 	movlhps		xmm1,		xmm2

 	movss		xmm2,		ff
 	shufps		xmm2,		xmm2,		0

 	movaps		xmm3,		[w2_flones]
 	subps		xmm3,		xmm2
 	mulps		xmm0,		xmm2
 	mulps		xmm1,		xmm3
 	addps		xmm0,		xmm1

 	movhlps		xmm1,		xmm0
 	cvtps2pi		mm0,		xmm0
 	cvtps2pi		mm1,		xmm1

 	packssdw		mm0,		mm1
 	packuswb		mm0,		mm1
 	movd		rezult,		mm0
 	emms

 
 }
 return rezult;
}
#endif
#ifdef profile_asm
long long prof_rdtsc(void)
{
	long long r;
	int *a;
	a = (int *) &r;
	__asm {
			mov		ecx,		a
			rdtsc
			mov		[ecx],		eax
			mov		[ecx+4],	edx
	}
	return r;
}
#endif
#ifdef shaders_asm
void convolve_mmx_w_shifts_generic(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift)
{
	int toadd, n, cols, rows;
	SSE_ALIGN(Sint16 wmm[MA3X_MAX_SIZE*MA3X_MAX_SIZE][4]);
	Sint16 *mm = wmm[0];
	SSE_ALIGN(Uint16 fourones[4]) = {256, 256, 256, 256};
	int pshift;

	int i, j, k;
	for (i=0;i<M->n;i++)
		for (j=0;j<M->n;j++)
			for (k=0;k<4;k++) wmm[i*M->n+j][k] = M->coeff[i][j];

	n = M->n;

	pshift = (shift>0)?shift:0;
	toadd = (resx - M->n)*4;
	rows = resy - (n & ~1);
	cols = resx - (n & ~1);
	dest+=(resx*(M->n/2) + M->n/2);

	__asm {
		push		ebx
	// initialize some mmx regs...
		pxor		mm7,		mm7
		movd		mm6,		shift
		movd		mm5,		[fourones]
	// init the stuff

	// bx = M->coeff; si = src[...]; di = dst[...]
		mov		esi,		[src]
		mov		edi,		[dest]
		mov		ebx,		[mm]

	rowloop:
		mov		edx,		cols

	ALIGN	16
	pixelloop:

		mov		ecx,		n
		pxor		mm0,		mm0
		pxor		mm2,		mm2

	ALIGN	16
	y:

		mov		eax,		n

	ALIGN	16
	x:

		cmp		eax,		2
		jnb		enough

		movd		mm1,		[esi]
		punpcklbw		mm1,		mm7
		pmullw		mm1,		[ebx]
		paddsw		mm0,		mm1

		add		esi,		4
		add		ebx,		8

		jmp		xend

	ALIGN	16
	enough:
		movd		mm1,		[esi]
		movd		mm3,		[esi + 4]
		punpcklbw		mm1,		mm7
		punpcklbw		mm3,		mm7
		pmullw		mm1,		[ebx]
		pmullw		mm3,		[ebx + 8]
		paddsw		mm0,		mm1
		paddsw		mm2,		mm3

		add		esi,		8
		add		ebx,		16
		sub		eax,		2
		jnz		x

	ALIGN	16
	xend:

		add		esi,		toadd

		dec		ecx
		jnz		y

		paddsw		mm0,		mm2
		paddsw		mm0,		mm5
		psubusw		mm0,		mm5
		psraw		mm0,		mm6
		packuswb		mm0,		mm7
		movd		[edi],		mm0

		mov		ebx,		mm

		mov		eax,		n
		xchg		ecx,		edx
		mul		toadd
		sub		esi,		eax
		mov		eax,		n
		mul		n
		shl		eax,		2
		sub		esi,		eax

		xchg		ecx,		edx

		add		edi,		4
		add		esi,		4

		dec		edx
		jnz		pixelloop

		mov		eax,		n
		and		eax,		0xfffffffe
		shl		eax,		2
		add		esi,		eax
		add		edi,		eax

		dec		rows
		jnz		rowloop
		pop		ebx
	
	}

	__asm {
		emms
	}
}

/*
	convolve_mmx_w_shifts_3

		DESCRIPTION:
		Does the convolution effect with the given coefficient table. This routine uses a hardcoded assumption, that
		the matrix size is 3 (and thus it is above twice as fast than the `generic version). If the convolution
		matrix is of any other size, error occurs.

		This routine maximizes parallelism utilization.

		INPUT:
		*src - Input array of RGB values
		*dest- Output framebuffer (note that this function can't do the conversion in-place, src must be different from dest)
		resx - width of the framebuffer
		resy - height of the framebuffer
		*M   - the input convoluion matrix
		shift- if the sum of the coefficients is above 1, the convolution result must be `normalized', i.e.
		       divided by that sum. It works best when the sum is a power of two and so shifts can be used
		       instead of divides. The `shift' value here means how many bits to shift-right after addition.
		       If the shift is less than 1, no actual shifting occurs.

		OUTPUT: none (*dest contains the convolution effect result)
*/

void convolve_mmx_w_shifts_3(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift)
{
	int toadd, cols, rows, n=3;
	SSE_ALIGN(Sint16 wmm[9][4]);
	Sint16 *mm = wmm[0];
	SSE_ALIGN(Uint16 fourones[4]) = {256, 256, 256, 256};
	int pshift;

	if (M->n!=3) {
		printf("convolve_mmx_w_shifts_3: expecting matrix size of 3, but size=%d received!\n", M->n);
		return;
	}

	int i, j, k;
	for (i=0;i<3;i++)
		for (j=0;j<3;j++)
			for (k=0;k<4;k++) wmm[i*3+j][k] = M->coeff[i][j];

	pshift = (shift>0)?shift:0;
	toadd = resx*4;
	rows = resy - 2;
	cols = resx - 2;
	dest+=(resx*1 + 1);

	__asm {
		push		ebx
	// initialize some mmx regs...
		pxor		mm7,		mm7
		movd		mm6,		pshift
	// init the stuff

	// bx = M->coeff; si = src[...]; di = dst[...]
		mov		esi,		[src]
		mov		edi,		[dest]
		mov		ebx,		mm

	rowloop_3:
		mov		edx,		cols

	ALIGN	16
	pixelloop_3:

		movd		mm0,		[esi]
		movd		mm1,		[esi + 4]
		movd		mm2,		[esi + 8]
		add		esi,		toadd

		punpcklbw		mm0,		mm7
		punpcklbw		mm1,		mm7
		punpcklbw		mm2,		mm7

		pmullw		mm0,		[ebx]
		pmullw		mm1,		[ebx + 8]
		pmullw		mm2,		[ebx + 16]

		movd		mm3,		[esi]
		movd		mm4,		[esi + 4]
		movd		mm5,		[esi + 8]
		add		esi,		toadd

		punpcklbw		mm3,		mm7
		punpcklbw		mm4,		mm7
		punpcklbw		mm5,		mm7

		pmullw		mm3,		[ebx + 24]
		pmullw		mm4,		[ebx + 32]
		pmullw		mm5,		[ebx + 40]

		paddsw		mm3,		mm0
		paddsw		mm4,		mm1
		paddsw		mm5,		mm2

		movd		mm0,		[esi]
		movd		mm1,		[esi + 4]
		movd		mm2,		[esi + 8]

		punpcklbw		mm0,		mm7
		punpcklbw		mm1,		mm7
		punpcklbw		mm2,		mm7

		pmullw		mm0,		[ebx + 48]
		pmullw		mm1,		[ebx + 56]
		pmullw		mm2,		[ebx + 64]


		paddsw		mm0,		mm3
		paddsw		mm1,		mm4
		paddsw		mm2,		mm5

		paddsw		mm0,		mm1
		paddsw		mm3,		[fourones]
		paddsw		mm0,		mm3

		psubusw		mm0,		[fourones]
		psraw		mm0,		mm6
		packuswb		mm0,		mm7
		movd		[edi],		mm0

		sub		esi,		toadd
		sub		esi,		toadd

		add		edi,		4
		add		esi,		4

		dec		edx
		jnz		pixelloop_3

		mov		eax,		n
		and		eax,		0xfffffffe
		shl		eax,		2
		add		esi,		eax
		add		edi,		eax

		dec		rows
		jnz		rowloop_3

		pop		ebx
	}

	__asm {
		emms
	}
}

// this function assumes that M->n is 5.
/*
	convolve_mmx_w_shifts_5

		DESCRIPTION: see the info for convolve_mmx_w_shifts_3, this function does the same,
	but is hardcoded and loop-unrolled for matrix size of 5, attempting to maximize parallelism utilization
*/
void convolve_mmx_w_shifts_5(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift)
{
	int toadd, cols, rows, n=5;
	SSE_ALIGN(Sint16 wmm[25][4]);
	Sint16 *mm = wmm[0];
	SSE_ALIGN(Uint16 fourones[4]) = {256, 256, 256, 256};
	int pshift;

	if (M->n!=5) {
		printf("convolve_mmx_w_shifts_5: expecting matrix size of 5, but size=%d received!\n", M->n);
		return;
	}

	int i, j, k;
	for (i=0;i<5;i++)
		for (j=0;j<5;j++)
			for (k=0;k<4;k++) wmm[i*5+j][k] = M->coeff[i][j];

	pshift = (shift>0)?shift:0;
	toadd = resx*4;
	rows = resy - 4;
	cols = resx - 4;
	dest+=(resx*2 + 2);

	__asm {
		push		ebx
	// initialize some mmx regs...
		pxor		mm7,		mm7
//		movd		mm6,		shift
	// init the stuff

	// bx = M->coeff; si = src[...]; di = dst[...]
		mov		esi,		[src]
		mov		edi,		[dest]
		mov		ebx,		mm

	rowloop_5:
		mov		edx,		cols

	ALIGN	16
	pixelloop_5:

	// row 1:

		movd		mm2,		[esi]
		movd		mm3,		[esi + 4]
		movd		mm4,		[esi + 8]
		movd		mm5,		[esi + 12]
		movd		mm6,		[esi + 16]
		add		esi,		toadd

		punpcklbw		mm2,		mm7
		punpcklbw		mm3,		mm7
		punpcklbw		mm4,		mm7
		punpcklbw		mm5,		mm7
		punpcklbw		mm6,		mm7

		pmullw		mm2,		[ebx]
		pmullw		mm3,		[ebx + 8]
		pmullw		mm4,		[ebx + 16]
		pmullw		mm5,		[ebx + 24]
		pmullw		mm6,		[ebx + 32]

		paddsw		mm4,		mm2
		paddsw		mm5,		mm3

	// row 2:
		movd		mm0,		[esi]
		movd		mm1,		[esi + 4]

		paddsw		mm6,		mm4

		movd		mm2,		[esi + 8]
		movd		mm3,		[esi + 12]
		movd		mm4,		[esi + 16]
		add		esi,		toadd

		punpcklbw		mm0,		mm7
		punpcklbw		mm1,		mm7
		punpcklbw		mm2,		mm7
		punpcklbw		mm3,		mm7
		punpcklbw		mm4,		mm7

		pmullw		mm0,		[ebx + 40]
		pmullw		mm1,		[ebx + 48]
		pmullw		mm2,		[ebx + 56]
		pmullw		mm3,		[ebx + 64]
		pmullw		mm4,		[ebx + 72]

		paddsw		mm2,		mm0
		paddsw		mm3,		mm1
		paddsw		mm5,		mm4
		paddsw		mm6,		mm2

	// row 3:
		movd		mm0,		[esi]
		movd		mm1,		[esi + 4]

		paddsw		mm5,		mm3

		movd		mm2,		[esi + 8]
		movd		mm3,		[esi + 12]
		movd		mm4,		[esi + 16]
		add		esi,		toadd

		punpcklbw		mm0,		mm7
		punpcklbw		mm1,		mm7
		punpcklbw		mm2,		mm7
		punpcklbw		mm3,		mm7
		punpcklbw		mm4,		mm7

		pmullw		mm0,		[ebx + 80]
		pmullw		mm1,		[ebx + 88]
		pmullw		mm2,		[ebx + 96]
		pmullw		mm3,		[ebx + 104]
		pmullw		mm4,		[ebx + 112]

		paddsw		mm2,		mm0
		paddsw		mm3,		mm1
		paddsw		mm5,		mm4
		paddsw		mm6,		mm2

	// row 4:
		movd		mm0,		[esi]
		movd		mm1,		[esi + 4]

		paddsw		mm5,		mm3

		movd		mm2,		[esi + 8]
		movd		mm3,		[esi + 12]
		movd		mm4,		[esi + 16]
		add		esi,		toadd

		punpcklbw		mm0,		mm7
		punpcklbw		mm1,		mm7
		punpcklbw		mm2,		mm7
		punpcklbw		mm3,		mm7
		punpcklbw		mm4,		mm7

		pmullw		mm0,		[ebx + 120]
		pmullw		mm1,		[ebx + 128]
		pmullw		mm2,		[ebx + 136]
		pmullw		mm3,		[ebx + 144]
		pmullw		mm4,		[ebx + 152]

		paddsw		mm2,		mm0
		paddsw		mm3,		mm1
		paddsw		mm5,		mm4
		paddsw		mm6,		mm2

	// row 5:
		movd		mm0,		[esi]
		movd		mm1,		[esi + 4]

		paddsw		mm5,		mm3

		movd		mm2,		[esi + 8]
		movd		mm3,		[esi + 12]
		movd		mm4,		[esi + 16]

		punpcklbw		mm0,		mm7
		punpcklbw		mm1,		mm7
		punpcklbw		mm2,		mm7
		punpcklbw		mm3,		mm7
		punpcklbw		mm4,		mm7

		pmullw		mm0,		[ebx + 160]
		pmullw		mm1,		[ebx + 168]
		pmullw		mm2,		[ebx + 176]
		pmullw		mm3,		[ebx + 184]
		pmullw		mm4,		[ebx + 192]

// situation here: we have data all over the first 7 registers.. we must first sum it up...

		paddsw		mm6,		mm5
		paddsw		mm1,		mm0
		paddsw		mm3,		mm2
		paddsw		mm6,		mm4
		paddsw		mm1,		[fourones]
		movd		mm0,		pshift

		paddsw		mm3,		mm6
		paddsw		mm1,		mm3


		psubusw		mm1,		[fourones]
		psraw		mm1,		mm0
		packuswb		mm1,		mm7
		movd		[edi],		mm1

		mov		eax,		toadd
		shl		eax,		2
		sub		esi,		eax

		add		edi,		4
		add		esi,		4

		dec		edx
		jnz		pixelloop_5

		mov		eax,		n
		and		eax,		0xfffffffe
		shl		eax,		2
		add		esi,		eax
		add		edi,		eax

		dec		rows
		jnz		rowloop_5

		pop		ebx	
	}

	__asm {
		emms
	}
}

/*
	convolve_mmx_w_shifts:

		DESCRIPTION: does the convolution effect on a given coefficien matrix (up to 8x8) with sum normalization
	using bitwize right shifts (note that this can only be done when the sum of the coeficients of the convolution matrix
	is either zero, or a power of two).
		Runs very fast in the special case of 3x3 or 5x5 matrices, as a special hardcoded and tightly optimized
	(loop unrolled) versions of the effect are called.
		All functions use the "original" MMX instruction set (it is OK on PentiumII and MMX-enabled Pentium-Is)

		INPUT:
		*src - Input array of RGB values
		*dest- Output framebuffer (note that this function can't do the conversion in-place, src must be different from dest)
		resx - width of the framebuffer
		resy - height of the framebuffer
		*M   - the input convoluion matrix
		shift- if the sum of the coefficients is above 1, the convolution result must be `normalized', i.e.
		       divided by that sum. It works best when the sum is a power of two and so shifts can be used
		       instead of divides. The `shift' value here means how many bits to shift-right after addition.
		       If the shift is less than 1, no actual shifting occurs.

		OUTPUT: none (*dest contains the convolution effect result)

*/
static inline void convolve_mmx_w_shifts(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift)
{
	//printf("called convolve_mmx_w_shifts(...,...,....,...,...,%d)\n", shift);
	switch (M->n) { // if we have a match for that matrix size, we should use it - it would be the fastest function
		case 3:  convolve_mmx_w_shifts_3(src, dest, resx, resy, M, shift); break;
		case 5:  convolve_mmx_w_shifts_5(src, dest, resx, resy, M, shift); break;
		default: convolve_mmx_w_shifts_generic(src, dest, resx, resy, M, shift);
	}
}

// shifts left all pixel data by the specified shift. This is the same as raising the gamma by 2^shift
// NOTE: uses MMX and requires the number of pixels % 4 == 0
void shader_gamma_shl(Uint32 *src, Uint32 *dest, int resx, int resy, int shift)
{
	int count = (resx * resy) / 4;

	// the innermost loop is 4 times unrolled
	__asm {

// prepare the loop:
		pxor		mm7,		mm7
		movd		mm6,		shift
		mov		eax,		count
		mov		edi,		[dest]
		mov		esi,		[src]

	ALIGN	16
	shader_gamma_shl_loop:

		movd		mm0,		[esi]
		movd		mm1,		[esi + 4]
		movd		mm2,		[esi + 8]
		movd		mm3,		[esi + 12]

		punpcklbw		mm0,		mm7
		punpcklbw		mm1,		mm7
		punpcklbw		mm2,		mm7
		punpcklbw		mm3,		mm7

		psllw		mm0,		mm6
		psllw		mm1,		mm6
		psllw		mm2,		mm6
		psllw		mm3,		mm6

		packuswb		mm0,		mm7
		packuswb		mm1,		mm7
		packuswb		mm2,		mm7
		packuswb		mm3,		mm7

		movd		[edi],		mm0
		movd		[edi + 4],		mm1
		movd		[edi + 8],		mm2
		movd		[edi + 12],		mm3

		add		esi,		16
		add		edi,		16
		dec		eax
		jnz		shader_gamma_shl_loop
		emms
	
	}
}

// shifts right all pixel data by the specified shift. This is the same as lowering the gamma by 2^shift
// NOTE: uses MMX and requires the number of pixels % 4 == 0
void shader_gamma_shr(Uint32 *src, Uint32 *dest, int resx, int resy, int shift)
{
	int count = (resx * resy) / 4;

	// the innermost loop is 4 times unrolled
	__asm {

// prepare the loop:
		pxor		mm7,		mm7
		movd		mm6,		shift
		mov		eax,		count
		mov		edi,		[dest]
		mov		esi,		[src]

	ALIGN	16
	shader_gamma_shr_loop:

		movd		mm0,		[esi]
		movd		mm1,		[esi + 4]
		movd		mm2,		[esi + 8]
		movd		mm3,		[esi + 12]

		punpcklbw		mm0,		mm7
		punpcklbw		mm1,		mm7
		punpcklbw		mm2,		mm7
		punpcklbw		mm3,		mm7

		psrlw		mm0,		mm6
		psrlw		mm1,		mm6
		psrlw		mm2,		mm6
		psrlw		mm3,		mm6

		packuswb		mm0,		mm7
		packuswb		mm1,		mm7
		packuswb		mm2,		mm7
		packuswb		mm3,		mm7

		movd		[edi],		mm0
		movd		[edi + 4],		mm1
		movd		[edi + 8],		mm2
		movd		[edi + 12],		mm3

		add		esi,		16
		add		edi,		16
		dec		eax
		jnz		shader_gamma_shr_loop
		emms

	}
}


/*
	fft_1D_complex_sse

		DESCRIPTION: SSE version of the 1-dimensional 2^m-point FFT, operating in the field of complex numbers.

			The code does four 2^m-point FFTs in parallel. The first FFT is performed on the multiple-of-four indicies
		of the real[] and imag[] arrays. The second FFT is on those indicies, which, divided by 4, yield 1 as a remainder, and
		so on. So if you want to make a 1024-point FFT, you must pass two 4096-element arrays, the first with real data
		and the second with imaginary data.
			There's no way to prevent making four ffts in parallel, but a simple workaround is (persume my_fft_func does
		a n-point fft:)

		void my_fft_func(complex *a, int n, int dir)
		{
			int i;
			float real[MAX_FFT_SIZE], imag[MAX_FFT_SIZE];
			for (i=0;i<n;i++) {
				real[4*i] = a[i].real;
				imag[4*i] = a[i].real;
			}
			FFT_1D_complex_sse(dir, log2(n), real, imag);
			for (i=0;i<n;i++) {
				a[i].real = real[4*i];
				a[i].imag = imag[4*i];
			}
		}

		This would be marginally slower than a dedicated scalar version of the FFT, since scalar arithmetic is not faster
		on the latest CPU's

		INPUT:
			dir - direction of the FFT 1 = forward(decimation in frequency), -1 = backward (decimation in time)
			m   - log2 of number of points
			x   - pointer to real floatingpoint input data, containing 4 * 2^m points
			y   - pointer to imaginary floatingpoint data, containing 4 * 2^m points

		OUTPUT:
			the conversion is done in-place
*/

void fft_1D_complex_sse(int dir, int m, float *x, float *y)
{
   long i,nn,j,l;
   int sdir = dir;
   SSE_ALIGN(float c1[4]);
   SSE_ALIGN(float c2[4]);
   SSE_ALIGN(float  fftconsts[20]) =			{ 1.0,  1.0,  1.0,  1.0,
							  0.0,  0.0,  0.0,  0.0,
							 -1.0, -1.0, -1.0, -1.0,
							  0.5,  0.5,  0.5,  0.5,
							 -2.0, -2.0, -2.0, -2.0
							};

   /* Calculate the number of points */
   nn = 1;
   for (i=0;i<m;i++)
      nn *= 2;

   __asm {
		push	ebx

   		mov		edx,		nn //edx=i2
		dec		nn
		shr		edx,		1
		mov		esi,		[x]
		mov		edi,		[y]
		xor		ebx,		ebx //ebx=j
		xor		eax,		eax //eax=i

	ALIGN	16
	fft_l1:

		cmp		eax,		ebx
		jge		fftb1

		shl		eax,		4
		shl		ebx,		4

		movaps		xmm0,		[esi + eax] // the fastest xchg known to man!
		movaps		xmm1,		[edi + eax]
		movaps		xmm2,		[esi + ebx]
		movaps		xmm3,		[edi + ebx]

		movaps		[esi + ebx],		xmm0
		movaps		[edi + ebx],		xmm1
		movaps		[esi + eax],		xmm2
		movaps		[edi + eax],		xmm3

		shr		eax,		4
		shr		ebx,		4


	ALIGN	16
	fftb1:

		mov		ecx,		edx // ecx=k
	ALIGN	16
	fftwhile1:
		cmp		ecx,		ebx
		jg		fftb2

		sub		ebx,		ecx
		shr		ecx,		1
		jmp		fftwhile1

	ALIGN	16
	fftb2:

		add		ebx,		ecx


		inc		eax
		cmp		eax,		nn
		jb		fft_l1
		inc		nn

		pop		ebx
   }

   /* Compute the FFT */

l = m;

__asm {
		push	ebx
// mapping: eax = i, ebx = i1, ecx = l1, edx = l2. esi = x, edi = y

//initialize:
		mov		esi,		[x]
		mov		edi,		[y]

		movaps		xmm0,		[fftconsts + 32]
		movaps		xmm1,		[fftconsts + 16]
		movaps		[c1],		xmm0 // move -1 to c1
		movaps		[c2],		xmm1 // move  0 to c2

		mov		edx,		1

/* XMM mappings
   0, 1 - accumulators
   2, 3 - u1, u2
   4, 5 - t1, t2
*/
	ALIGN	16
	fft_log2:

		mov		ecx,		edx
		shl		edx,		1

		movaps		xmm2,		[fftconsts] // move 1 to u1
		movaps		xmm3,		[fftconsts + 16] // move 0 to u2
		mov		j,		0     // j = 0

	ALIGN	16
	fft_c1:
		mov		eax,		j // i= j

	ALIGN	16
	fft_c2:

		mov		ebx,		eax
		add		ebx,		ecx // ebx = i+l1

		shl		ebx,		4
		shl		eax,		4

		movaps		xmm0,		[esi + ebx] // xmm0 = x[i1]
		movaps		xmm1,		[edi + ebx] // xmm1 = y[i1]
		movaps		xmm4,		xmm0
		movaps		xmm5,		xmm1

		mulps		xmm0,		xmm3	// xmm0 = x[i1]*u2
		mulps		xmm1,		xmm3	// xmm1 = y[i1]*u2
		mulps		xmm4,		xmm2	// t1 = x[i1]*u1
		mulps		xmm5,		xmm2	// t2 = y[i1]*u1

		subps		xmm4,		xmm1	// t1 = x[i1]*u1 - y[i1]*u2
		addps		xmm5,		xmm0	// t2 = x[i1]*u2 + y[i1]*u1

		movaps		xmm6,		[esi + eax] // xmm6 = x[i]
		movaps		xmm7,		[edi + eax] // xmm7 = y[i]

		movaps		xmm0,		xmm6  // clone to xmm0 and xmm1
		movaps		xmm1,		xmm7

		subps		xmm6,		xmm4 // xmm6 = x[i] - t1
		subps		xmm7,		xmm5 // xmm7 = x[i] - t2
		addps		xmm0,		xmm4 // xmm0 = x[i] + t1
		addps		xmm1,		xmm5 // xmm1 = x[i] + t2

		movaps		[esi + ebx],		xmm6 // x[i1] = x[i] - t1
		movaps		[edi + ebx],		xmm7 // y[i1] = y[i] - t2
		movaps		[esi + eax],		xmm0 // x[i] += t1
		movaps		[edi + eax],		xmm1 // y[i] += t2

		shr		ebx,		4
		shr		eax,		4

		add		eax,		edx
		cmp		eax,		nn
		jl		fft_c2

		movaps		xmm0,		xmm2 // xmm0 == xmm2 == u1
		movaps		xmm1,		xmm3 // xmm1 == xmm3 == u2

/*
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z; (c1 = %2, c2 = %3)
*/
		mulps		xmm2,		[c1] // xmm2 = u1 * c1
		mulps		xmm3,		[c1] // xmm3 = u2 * c1
		mulps		xmm0,		[c2] // xmm0 = u1 * c2
		mulps		xmm1,		[c2] // xmm1 = u2 * c2
		addps		xmm3,		xmm0 // new_u2 = u2 * c1 + u1 * c2
		subps		xmm2,		xmm1 // new_u1 = u1 * c1 - u2 * c2

		inc		j
		cmp		j,		ecx
		jl		fft_c1
/*
      c2 = sqrt((1.0 - c1) * 0.5)
      if (dir == 1)
         c2 = -c2
      c1 = sqrt((1.0 + c1) * 0.5)
*/
		movaps		xmm0,		[fftconsts] // xmm0 = 1.0
		movaps		xmm1,		xmm0 // xmm1 = 1.0
		subps		xmm0,		[c1] // xmm0 = 1.0 - c1
		addps		xmm1,		[c1] // xmm1 = 1.0 + c1
		mulps		xmm0,		[fftconsts + 48] // xmm0 = (1.0 - c1) * 0.5
		mulps		xmm1,		[fftconsts + 48] // xmm1 = (1.0 + c1) * 0.5
		sqrtps		xmm0,		xmm0 // xmm0 ^= 0.5
		sqrtps		xmm1,		xmm1 // xmm1 ^= 0.5

		movaps		[c2],		xmm0	// c2 = sqrt((1.0 - c1) * 0.5)
		movaps		[c1],		xmm1   // c1 = sqrt((1.0 + c1) * 0.5)

		cmp		sdir,		1
		jne		fft_invb

		mulps		xmm0,		[fftconsts + 64] // if (dir == 1) c2 = -c2
		addps		xmm0,		[c2]
		movaps		[c2],		xmm0

	ALIGN	16
	fft_invb:

		dec		l // l--
		jnz		fft_log2 // if l ... jump

   /* Scaling for forward transform */

		cmp		sdir,		1
		jne		fft_nscale

		cvtsi2ss		xmm0,		nn
		movaps		xmm1,		xmm0
		shufps		xmm1,		xmm0,		0 //xmm1 = nn nn nn nn

		movaps		xmm0,		[fftconsts] // xmm0 = 1 1 1 1
		divps		xmm0,		xmm1 // xmm0 = 1/nn 1/nn 1/nn 1/nn
	//	addps		xmm0,		xmm0 // xmm0 = 2/nn 2/nn 2/nn 2/nn

		mov		ecx,		nn
	ALIGN	16
	fft_scale:

		movaps		xmm1,		xmm0
		movaps		xmm2,		xmm0
		mulps		xmm1,		[esi]
		mulps		xmm2,		[edi]
		movaps		[esi],		xmm1
		movaps		[edi],		xmm2

		add		esi,		16
		add		edi,		16

		dec		ecx
		jnz		fft_scale

	ALIGN	16
	fft_nscale:

		emms

/*
	FFT Constants:
	offset		0	16	32	48	64
	constant	+1	 0	-1	0.5	-2
*/
	pop	ebx

}

}

/*
	this function replaces the following code:
      		for (i=0;i<fft_size;i++) {
         		real[4*i  ] = c[i][4*j  ].re;
         		imag[4*i  ] = c[i][4*j  ].im;
         		real[4*i+1] = c[i][4*j+1].re;
         		imag[4*i+1] = c[i][4*j+1].im;
         		real[4*i+2] = c[i][4*j+2].re;
         		imag[4*i+2] = c[i][4*j+2].im;
         		real[4*i+3] = c[i][4*j+3].re;
         		imag[4*i+3] = c[i][4*j+3].im;
      		}
*/
__declspec(noinline) void float_copy_ij_i(float *a, float *b, complex c[], int fft_size)
{
	__asm {
		push	ebx
		mov		eax,		fft_size
	//	mov		edx,		eax
	//	shl		edx,		3
		mov		esi,		[a]
		mov		edi,		[b]
		mov		ebx,		c

	ALIGN	16
	copy_ij_i_loop:

		movaps		xmm0,		[ebx] // xmm0 = re[0] im[0] re[1] im[1]
		movaps		xmm1,		[ebx + 16] // xmm1 = re[2] im[2] re[3] im[3]
		movaps		xmm2,		xmm0	  // xmm2 = re[0] im[0] re[1] im[1]
		movaps		xmm3,		xmm1	  // xmm3 = re[2] im[2] re[3] im[3]
		shufps		xmm0,		xmm1,		0x88 // 10 00 10 00 // xmm0 = re[0] re[1] re[2] re[3]
		shufps		xmm2,		xmm3,		0xdd // 11 01 11 01 // xmm2 = im[0] im[1] im[2] im[3]
		movaps		[esi],		xmm0
		movaps		[edi],		xmm2

		add		ebx,		8192
		add		esi,		16
		add		edi,		16

		dec		eax
		jnz		copy_ij_i_loop

		emms
		pop		ebx
	}

}

/*
	this function replaces the following code:
      		for (i=0;i<fft_size;i++) {
         		 c[i][4*j  ].re = real[4*i  ];
         		 c[i][4*j  ].im = imag[4*i  ];
         		 c[i][4*j+1].re = real[4*i+1];
         		 c[i][4*j+1].im = imag[4*i+1];
         		 c[i][4*j+2].re = real[4*i+2];
         		 c[i][4*j+2].im = imag[4*i+2];
         		 c[i][4*j+3].re = real[4*i+3];
         		 c[i][4*j+3].im = imag[4*i+3];
      		}
*/
__declspec(noinline) void float_copy_i_ij(float *a, float *b, complex c[], int fft_size)
{
	__asm {
		push	ebx
		mov		eax,		fft_size
	//	mov		edx,		eax
	//	shl		edx,		3
		mov		esi,		[a]
		mov		edi,		[b]
		mov		ebx,		c

	ALIGN	16
	copy_i_ij_loop:

		movaps		xmm0,		[esi] // xmm0 = re[0] re[1] re[2] re[3]
		movaps		xmm1,		[edi] // xmm1 = im[0] im[1] im[2] im[3]
		movaps		xmm2,		xmm0	  // xmm2 = re[0] re[1] re[2] re[3]
		movaps		xmm3,		xmm1	  // xmm3 = im[0] im[1] im[2] im[3]

		unpcklps		xmm0,		xmm1 // xmm0 = re[0] im[0] re[1] im[1]
		unpckhps		xmm2,		xmm3 // xmm2 = re[2] im[2] re[3] im[3]

		movaps		[ebx],		xmm0
		movaps		[ebx + 16],		xmm2

		add		ebx,		8192
		add		esi,		16
		add		edi,		16

		dec		eax
		jnz		copy_i_ij_loop

		emms

		pop		ebx
	}

}


/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/


/*
	this function replaces the following code:
      		for (j=0;j<fft_size;k++) {
         		real[4*j  ] = c[4*i  ][j].re;
         		imag[4*j  ] = c[4*i  ][j].im;
         		real[4*j+1] = c[4*i+1][j].re;
         		imag[4*j+1] = c[4*i+1][j].im;
         		real[4*j+2] = c[4*i+2][j].re;
         		imag[4*j+2] = c[4*i+2][j].im;
         		real[4*j+3] = c[4*i+3][j].re;
         		imag[4*j+3] = c[4*i+3][j].im;
      		}
*/
__declspec(noinline) void float_copy_ij_j(float *a, float *b, complex c[], int fft_size)
{
	__asm {
		push	ebx
		mov		eax,		fft_size
		mov		esi,		[a]
		mov		edi,		[b]
		mov		ebx,		c

	ALIGN	16
	copy_ij_j_loop:

		movlps		xmm0,		[ebx]
		movhps		xmm0,		[ebx + 8192] // xmm0 = re[0] im[0] re[1] im[1]
		movlps		xmm1,		[ebx + 16384]
		movhps		xmm1,		[ebx + 24576] // xmm1 = re[2] im[2] re[3] im[3]
		movaps		xmm2,		xmm0	  // xmm2 = re[0] im[0] re[1] im[1]
		movaps		xmm3,		xmm1	  // xmm3 = re[2] im[2] re[3] im[3]
		shufps		xmm0,		xmm1,		0x88 // 10 00 10 00 // xmm0 = re[0] re[1] re[2] re[3]
		shufps		xmm2,		xmm3,		0xdd // 11 01 11 01 // xmm2 = im[0] im[1] im[2] im[3]
		movaps		[esi],		xmm0
		movaps		[edi],		xmm2

		add		ebx,		8
		add		esi,		16
		add		edi,		16

		dec		eax
		jnz		copy_ij_j_loop

		emms
	
		pop		ebx
	}

}


/*
	this function replaces the following code:
      		for (j=0;j<fft_size;k++) {
         		 c[4*i  ][j].re = real[4*j  ];
         		 c[4*i  ][j].im = imag[4*j  ];
         		 c[4*i+1][j].re = real[4*j+1];
         		 c[4*i+1][j].im = imag[4*j+1];
         		 c[4*i+2][j].re = real[4*j+2];
         		 c[4*i+2][j].im = imag[4*j+2];
         		 c[4*i+3][j].re = real[4*j+3];
         		 c[4*i+3][j].im = imag[4*j+3];
      		}
*/

__declspec(noinline) void float_copy_j_ij(float *a, float *b, complex c[], int fft_size)
{
	__asm {
		push	ebx
		mov		eax,		fft_size
		mov		esi,		[a]
		mov		edi,		[b]
		mov		ebx,		c

	ALIGN	16
	copy_j_ij_loop:

		movaps		xmm0,		[esi] // xmm0 = re[0] re[1] re[2] re[3]
		movaps		xmm1,		[edi] // xmm1 = im[0] im[1] im[2] im[3]
		movaps		xmm2,		xmm0	  // xmm2 = re[0] re[1] re[2] re[3]
		movaps		xmm3,		xmm1	  // xmm3 = im[0] im[1] im[2] im[3]

		unpcklps		xmm0,		xmm1 // xmm0 = re[0] im[0] re[1] im[1]
		unpckhps		xmm2,		xmm3 // xmm2 = re[2] im[2] re[3] im[3]

		movlps		[ebx],		xmm0
		movhps		[ebx + 8192],		xmm0
		movlps		[ebx + 16384],		xmm2
		movhps		[ebx + 24576],		xmm2

		add		ebx,		8
		add		esi,		16
		add		edi,		16

		dec		eax
		jnz		copy_j_ij_loop

		emms
	
		pop		ebx
	}

}


__declspec(noinline) void shader_spill_mmx(Uint8 *src, Uint8 * dest, int resx, int resy, float coeff)
{
	SSE_ALIGN(unsigned short vcoeff[4]);
	SSE_ALIGN(unsigned short vncoeff[4]);
	SSE_ALIGN(unsigned short firstok[4]) = {0xffff,0,0,0};
	SSE_ALIGN(unsigned short lastok[4]) = {0,0,0,0xffff};
	vcoeff[0] = (unsigned short) (255 * coeff);
	vncoeff[0] = (unsigned short) (255 * (1-coeff));
	for (int i = 1; i < 4; i++) {
		vcoeff[i] = vcoeff[0];
		vncoeff[i] = vncoeff[0];
	}
	__asm {
//mmx init:
		pxor	mm5,	mm5
		movq	mm6,	[vcoeff]
		movq	mm7,	[vncoeff]


/* HORIZONTAL PASSES (IA32) */
		mov	esi,	[src]
		mov	edi,	[dest]
		mov	edx,	[resy]
f_y_0:
		mov	ecx,	[resx]
		shr	ecx,	2
		pxor	mm0,	mm0
ALIGN 16
f_x_0:
		movd	mm1,	[esi]
		punpcklbw	mm1,	mm5
		mov	eax,	4
ALIGN 16
quad_loop_0:
		movq	mm3,	[firstok]
		pand	mm3,	mm1
		pmullw	mm0,	mm7
		pmullw	mm3,	mm6
		paddusw	mm0,	mm3
		psllq	mm0,	48
		psrlq	mm1,	16
		por		mm1,	mm0
		psrlq	mm0,	56
		dec	eax
		jnz	quad_loop_0
		//
		psrlw	mm1,	8
		packuswb	mm1,	mm5
		movd	[edi],	mm1
        add	esi,	4
		add	edi,	4
		dec	ecx
		jnz	f_x_0
// REVERSE:
		mov	ecx,	[resx]
		shr	ecx,	2
		pxor	mm0,	mm0
ALIGN 16
f_x_1:
		sub	edi,	4
		movd	mm1,	[edi]
		punpcklbw	mm1,	mm5
		mov	eax,	4
ALIGN 16
quad_loop_1:
		movq	mm3,	[lastok]
		pand	mm3,	mm1
		pmullw	mm0,	mm7
		pmullw	mm3,	mm6
		paddusw	mm0,	mm3
		psrlq	mm0,	48
		psllq	mm1,	16
		por		mm1,	mm0
		psllq	mm0,	40
		dec	eax
		jnz	quad_loop_1
		//
		psrlw	mm1,	8
		packuswb	mm1,	mm5
		movd	[edi],	mm1
		dec	ecx
		jnz	f_x_1

		//
		add	edi,	[resx]
		dec	edx
		jnz	f_y_0


/* VERTICAL PASSES (MMX) */

//addr init:
		mov	edi,	[dest]
		mov	ecx,	[resx]
		shr	ecx,	2
		mov	esi,	[resx]
f_x_2:
		mov	edx,	2
ALIGN 16
negloop:
		movd	mm0,	[edi]
		punpcklbw	mm0,	mm5
		mov	eax,	[resy]
		dec	eax
ALIGN 16
f_y_1:
		add	edi,	esi
		movd	mm1,	[edi]
		pmullw	mm0,	mm7
		punpcklbw	mm1,	mm5
		pmullw	mm1,	mm6
		paddusw	mm0,	mm1
		psrlw	mm0,	8
		movq	mm2,	mm0
		packuswb	mm2,	mm5
		movd	[edi],	mm2
		//
		dec	eax
		jnz	f_y_1
		//
		neg	esi
		dec	edx
		jnz	negloop
		//
		add	edi,	4
		dec	ecx
		jnz f_x_2

		emms
	}
}

void shader_fbmerge_mmx2(Uint32 *dest, Uint8 * src, int resx, int resy, float intensity, Uint32 glow_color)
{
	SSE_ALIGN(unsigned short vint[4]);
	SSE_ALIGN(unsigned short vnint[4]);
	SSE_ALIGN(unsigned short glowcol[4]);
	vint[0] = (int)(65535* intensity);
	vnint[0] = 65535;
	glowcol[0] = (glow_color<<8) & 0xff00;
	glowcol[1] = (glow_color   ) & 0xff00;
	glowcol[2] = (glow_color>>8) & 0xff00;
	for (int i = 1; i < 4; i++) {
		vint[i] = vint[0];
		vnint[i] = vnint[0];
	}
	__asm {
		pxor	mm7,	mm7
		movq	mm4,	[glowcol]
		movq	mm5,	[vint]
		movq	mm6,	[vnint]
		
		mov	eax,	[resx]
		mov	edx,	[resy]
		mul	edx
		shr	eax,	2
		mov	esi,	[src]
		mov	edi,	[dest]

/*op:
		int mult1 = (int) (((src[i] & 0xff000000) >> 16)*intensity);
		dest[i] = multiplycolor(dest[i], 65535 - mult1)  + multiplycolor(glow_color, mult1);
*/
	ALIGN 16
	pix_loop_1:
		movd	mm3,	[esi]
		
		punpcklbw	mm3,	mm7
		psllw	mm3,	8
		pmulhuw	mm3,	mm5

//0:
		movd	mm1,	[edi]
		punpcklbw	mm1,	mm7
		pshufw	mm0,	mm3,	0x00
		psllw	mm1,	8
		movq	mm2,	mm6
		psubw	mm2,	mm0
		pmulhuw	mm1,	mm2
		pmulhuw	mm0,	mm4 
		paddusw	mm0,	mm1
		psrlw	mm0,	8
		packuswb	mm0,	mm7
		movd	[edi],	mm0
//1:
		movd	mm1,	[edi+4]
		punpcklbw	mm1,	mm7
		pshufw	mm0,	mm3,	0x55
		psllw	mm1,	8
		movq	mm2,	mm6
		psubw	mm2,	mm0
		pmulhuw	mm1,	mm2
		pmulhuw	mm0,	mm4 
		paddusw	mm0,	mm1
		psrlw	mm0,	8
		packuswb	mm0,	mm7
		movd	[edi+4],	mm0
//2:
		movd	mm1,	[edi+8]
		punpcklbw	mm1,	mm7
		pshufw	mm0,	mm3,	0xaa
		psllw	mm1,	8
		movq	mm2,	mm6
		psubw	mm2,	mm0
		pmulhuw	mm1,	mm2
		pmulhuw	mm0,	mm4 
		paddusw	mm0,	mm1
		psrlw	mm0,	8
		packuswb	mm0,	mm7
		movd	[edi+8],	mm0
//3:
		movd	mm1,	[edi+12]
		punpcklbw	mm1,	mm7
		pshufw	mm0,	mm3,	0xff
		psllw	mm1,	8
		movq	mm2,	mm6
		psubw	mm2,	mm0
		pmulhuw	mm1,	mm2
		pmulhuw	mm0,	mm4 
		paddusw	mm0,	mm1
		psrlw	mm0,	8
		packuswb	mm0,	mm7
		movd	[edi+12],	mm0

		//
		add	esi,	4
		add	edi,	16

		dec	eax
		jnz	pix_loop_1

		emms
	}
}
void shader_sobel_sse(Uint32 *src, Uint32 *dest, int resx, int resy, const int hk[], const int vk[])
{
	SSE_ALIGN(short int h_kernel[9][4]);
	SSE_ALIGN(short int v_kernel[9][4]);
	SSE_ALIGN(float two_pow_m16[4]) = {
		0.0000152587890625,0.0000152587890625,0.0000152587890625,0.0000152587890625};

	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 4; j++) {
			h_kernel[i][j] = (short int) hk[i];
			v_kernel[i][j] = (short int) vk[i];
		}
	}
	for (int i = 0; i < resx; i++) dest[i] = dest[(resy-1)*resx+i] = 0;
	src += resx;
	dest += resx;
	resy -= 2;
	//
	__asm {
		pxor	mm7,	mm7
		mov	esi,	src
		mov	edi,	dest
		xor	edx,	edx
		movaps	xmm7,	two_pow_m16
		
		ALIGN 16
	yloop:
		mov	eax,	2
		mov	[edi],	0
		add	esi,	4
		add	edi,	4
		
		ALIGN 16
	xloop:

		push	eax

		mov	ecx,	3
		mov	eax,	resx
		neg	eax
		lea	esi,	[esi+eax*4-4]
		pxor	mm0,	mm0
		pxor	mm1,	mm1
		
		ALIGN	16
	yiloop:

		lea	eax,	[ecx*8-8] 
		lea	eax,	[eax + eax*2] //eax = (ecx-1)*24

		/* 1 */
		movq	mm4,	[h_kernel+eax]
		movq	mm5,	[v_kernel+eax]
		movd	mm6,	[esi]

		punpcklbw	mm6,	mm7
		pmullw	mm4,	mm6
		pmullw	mm5,	mm6
		paddw	mm0,	mm4
		paddw	mm1,	mm5
		/* 2 */
		movq	mm4,	[h_kernel+eax+8]
		movq	mm5,	[v_kernel+eax+8]
		movd	mm6,	[esi+4]

		punpcklbw	mm6,	mm7
		pmullw	mm4,	mm6
		pmullw	mm5,	mm6
		paddw	mm0,	mm4
		paddw	mm1,	mm5
		/* 3 */
		movq	mm4,	[h_kernel+eax+16]
		movq	mm5,	[v_kernel+eax+16]
		movd	mm6,	[esi+8]

		punpcklbw	mm6,	mm7
		pmullw	mm4,	mm6
		pmullw	mm5,	mm6
		paddw	mm0,	mm4
		paddw	mm1,	mm5
		// row ready

		mov	eax,	resx
		lea	esi,	[esi+eax*4]
		dec	ecx
		jnz	yiloop
		// result ready in mm0 and mm1
		movq	mm2,	mm0
		movq	mm3,	mm1

		// convert to dwords
		punpcklwd	mm0,	mm7
		punpcklwd	mm1,	mm7
		punpckhwd	mm2,	mm7
		punpckhwd	mm3,	mm7

		// shift 'em, so that sign is preserved:
		pslld	mm0,	16
		pslld	mm1,	16
		pslld	mm2,	16
		pslld	mm3,	16

		//transfer to SSE
		cvtpi2ps	xmm0,	mm0
		cvtpi2ps	xmm1,	mm1
		cvtpi2ps	xmm2,	mm2
		cvtpi2ps	xmm3,	mm3

		// repackage in xmm. Xmm0 + Xmm2, Xmm1 + Xmm3
		movlhps	xmm0,	xmm2
		movlhps	xmm1,	xmm3

		// perform ops
		mulps	xmm0,	xmm0
		mulps	xmm1,	xmm1
		addps	xmm0,	xmm1

		// Square root

		//sqrtps	xmm0,	xmm0
		//using these instead of sqrtps
		addps	xmm0,	xmm7
		rsqrtps	xmm0,	xmm0
		rcpps	xmm0,	xmm0

		// Negate the effect of the MMX shifting by 16 position: multiply by 2^-16
		mulps	xmm0,	xmm7
		
		// unpackage
		movhlps	xmm1,	xmm0

		// transfer back to MMX
		cvtps2pi	mm0,	xmm0
		cvtps2pi	mm1,	xmm1

		// truncate to 16bit with saturation
		packssdw	mm0,	mm1

		// truncate to 8bit with saturation
		packuswb	mm0,	mm7

		// push the result
		movd	[edi],	mm0

		mov	eax,	resx
		neg	eax
		lea	esi,	[esi+eax*8+4]
		pop	eax
		// end of x loop
		add	esi,	4
		add	edi,	4
		inc	eax
		cmp	eax,	resx
		jb	xloop

		// end of y loop
		mov	[edi],	0
		add	edi,	4
		add	esi,	4
		inc	edx
		cmp	edx,	resy
		jb	yloop

		emms
	}
}

#endif
#ifdef apply_fft_filter_asm
void apply_fft_filter_sse(complex *dest, float *filter, int fft_size)
{
	if (!filter) return;
	fft_size = fft_size * fft_size / 4;
	//do {
	//	dest->re *= (*filter);
	//	dest->im *= (*filter);
	//	dest++; filter++;
		__asm {

			mov			ecx,		fft_size;
			mov			edi,		[dest]
			mov			esi,		[filter]
applyfftfilterloop:

			movups		xmm0,		[esi] // f1 f2 f3 f4
			movaps		xmm1,		xmm0 // f1 f2 f3 f4
			movaps		xmm2,		xmm0 // f1 f2 f3 f4

			shufps		xmm1,		xmm0,		0x50 // 01 01 00 00 = f1 f1 f2 f2
			shufps		xmm2,		xmm0,		0xfa // 11 11 10 10 = f3 f3 f4 f4

			mulps		xmm1,		[edi]
			mulps		xmm2,		[edi + 16]

			movaps		[edi],		xmm1
			movaps		[edi + 16],		xmm2

			add			edi,	32
			add			esi,	16
			dec			ecx
			jnz			applyfftfilterloop

			emms
		}
	//	dest+=4; filter+=4;
	//}while (--fft_size);
}
#endif
#ifdef bilinea_p5_asm
	__asm {

		pxor		mm7,		mm7
		movd		mm2,		x0y0
		mov		eax,		Fx
		mov		ebx,		Fy

		punpcklbw		mm2,		mm7

		mov		cl,		255
		mov		dl,		255

		movd		mm3,		x1y0

		sub		cl,		al
		sub		dl,		bl

		punpcklbw		mm3,		mm7

// ax = Fx, bx = Fy. cx = 1-Fx, dx = 1-Fy

// ax = 	Fx	1-Fx	  Fx	1-Fx
// bx = 	Fy	  Fy	1-Fy	1-Fy

		mov		ch,		al
		mov		bh,		bl

		movd		mm4,		x0y1

		mov		ax,		cx
		shl		ebx,		16

		punpcklbw		mm4,		mm7

		mov		dh,		dl
		shl		eax,		16
		movd		mm5,		x1y1
		mov		bx,		dx // bx ready
		mov		ax,		cx // ax ready

		punpcklbw		mm5,		mm7

		movd		mm1,		ebx
		movd		mm0,		eax
		mov		edx,		0xffff

		punpcklbw		mm1,		mm7
		punpcklbw		mm0,		mm7

		movd		mm6,		edx //mm6 = 0xffff

		pmullw		mm0,		mm1
		psrlw		mm0,		8
//1:
		movq		mm7,		mm6
		movq		mm1,		mm6
		psllq		mm7,		16

		pand		mm6,		mm0
		pand		mm1,		mm0

		psllq		mm6,		16
		por		mm1,		mm6
		psllq		mm6,		16
		por		mm1,		mm6

		pmullw		mm2,		mm1
//2:
		movq		mm6,		mm7
		movq		mm1,		mm7
		psllq		mm7,		16

		pand		mm6,		mm0
		pand		mm1,		mm0
		psllq		mm6,		16
		por		mm1,		mm6
		psrlq		mm6,		32
		por		mm1,		mm6

		pmullw		mm3,		mm1
//3:
		movq		mm6,		mm7
		movq		mm1,		mm7
		psllq		mm7,		16

		pand		mm6,		mm0
		pand		mm1,		mm0
		psrlq		mm6,		16
		por		mm1,		mm6
		psrlq		mm6,		16
		por		mm1,		mm6

		pmullw		mm4,		mm1
//4:
		movq		mm6,		mm7
		movq		mm1,		mm7
		pxor		mm7,		mm7

		pand		mm6,		mm0
		pand		mm1,		mm0
		psrlq		mm6,		16
		psrlq		mm1,		48
		por		mm1,		mm6
		psrlq		mm6,		16
		por		mm1,		mm6

		pmullw		mm5,		mm1

		paddusw		mm3,		mm2
		paddusw		mm5,		mm4
		paddusw		mm5,		mm3
		psrlw		mm5,		8

		packuswb		mm5,		mm7
		movd		rez,		mm5

		emms

	}
#endif

#ifdef common_asm
/* THIS VERSION IS DUE TO barney. */

/*
(
|------|------| ^
|      |      | |
| x0y0 | x1y0 | |
|      |      | |
|------|------| |  y
|      |      | |
| x0y1 | x1y1 | |
|      |      | |
|------|------| V

<------------->
       x

The Formula is identical to:
result = MultiplyColor(x0y0, (1-x)*(1-y)) + MultiplyColor(x0y0, (  x)*(1-y)) +
	 MultiplyColor(x0y1, (1-x)*(  y)) + MultiplyColor(x1y1, (  x)*(  y));
)
Requires SSE. Very fast :)
*/

Uint32 bilinea4(Uint32 x0y0, Uint32 x1y0, Uint32 x0y1, Uint32 x1y1, int x, int y)
{	Uint32 xx_result;
//    __m64 mm;
	__asm {

		pxor		mm6,		mm6
		movd		mm0,		x0y0
		mov		eax,		65535
		mov		edi,		y
		movd		mm1,		x1y0
		mov		ecx,		65535
		sub		ecx,		edi
		movd		mm2,		x0y1
		shl		edi,		16
		mov		esi,		x
		movd		mm3,		x1y1
		sub		eax,		esi
		shl		esi,		16
		punpcklbw		mm0,		mm6
		or		ecx,		edi
		or		eax,		esi
		punpcklbw		mm1,		mm6
		movd		mm5,		eax
		movd		mm7,		ecx
		punpcklbw		mm2,		mm6
		punpcklbw		mm3,		mm6
		pshufw		mm7,		mm7,		80
		pshufw		mm5,		mm5,		68
		pmulhuw		mm7,		mm5
		pshufw		mm4,		mm7,		0
		pshufw		mm5,		mm7,		85
		pmulhuw		mm0,		mm4
		pshufw		mm6,		mm7,		170
		pmulhuw		mm1,		mm5
		pshufw		mm7,		mm7,		255
		pmulhuw		mm2,		mm6
		paddw		mm0,		mm1
		pmulhuw		mm3,		mm7
		paddw		mm2,		mm3
		paddw		mm0,		mm2
		packuswb		mm0,		mm0
		movd		xx_result,		mm0
		emms

	}
	return xx_result;
}
#endif

#ifdef blur_asm

/*********************************************************
 * MMX blur routines:                                    *
 * blur_forward_mmx, blur_backward_mmx, buffer_plus_mmx, *
 * and buffer_minus_mmx works just like their non-mmx    *
 * counterparts, except for saturation arithmetic        *
 *********************************************************/

// note: count must be divisable by 8
void blur_forward_mmx(Uint32 *dest, Uint32 *src, int count)
{
	SSE_ALIGN(int andval[6]) = {
			0x000000ff, 0x000000ff,
			0x0000ff00, 0x0000ff00,
			0x00f80000, 0x00f80000
	};
	// load this useful value:
	__asm {

		movq		mm6,		[andval + 8]
		movq		mm7,		[andval + 16]
	
	}
	count /= 8; // we are doing 8 pixels per cycle :)
	//while (count--) {
		__asm {

			push		esi
			push		edi
			mov			esi,	src
			mov			edi,	dest
			mov			ecx,	count

			ALIGN	16
	UaLoop:
			movq		mm0,		[esi]
			movq		mm3,		[esi + 8]
			movq		mm4,		mm6
			movq		mm5,		mm7
			movq		mm1,		mm0
			movq		mm2,		mm0

			pand		mm0,		[andval]
			pand		mm4,		mm3
			pand		mm5,		mm3
			pand		mm3,		[andval]
			pand		mm1,		mm6
			pand		mm2,		mm7

			pslld		mm4,		3
			pslld		mm5,		5

			pslld		mm1,		3
			pslld		mm2,		5

			por		mm4,		mm5
			movq		mm5,		[esi + 24]
			por		mm1,		mm2
			movq		mm2,		[esi + 16]
/* REGISTER ALLOCATION:
	mm0	- blue  of point0 + orig of point0
	mm1	- R+G   of point0
	mm2	- third point
	mm3	- blue  of point1 + orig of point1
	mm4	- R+G   of point1
	mm5	- fourth point
*/
			paddd		mm3,		mm4
			movq		mm4,		mm7
			paddd		mm0,		mm1
			movq		mm1,		mm7
			movq		[edi + 8],		mm3
			movq		[edi],		mm0
// 1 & 2 done;

			pand		mm4,		mm5
			movq		mm3,		mm5
			pand		mm5,		[andval]
			movq		mm0,		mm2
			pand		mm1,		mm2
			pand		mm2,		[andval]
			pand		mm3,		mm6
			pand		mm0,		mm6
			pslld		mm4,		5
			pslld		mm3,		3
			pslld		mm1,		5
			pslld		mm0,		3
			paddd		mm3,		mm4
			paddd		mm0,		mm1
			paddd		mm5,		mm3
			paddd		mm2,		mm0
			movq		[edi + 24],		mm5
			movq		[edi + 16],		mm2

			add			esi,		32
			add			edi,		32
			dec			ecx
			jnz			UaLoop

			pop			edi
			pop			esi
		
			emms
		}
		//src+=8; dest+=8;
	//}
}

// note: count must be divisable by 2
void blur_backward_mmx(Uint32 *dest, Uint32 *src, int count)
{
	SSE_ALIGN(int shifted_8bits[6]) = {
			0x000007f8, 0x000007f8,
			0x003fc000, 0x003fc000,
			0xff000000, 0xff000000
	};
	count /= 2;
	__asm {

			pxor		mm4,		mm4
			movq		mm5,		[shifted_8bits]
			movq		mm6,		[shifted_8bits + 8]
			movq		mm7,		[shifted_8bits + 16]
		
	}


	//while (count--) {
		__asm {

			push		esi
			push		edi
			mov			esi,		src
			mov			edi,		dest
			mov			ecx,		count

ALIGN 16
UbLoop:

			movq		mm0,		[esi]
			movq		mm1,		mm6
			movq		mm2,		mm7
			pand		mm1,		mm0
			pand		mm2,		mm0
			pand		mm0,		mm5

			psrld		mm1,		6
			psrld		mm2,		8
			psrld		mm0,		3
			por		mm0,		mm1
			por		mm0,		mm2

			movq		[edi],		mm0

			add			esi,		8
			add			edi,		8
			dec			ecx
			jnz			UbLoop
			pop			edi
			pop			esi

			emms
		
		}
		//dest+=2; src+=2;
	//}
}

// note: count must be divisable by 8
void buffer_minus_mmx(Uint32 *dest, Uint32 *src, int count)
{
	count /= 8;
		__asm {

			push		esi
			push		edi
			mov			esi,		src
			mov			edi,		dest
			mov			ecx,		count

ALIGN 16
UcLoop:

			movq		mm0,		[edi]
			movq		mm1,		[edi + 8]
			movq		mm2,		[edi + 16]
			movq		mm3,		[edi + 24]
			psubd		mm0,		[esi]
			psubd		mm1,		[esi + 8]
			psubd		mm2,		[esi + 16]
			psubd		mm3,		[esi + 24]
			movq		[edi],		mm0
			movq		[edi + 8],		mm1
			movq		[edi + 16],		mm2
			movq		[edi + 24],		mm3

			add			esi,		32
			add			edi,		32
			dec			ecx
			jnz			UcLoop

			pop			edi
			pop			esi
			
			emms
		
		}
}

// note: count must be divisable by 8
void buffer_plus_mmx(Uint32 *dest, Uint32 *src, int count)
{
	count /= 8;
//	do {
		__asm {

			push		esi
			push		edi
			mov			esi,		src
			mov			edi,		dest
			mov			ecx,		count

ALIGN 16
UdLoop:
			movq		mm0,		[edi]
			movq		mm1,		[edi + 8]
			movq		mm2,		[edi + 16]
			movq		mm3,		[edi + 24]
			paddd		mm0,		[esi]
			paddd		mm1,		[esi + 8]
			paddd		mm2,		[esi + 16]
			paddd		mm3,		[esi + 24]
			movq		[edi],		mm0
			movq		[edi + 8],		mm1
			movq		[edi + 16],		mm2
			movq		[edi + 24],		mm3

			add			esi,		32
			add			edi,		32
			dec			ecx
			jnz			UdLoop

			pop			edi
			pop			esi

			emms
		}
//		dest += 8; src += 8;
//	} while (--count);
}

#endif


#ifdef fract_asm
// contains some of fract.cpp's source code (merely all that is in assembly)

float w2_22 = 25.0;
float w2_shl16 = 65536.0;
float w2_22_16=w2_22*65536.0;
float w2_22_8 =w2_22*256.0;

// this is the integrated version of multiplycolor and lform2 - does the whole thing in a single routine
// input:  raw color from the texture. The x and y coord of the destination (where the ray hit the floor/ceiling).
//         The Lx, Ly coordinates of the light source
// output: the calculated color that should be printed to the screen
// The routine requires SSE instruction set. Supported CPUs: Pentium III and above (Intel); Athlon XP/Duron Morgan or above (AMD)
// This one uses some ideas and optimizations by barnie.

static inline Uint32	igrated(Uint32 color, int x, int y, int lx, int ly, int ysq)
{
	Uint32 xx_result;
 __asm {
  // perform the light degradation math -> color multiplyer = const * ((x-lx)^2+(y-ly)^2)^-0.25
 	mov		eax,		x
 	mov		ecx,		y
	cvtsi2ss	xmm4,	ysq
 	sub		eax,		lx
 	sub		ecx,		ly

 	movss		xmm2,		w2_22_8 // move the constant to xmm2
 	cvtsi2ss		xmm0,		eax // ...try to exploit parallelism
 	cvtsi2ss		xmm1,		ecx
 	mulss		xmm0,		xmm0
 	mulss		xmm1,		xmm1
 	addps		xmm4,		xmm0
	addps		xmm4,		xmm1 // xmm4 = dist^2

// the following is the barnie's idea of obtaining  const / dist^0.5
 	rsqrtss		xmm4,		xmm4 // xmm4 = 1/dist
 	mulss		xmm2,		xmm4 // xmm2 = const / dist
 	rsqrtss		xmm4,		xmm4 // xmm4 = sqrt(dist)
 	mulss		xmm2,		xmm4 // xmm2 = const / sqrt(dist)
 //	load color in mm0: (via mmx):

 // note: we switch to MMX here:
 	pxor		mm0,		mm0
 	movd		mm1,		color
 	punpcklbw		mm0,		mm1

 	shufps		xmm2,		xmm2,		0
 	cvtps2pi		mm2,		xmm2
 	pxor		mm4,		mm4
 	movq		mm3,		mm2
 	packssdw		mm2,		mm3

 	pmulhuw		mm0,		mm2

 	packuswb		mm0,		mm4
 	movd		xx_result,		mm0
 	emms
 
 }
	return xx_result;
}
#endif


#ifdef genrender_asm
/*********************************************************
 * blur_do_MMX: does the `blur' effect using MMX; should *
 * be about two times faster on PIII even more on P4     *
 * NOTE: count must be DIVISABLE by 8!                   *
 * Works by getting six points and processing them and   *
 * then gets additional two points and processes them as *
 * well.                                                 *
 *********************************************************/

void blur_do_MMX(Uint32 *bb, Uint32 *src, Uint32 *dst, int count, int kopy)
{
	int andval[2]= {0x1f1f1f1f, 0x1f1f1f1f};
	int cs = count;
	// load this useful value:
	__asm {

		movq		mm7,		[andval]
	
	}
	count /= 8; // we are doing 8 pixels per cycle :)
	//while (count--) {
		__asm {

			mov			ecx,		count
			mov			esi,		[src]
			mov			edi,		[bb]
ALIGN 16
blurloop:
			movq		mm0,		[esi]
			movq		mm1,		[edi]
			movq		mm2,		[esi + 8]
			movq		mm3,		[edi + 8]
			movq		mm4,		[esi + 16]
			movq		mm5,		[edi + 16]

			psrld		mm0,		3
			psrld		mm2,		3
			psrld		mm4,		3

			pand		mm0,		mm7
			pand		mm2,		mm7
			pand		mm4,		mm7

			paddd		mm1,		mm0
			paddd		mm3,		mm2
			paddd		mm5,		mm4

			movq		mm0,		[esi + 24]
			movq		mm2,		[edi + 24]
			psrld		mm0,		3
			pand		mm0,		mm7
			paddd		mm0,		mm2
			movq		[edi],		mm1
			movq		[edi + 8],		mm3
			movq		[edi + 16],		mm5
			movq		[edi + 24],		mm0

	
			add			esi,		32
			add			edi,		32
			dec			ecx
			jnz			blurloop
		}
	//	src+=8; bb+=8;
	//}
	if (kopy) {
		bb-=(cs);
		count = cs/2;
		//while (count--) {
			__asm {
				mov			ecx,	count
				mov			edi,	[dst]
				mov			esi,	[bb]
				pxor		mm1,	mm1
ALIGN 16
blurcopyloop:
				movq		mm0,		[esi]
				movq		[edi],		mm0
				movq		[esi],		mm1
		
				add			edi,	8
				add			esi,	8
				dec			ecx
				jnz			blurcopyloop
			}
			//dst+=2; bb+=2;
		//}
	}
	__asm {
		emms
	}
}

/*********************************************************************
 *  MMX version of the merging function, which works on a single row *
 *-------------------------------------------------------------------*
 * INPUT:  `flr' is the pixel buffer from the floor engine           *
 *         `sph' is the pixel buffer from the sphere engine          *
 *         `multi' is an array of multipliers in 16.16 fixedpoint    *
 *             format. A value of 16384 for example, means, to blend *
 *             1/4 of the sphere pixel with 3/4 of the floor pixel   *
 *         `count' : how many pixels, must be divisable by 2.        *
 *********************************************************************/

void merge_rows(Uint32 *flr, Uint32 *sph, unsigned short *multi, int count)
{
	SSE_ALIGN(const unsigned short ones[4]) = {0xffff, 0xffff, 0xffff, 0xffff};
	SSE_ALIGN(const unsigned short  bla[4]) = {0x0001, 0x0001, 0x0001, 0x0001};
	// load one useful value:
	__asm {
	movq		mm7,		[ones]
	}
	count /= 2;
	//while (count--) {
		__asm {
			mov			ecx,		count
			mov			esi,		[flr]
			mov			edi,		[sph]
			mov			eax,		[multi]
ALIGN 16
pixloop:
			pxor		mm6,		mm6
			movq		mm0,		[esi]
			movq		mm2,		[edi]
			movd		mm4,		[eax]
			movq		mm1,		mm0
			movq		mm3,		mm2
			punpcklbw		mm0,		mm6
			punpckhbw		mm1,		mm6
			punpcklbw		mm2,		mm6
			punpckhbw		mm3,		mm6
// mm0, mm1 - unpacked pixels from floor engine
// mm2, mm3 - unpacked pixels from sphere engine
// mm4 - packed multipliers

			movq		mm6,		mm7
			pshufw		mm5,		mm4,		0
			psubw		mm6,		mm5
			pmulhuw		mm2,		mm5
			pmulhuw		mm0,		mm6
			pshufw		mm5,		mm4,		0x55
			movq		mm6,		mm7
			pmulhuw		mm3,		mm5
			paddusw		mm0,		[bla]
			psubw		mm6,		mm5
			pmulhuw		mm1,		mm6
			paddusw		mm3,		[bla]
			paddusw		mm0,		mm2
			paddusw		mm1,		mm3
			packuswb		mm0,		mm7
			packuswb		mm1,		mm7
			movd		[esi],		mm0
			movd		[esi + 4],		mm1
			add			esi,	8
			add			edi,	8
			add			eax,	4
			dec			ecx
			jnz			pixloop
			emms
		}
}
#endif

#ifdef shadows_related
/***********************************************************************
	shadows_merge_mmx2: MMX2 version of the shadow merging routine

	multiplies every pixel of dst by the inverted low byte of the
	src "shadow-buffer" array.

	the formula is:
	for (i=0;i<xr*yr;i++)
		floorbuffer[i] = multiplycolor(floorbuffer[i], 255 * (255 - (255&sbuffer[i])));

	note: count must be divisable by 4!
 ***********************************************************************/

void shadows_merge_mmx2(Uint32 *dst, Uint16 *src, int count)
{
	unsigned short bandmask[4] = {255, 255, 255, 255};
	count /= 4;
	__asm {


		mov		ecx,		count
		mov		edi,		[dst]
		mov		esi,		[src]

		movq		mm6,		[bandmask]
		pxor		mm7,		mm7

	ALIGN	16
	s_merge_loop:

		movq		mm5,		mm6
		pshufw		mm4,		mm6,		0xe4 // essentialy, movq %%mm6, %%mm4

		movq		mm0,		[edi]
		movq		mm2,		[edi + 8]
		pand		mm5,		[esi]

		pshufw		mm1,		mm0,		0xee
		pshufw		mm3,		mm2,		0xee

		psubw		mm4,		mm5

		punpcklbw		mm0,		mm7
		punpcklbw		mm1,		mm7

		pshufw		mm5,		mm4,		0x00

		punpcklbw		mm2,		mm7
		punpcklbw		mm3,		mm7

		pmullw		mm0,		mm5
		pshufw		mm5,		mm4,		0x55

		pmullw		mm1,		mm5

		pshufw		mm5,		mm4,		0xaa
		pshufw		mm4,		mm4,		0xff
		pmullw		mm2,		mm5
		pmullw		mm3,		mm4

		psrlw		mm0,		8
		psrlw		mm1,		8
		psrlw		mm2,		8
		psrlw		mm3,		8

		packuswb		mm0,		mm1
		packuswb		mm2,		mm3

		movq		[edi],		mm0
		movq		[edi + 8],		mm2

		add		esi,		8
		add		edi,		16

		dec		ecx
		jnz		s_merge_loop

		emms


	
	}
}
/*
	Perform triangle shadow and sphere shadow buffers merge
	The `dst` buffer is in 16bit format with lower 7 bits
	designating the amount of shadow.
	The `src' buffer is in 8bit format with lower 7 bits
	designating the amount of shadow.
*/
void triangle_sp_merge(Uint16 *dst, Uint8 *src, int count)
{
	for (int i = 0; i < count; i++) {
		dst[i] |= src[i];
	}
}

void fast_line_fill(Uint16 *p, int size, Uint16 fill)
{
	SSE_ALIGN(Uint16 bigfill[4]) = {fill, fill, fill, fill};
	int k = size / 4;
	/*
	__asm {
	movq		mm7,		[bigfill]
	
	}
	while (k--) {
		__asm {

					movq		mm0,		[p]
					por		mm0,		mm7
					movq		[p],		mm0
			
		}
		p += 4;
	}
	__asm {
		emms
	}*/
	__asm {
		movq	mm7,	[bigfill]
		mov		eax,	p
		mov		ecx,	k

		ALIGN	16
ploop:
		test	ecx,	ecx
		jz	ende
		dec	ecx
		movq	mm0,	[eax]
		por		mm0,	mm7
		movq	[eax],	mm0

		add	eax,	8
		jmp	ploop
ende:
		emms
	}
	size %= 4;
	p += k*4;
	for (int i = 0; i < size; i++)
		p[i] |= fill;
}

static void fast_reblend_mmx(int x1, int y1, int x2, int y2, Uint16 *sbuff, int xr, Uint16 sintensity, int cpui, int cpuc)
{
	sintensity = 255 - sintensity;
	SSE_ALIGN(Uint16 x_mor[4]) = { 0xff, 0xff, 0xff, 0xff} ;
	SSE_ALIGN(Uint16 x_add[4]) = { sintensity, sintensity, sintensity, sintensity };
	x2 = ((x2/4)+1)*4;
	x1 &= ~3;
	int xsize = (x2 - x1)/4;
	int ysize = y2 - y1 + 1;
	if (xsize <= 0 || ysize <= 0) return;
	Uint16 *p = &sbuff[y1 * xr + x1];
	xr *= 2;
	
	cpui = ((y1 % cpuc) + cpuc - cpui) % cpuc + 1;
	//
	__asm {
// 0 - p, 1 - ysize, 2 - xsize, 3 - xr
				push	esi
				push	edi
				mov		esi,		p
				mov		edx,		ysize
				mov		ecx,		cpui
				movq		mm7,		[x_mor]
				movq		mm6,		[x_add]
			
			ALIGN	16
			yyloop:
				mov		edi,		esi
				add		esi,		xr
			
				dec		ecx
				jnz		skip_row
				mov		ecx,		cpuc
				mov		eax,		xsize
			
			ALIGN	16
			xxloop:
				movq		mm0,		[edi]
				movq		mm1,		mm0
				pand		mm0,		mm7
				psrlw		mm1,		8
				paddusb		mm0,		mm1
				paddusb		mm0,		mm6
				psubusb		mm0,		mm6
				movq		[edi],		mm0
			
				add		edi,		8
				dec		eax
				jnz		xxloop
			
			ALIGN	16
			skip_row:
				dec		edx
				jnz		yyloop
			
				emms
				pop		edi
				pop		esi
	
	}
}
#endif
#ifdef rgb2yuv_asm

// ConvertRGB2YUV_ASM - convers [count] pixels from [*src] to [*dest]
// transformation is FROM RGB colorspace TO YUY2 packed format
// -- Assembly version of ConvertRGB2YUV_X86. Uses i386 intructions only. No precision loss also.
// count must be even
void ConvertRGB2YUV_X86_ASM(Uint32 *dest, Uint32 *src, size_t count)
{int s[9] = {IM11, IM12, IM13, IM21, IM22, IM23, IM31, IM32, IM33};
 count /= 2;
 	__asm {

		mov		esi,		[src]
		mov		edi,		[dest]

ALIGN	16
rgbasmloop:
	//  Y0 ::
		mov		ecx,		[esi]
		movzx		eax,		cl
		imul		[s + 8]
		xchg		ebx,		eax
		movzx		eax,		ch
		ror		ecx,		16
		imul		[s + 4]
		add		ebx,		eax
		movzx		eax,		cl
		ror		ecx,		16
		add		ebx,		0x100000
		imul		[s]
		add		eax,		ebx
		shr		eax,		16
		and		eax,		0xff
		mov		[edi],		eax
	//  U0 ::
		movzx		eax,		cl
		imul		[s + 32]
		xchg		ebx,		eax
		movzx		eax,		ch
		ror		ecx,		16
		imul		[s + 28]
		add		ebx,		eax
		movzx		eax,		cl
		ror		ecx,		16
		add		ebx,		0x800000
		imul		[s + 24]
		add		eax,		ebx
		shr		eax,		8
		and		eax,		0xff00
		or		[edi],		eax
	//  V0 ::
		movzx		eax,		cl
		imul		[s + 20]
		xchg		ebx,		eax
		movzx		eax,		ch
		ror		ecx,		16
		imul		[s + 16]
		add		ebx,		eax
		movzx		eax,		cl
		ror		ecx,		16
		add		ebx,		0x800000
		imul		[s + 12]
		add		eax,		ebx
		shl		eax,		8
		and		eax,		0xff000000
		or		[edi],		eax
	//  Y1 ::
		mov		ecx,		[esi + 4]
		movzx		eax,		cl
		imul		[s + 8]
		xchg		ebx,		eax
		movzx		eax,		ch
		ror		ecx,		16
		imul		[s + 4]
		add		ebx,		eax
		movzx		eax,		cl
		ror		ecx,		16
		add		ebx,		0x100000
		imul		[s]
		add		eax,		ebx
		and		eax,		0xff0000
		or		[edi],		eax

		add		edi,	4
		add		esi,	8
		dec		count
		jnz		rgbasmloop
 	}
}

// ConvertRGB2YUV_X86_FPU - convers [count] pixels from [*src] to [*dest] using floating point math
// transformation is FROM RGB colorspace TO YUY2 packed format
// count must be even.
void ConvertRGB2YUV_X86_FPU(Uint32 *dest, Uint32 *src, size_t count)
{
	SSE_ALIGN(float M[9]) = {M13, M33, M23, M11, M31, M21, M12, M32, M22};
	SSE_ALIGN(int   a[2]) = {16, 128};
	Uint16 cwsave, mycw;

	count /= 2;
	// lower FPU precision to single:
	__asm {
	fstcw		cwsave
	}
 // get conrol word
	mycw = cwsave & (~0x0300); // set single precision
	__asm {
	fldcw		mycw
	}
 // put control word
		__asm {
			mov		ecx,		count
			mov		edi,		[dest]
			mov		esi,		[src]
ALIGN 16
rgbfpuloop:
//get B0, R0, G0:
			mov		eax,		[esi]
			and		dword ptr [esi],		0xff00ff
			mov		edx,		0x00ff00
			fild		word	ptr [esi]
			and		edx,		eax
			shr		dword ptr [esi],		16
			shr		edx,		8
			fild		word	ptr	[esi]
			mov		[esi],		edx
			fild		word	ptr	[esi]
// st(0) = g0; st(1) = r0; st(2) = b0

// results will be: st -> V0, st(1) -> U0, st(2) -> Y0

// B0:
			fld		st(2)
			fmul		dword	ptr	[M]
			fld		st(3)
			fmul		dword	ptr	[M + 4]
			fld		st(4)
			ffree		st(5)
			fmul		dword	ptr	[M + 8]

// R0:
			fld		st(4)
			fmul		dword	ptr	[M + 12]
			fld		st(5)
			fmul		dword	ptr	[M + 16]
			fld		st(6)
			fmul		dword	ptr	[M + 20]
			ffree		st(7)

			faddp		st(3),		st
			faddp		st(3),		st
			faddp		st(3),		st
// G0:
			fld		st(3)
			fmul		dword	ptr	[M + 24]
			fld		st(4)
			fmul		dword	ptr	[M + 28]
			fld		st(5)
			fmul		dword	ptr	[M + 32]
			ffree		st(6)

			faddp		st(3),		st
			faddp		st(3),		st
			faddp		st(3),		st

			fiadd		dword	ptr	[a + 4]

// ST(0) = V0; ST(1) = U0; ST(2) = Y0
			fistp		word	ptr [edi]
			fiadd		dword	ptr	[a + 4]
			fistp		word	ptr	[esi]
			fiadd		dword	ptr	[a]
			shl		dword ptr [edi],		24

			fistp		word	ptr	[edi]
			movzx		edx,	byte ptr	[esi]
			shl		edx,		8
			or		[edi],		edx

// %0 = V0 00 U0 Y0
			mov		[esi],		eax

			fild	dword	ptr	[a]
			fld		dword	ptr	[M + 24]
			fld		dword	ptr	[M + 12]
			fld		dword	ptr	[M]

			mov		eax,		[esi + 4]
			mov		edx,		0x00ff00
			and		dword ptr [esi + 4],		0xff00ff
			and		edx,		eax
			fimul	word	ptr		[esi + 4]
			shr		dword ptr [esi + 4],		16
			faddp		st(3),		st
			shr		edx,		8
			fimul	word	ptr		[esi + 4]
			faddp		st(2),		st
			mov		[esi + 4],		edx
			fimul	word	ptr		[esi + 4]

			fadd

			fistp	dword	ptr		[esi + 4]
			mov		edx,		[esi + 4]
			shl		edx,		16
			or		dword ptr [edi],		edx
			mov		[esi + 4],		eax

			add		edi,		4
			add		esi,		8
			dec		ecx
			jnz		rgbfpuloop
		}
//		dest++; src+=2;
	__asm {
	fldcw		cwsave
	}
}

// ConvertRGB2YUV_MMX - convers [count] pixels from [*src] to [*dest] using MMX
// transformation is FROM RGB colorspace TO YUY2 packed format
// uses tightly optimized (pairable) MMX code
// should work significantly faster on PIII & P4. (And of course AthlonXP). It's not such a big deal on a PI and PII
// there is a small precision cost, which should be unnoticable
// count must be even.

void ConvertRGB2YUV_MMX(Uint32 *dest, Uint32 *src, size_t count)
{Sint16 pmm3[4]={SIM13, SIM33, SIM13, SIM23};
 Sint16 pmm4[4]={SIM12, SIM32, SIM12, SIM22};
 Sint16 pmm5[4]={SIM11, SIM31, SIM11, SIM21};
 Uint16 pmm6[4]={4096, 32768, 4096, 32768};
 Sint16 pmm7[4]={0, 0, 0, 0};


 count /= 2;
 // load some useful values
 __asm {

 	movq		mm3,		[pmm3]
 	movq		mm4,		[pmm4]
 	movq		mm5,		[pmm5]
 	movq		mm7,		[pmm7]
 	movq		mm6,		[pmm6]
 
 }
 //while (count--) {
	__asm {
		push		ebx
		mov		ecx,	count
		mov		esi,	[src]
		mov		edi,	[dest]
ALIGN 16
rgbmmx1loop:
	// get the current pixel in ebx, the next in edx
		mov		ebx,		[esi]
		mov		edx,		[esi + 4]
	// prepare mm0 - fill with (b0, b0, b1, b0)
		mov		ah,		bl
		mov		al,		dl
		shl		eax,		16
		mov		al,		bl
		mov		ah,		bl
		movd		mm0,		eax

		shr		ebx,		8
		shr		edx,		8

		punpcklbw		mm0,		mm7
	// prepare mm1 - fill with (g0, g0, g1, g0)
		mov		ah,		bl
		mov		al,		dl
		shl		eax,		16
		mov		al,		bl
		mov		ah,		bl
		movd		mm1,		eax

		shr		ebx,		8
		shr		edx,		8

		punpcklbw		mm1,		mm7
	// prepare mm2 - fill with (r0, r0, r1, r0)
		mov		ah,		bl
		mov		al,		dl
		shl		eax,		16
		mov		al,		bl
		mov		ah,		bl
		movd		mm2,		eax

	// do the actual math:

		punpcklbw		mm2,		mm7

		pmullw		mm0,		mm3
		pmullw		mm1,		mm4
		pmullw		mm2,		mm5

		paddw		mm0,		mm1
		paddw		mm2,		mm6
		paddw		mm0,		mm2
		psrlw		mm0,		8

		packuswb		mm0,		mm7

		movd		[edi],		mm0

		add			edi,	4
		add			esi,	8
		dec			ecx
		jnz			rgbmmx1loop
		pop			ebx
	//}
 	//dest++;
	//src+=2;
 	}
 __asm {
 	emms
 }
}

// ConvertRGB2YUV_MMX2 - convers [count] pixels from [*src] to [*dest] using MMX2
// transformation is FROM RGB colorspace TO YUY2 packed format
// uses tightly optimized (pairable) MMX code
// count must be even.

void ConvertRGB2YUV_MMX2(Uint32 *dest, Uint32 *src, size_t count)
{SSE_ALIGN(Sint16 pmm3[4])={SIM13, SIM33, SIM13, SIM23};
 SSE_ALIGN(Sint16 pmm4[4])={SIM12, SIM32, SIM12, SIM22};
 SSE_ALIGN(Sint16 pmm5[4])={SIM11, SIM31, SIM11, SIM21};
 SSE_ALIGN(Uint16 pmm6[4])={4096, 32768, 4096, 32768};
 SSE_ALIGN(Uint16 pmm7[4])={0xffff, 0x00ff, 0xffff, 0x00ff};
 //Sint16 pmm7[4]={0, 0, 0, 0};

 count /= 2;

 // load some constants:
 __asm {

 	pxor		mm7,		mm7
 	movq		mm6,		[pmm7]
 
 }

 //while (count--) {
 	__asm {

		mov			ecx,		count
		mov			edi,		[dest]
		mov			esi,		[src]

ALIGN 16
rgbmmx2loop:
		movq		mm0,		[esi]
		pand		mm0,		mm6
		pshufw		mm1,		mm0,		0x0e
// mm0 contains pixel 1 and pixel 2
// low dword of mm1 contains pixel 2

		punpcklbw		mm0,		mm7
		punpcklbw		mm1,		mm7

// mm0 = (b0 g0 r0 00) (words)
// mm1 = (b1 g1 r1 00) (words)

// SOME REAALY WEIRD SHUFFLES::: :)
		pshufw		mm2,		mm0,		0x30 // 00110000
		pshufw		mm3,		mm1,		0xcf // 11001111
		pshufw		mm4,		mm0,		0x75 // 01110101
		pshufw		mm5,		mm1,		0xdf // 11011111
		pshufw		mm0,		mm0,		0xba // 10111010
		pshufw		mm1,		mm1,		0xef // 11101111
// ends up like this:
		por		mm2,		mm3 // mm2 - b0 b0 b1 b0
		por		mm4,		mm5 // mm4 - g0 g0 g1 g0
		por		mm0,		mm1 // mm0 - r0 r0 r1 r0

		pmullw		mm2,		[pmm3] // mm2 *= M13 M33 M13 M23
		pmullw		mm4,		[pmm4] // mm4 *= M12 M32 M12 M22
		pmullw		mm0,		[pmm5] // mm0 *= M11 M31 M11 M21

		paddw		mm2,		[pmm6] // mm2 += 16  128  16 128
		paddw		mm0,		mm4
		paddw		mm0,		mm2
		psrlw		mm0,		8 // mm0 = Y0 U0 Y1 V0

		packuswb		mm0,		mm7
		movd		[edi],		mm0

		add			esi,	8
		add			edi,	4
		dec			ecx
		jnz			rgbmmx2loop
	
// 	}
 //	src+=2;
//	dest++;
 }

 __asm {
 	emms
 }
}


// ConvertRGB2YUV_SSE - convers [count] pixels from [*src] to [*dest] using MMX2 and SSE
// transformation is FROM RGB colorspace TO YUY2 packed format
// count must be divisable by four! (twice unrolled loop)

void ConvertRGB2YUV_SSE(Uint32 *dest, Uint32 *src, size_t count)
{
	SSE_ALIGN(float cxmm4[4])= { M13, M33, M13, M23 };
	SSE_ALIGN(float cxmm5[4])= { M12, M32, M12, M22 };
	SSE_ALIGN(float cxmm6[4])= { M11, M31, M11, M21 };
	SSE_ALIGN(float cxmm7[4])= {  16, 128,  16, 128 };
	Uint16 cmm6[4] = {0xffff, 0x00ff, 0xffff, 0x00ff};
	count /= 4;
	__asm {
	pxor		mm7,		mm7
	}
//	while (count -- ) {

/*
	Assembly Implementation:
	get 16 bytes per cycle (4 points) to produce 8 bytes of YUV results
	i.e. the conversion loop is unrolled once. Proved to be 15% faster than
	the non-unrolled variant on an Athlon XP
*/
		__asm {

			mov			ecx,		count
			mov			esi,		[src]
			mov			edi,		[dest]

ALIGN	16
rgbsseloop:
			movq		mm0,		[esi + 8]
			movq		mm4,		[esi]
			pand		mm0,		[cmm6]
			pand		mm4,		[cmm6]
			pshufw		mm1,		mm0,		0x0e
			pshufw		mm5,		mm4,		0x0e
// hint hardware to prefetch next 4 points:
			prefetchnta		[esi + 16]

			punpcklbw		mm0,		mm7
			punpcklbw		mm4,		mm7
			punpcklbw		mm1,		mm7
			punpcklbw		mm5,		mm7

// mm0 = (b0, g0, r0, 00) (words)
// mm1 = (b1, g1, r1, 00) (words)
///---------------------------------------
// mm4 = (b2, g2, r2, 00) (words)
// mm5 = (b3, g3, r3, 00) (words)

			pshufw		mm2,		mm0,		0xcc
			pshufw		mm6,		mm4,		0xcc
			pshufw		mm3,		mm1,		0xcc
			pshufw		mm7,		mm5,		0xcc

			cvtpi2ps		xmm1,		mm2
			cvtpi2ps		xmm5,		mm6
			cvtpi2ps		xmm0,		mm3
			cvtpi2ps		xmm4,		mm7

			pshufw		mm2,		mm0,		0xdd
			pshufw		mm3,		mm1,		0xdd
			pshufw		mm6,		mm4,		0xdd
			pshufw		mm7,		mm5,		0xdd

			movlhps		xmm1,		xmm0
			movlhps		xmm5,		xmm4

			pshufw		mm0,		mm0,		0xee
			pshufw		mm1,		mm1,		0xee
			pshufw		mm4,		mm4,		0xee
			pshufw		mm5,		mm5,		0xee

			mulps		xmm1,		[cxmm4]
			mulps		xmm5,		[cxmm4]

			cvtpi2ps		xmm2,		mm2
			cvtpi2ps		xmm3,		mm3
			cvtpi2ps		xmm6,		mm6
			cvtpi2ps		xmm7,		mm7

			movlhps		xmm2,		xmm3
			movlhps		xmm6,		xmm7

			cvtpi2ps		xmm3,		mm0
			cvtpi2ps		xmm0,		mm1
			cvtpi2ps		xmm7,		mm4
			cvtpi2ps		xmm4,		mm5

			mulps		xmm2,		[cxmm5]
			mulps		xmm6,		[cxmm5]

			movlhps		xmm3,		xmm0
			movlhps		xmm7,		xmm4

			movaps		xmm0,		[cxmm7]
			movaps		xmm4,		xmm0

			mulps		xmm3,		[cxmm6]
			mulps		xmm7,		[cxmm6]

			pxor		mm7,		mm7

			addps		xmm0,		xmm1
			addps		xmm4,		xmm5
			addps		xmm2,		xmm3
			addps		xmm6,		xmm7
			addps		xmm0,		xmm2
			addps		xmm4,		xmm6

			movhlps		xmm1,		xmm0
			movhlps		xmm5,		xmm4

			cvtps2pi		mm0,		xmm0
			cvtps2pi		mm4,		xmm4
			cvtps2pi		mm1,		xmm1
			cvtps2pi		mm5,		xmm5

			packssdw		mm0,		mm1
			packssdw		mm4,		mm5
			packuswb		mm0,		mm7
			packuswb		mm4,		mm7

		//	movd		[dest],		mm0
			psllq		mm0,		32
			por		mm4,		mm0
		//	movd		[dest + 4],		mm4
// we prefer movntq while it minimizes cache pollution:
			movntq		[edi],		mm4


			add			edi,	8
			add			esi,	16
			dec			ecx
			jnz			rgbsseloop
//		}
//		dest+=2;
//		src+=4;
	}
	__asm {
		emms
	}
}
#endif
#ifdef rgbhacks1
/***************************************************************
 * gets some flags about the processor's capabilities          *
 *-------------------------------------------------------------*
 * This is the Windows version, requires CPUID. Should work on *
 * both AMD and Intel CPUs. Code taken from the internet:      *
 *  (looks very ugly :)                                        *
 * http://www.tommesani.com/DetectingMMXSSE.html               *
 ***************************************************************/
int CPUZ=0, CPUEXT=0, ECS=0;

 __asm {

 	mov		eax,		1
 	cpuid
 	mov		CPUZ,		edx
 	mov		eax,		0x80000000
 	cpuid
 	cmp		eax,		0x80000001
 	jb		not_supported

 	mov		eax,		0x80000001
 	cpuid
 	mov		CPUEXT,		edx
 	mov		ECS,		1
 	jmp		ende

 not_supported:
 	mov		ECS,		0
 ende:

 }

 hasmmx = (CPUZ&MMX_MASK?1:0);
 hassse = (CPUZ&SSE_MASK?1:0);
 hassse2= (CPUZ&SSE2_MASK?1:0);
 if (hassse) hasmmx2 = 1;
     else {
     if (ECS && (CPUEXT&MMX2_MASK)) hasmmx2=1;
                               else hasmmx2=0;
     }

#endif
#ifdef rgbhacks2
static void get_vendor_string(char *name) // for benchmarking only:
{
	__asm {
		push	esi
		push	ebx
		
		mov	esi,	name
		xor	eax,	eax
		cpuid
		
		mov	[esi],		ebx
		mov	[esi+4],	edx
		mov	[esi+8],	ecx
		
		pop	ebx
		pop	esi
	}
}
#endif // rgbhacks2

#ifdef threads1 
int atomic_add(volatile int addr[], int val) 
{
	__asm {
		mov		edx,	addr
		mov		eax,	val
		lock xadd	[edx],	eax
		mov		val,		eax
	}
	return val;
}
#endif

#ifdef VECTOR3_ASM
static inline float sse_rcp(float x)  
{
	register float _rezultf;
	__asm {
		rcpss	xmm0,	x
		movss	xmm2,	x
		movss	xmm1,	xmm0
		addss	xmm0,	xmm0
		mulss	xmm1,	xmm1
		mulss	xmm1,	xmm2
		subss	xmm0,	xmm1
		movss	_rezultf,	xmm0
	}
	return _rezultf;
}
#endif //VECTOR3_ASM

#else // _MSC_VER
#error unsupported compiler detected
#endif //_MSC_VER
#endif // __GNUC__

#else  // USE_ASSEMBLY
// define dummies for the missing functions:
#ifdef antialias_asm
static void antialias_4x_mmx2_lo_fi(Uint32 *fb) {}
static void antialias_4x_mmx_hi_fi(Uint32 *fb) {}
#endif
#ifdef gfx_asm
Uint32 blend_sse(Uint32 f, Uint32 b, float ff) {return 0;}
#endif
#ifdef profile_asm
long long fake_tsc = 0;
long long prof_rdtsc(void) {return ++fake_tsc; }
#endif
#ifdef shaders_asm
void convolve_mmx_w_shifts_generic(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift) {}
void convolve_mmx_w_shifts_3(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift) {}
void convolve_mmx_w_shifts_5(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift) {}
static inline void convolve_mmx_w_shifts(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M, int shift) {}
void FFT_1D_complex_sse(int dir, int m, float *x, float *y) {}
void shader_gamma_shl(Uint32 *src, Uint32 *dest, int resx, int resy, int shift) {}
void shader_gamma_shr(Uint32 *src, Uint32 *dest, int resx, int resy, int shift) {}
void float_copy_ij_i(float *a, float *b, complex c[], int fft_size) {}
void float_copy_i_ij(float *a, float *b, complex c[], int fft_size) {}
void float_copy_ij_j(float *a, float *b, complex c[], int fft_size) {}
void float_copy_j_ij(float *a, float *b, complex c[], int fft_size) {}
void shader_spill_mmx(Uint8*, Uint8*, int, int, float) {}
void shader_fbmerge_mmx2(Uint32 *dest, Uint8 * src, int resx, int resy, float intensity, Uint32 glow_color) {}

#endif // shaders_asm
#ifdef apply_fft_filter_asm
void apply_fft_filter_sse(complex *dest, float *filter, int fft_size) {}
#endif
#ifdef blur_asm
void blur_forward_mmx(Uint32 *dest, Uint32 *src, int count){}
void blur_backward_mmx(Uint32 *dest, Uint32 *src, int count){}
void buffer_minus_mmx(Uint32 *dest, Uint32 *src, int count){}
void buffer_plus_mmx(Uint32 *dest, Uint32 *src, int count) {}
#endif
#ifdef fract_asm
//static inline Uint32 bilinea_p5(Uint32 x0y0, Uint32 x1y0, Uint32 x0y1, Uint32 x1y1, unsigned Fx, unsigned Fy) {return 0;}
static inline Uint32	igrated(Uint32 color, int x, int y, int lx, int ly, int ysq) {return 0;}
Uint32 bilinea4(Uint32 x0y0, Uint32 x1y0, Uint32 x0y1, Uint32 x1y1, int x, int y) {return 0;}
void blur_do_MMX(Uint32 *bb, Uint32 *src, Uint32 *dst, int count, int kopy) {}
#endif
#ifdef genrender_asm
void merge_rows(Uint32 *flr, Uint32 *sph, unsigned short *multi, int count) {}
void shadows_merge_mmx2(Uint32 *dst, Uint16 *src, int count) {}
#endif
#ifdef rgb2yuv_asm
void ConvertRGB2YUV_X86_ASM(Uint32 *dest, Uint32 *src, size_t count) {}
void ConvertRGB2YUV_X86_FPU(Uint32 *dest, Uint32 *src, size_t count) {}
void ConvertRGB2YUV_MMX(Uint32 *dest, Uint32 *src, size_t count) {}
void ConvertRGB2YUV_MMX2(Uint32 *dest, Uint32 *src, size_t count) {}
void ConvertRGB2YUV_SSE(Uint32 *dest, Uint32 *src, size_t count) {}
#endif
#ifdef rgbhacks1
//void GetCPUCaps(CPUCaps *a) {a->hasmmx=a->hasmmx2=a->hassse=a->largemem=0;}
#endif
#ifdef rgbhacks2
static void get_vendor_string(char *name);
#endif
#ifdef threads1 
int atomic_add(volatile int *addr, int val) 
{
	int x = *addr;
	addr[0]+=val;
	return x;
}
#endif

#endif // USE_ASSEMBLY

//#endif //__X86_ASM_H__
