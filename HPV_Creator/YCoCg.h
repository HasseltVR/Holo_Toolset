/*
RGB_ to CoCg_Y conversion and back.
Copyright (C) 2007 Id Software, Inc.
Written by J.M.P. van Waveren

This code is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
*/

#ifndef YCoCg_h
#define YCoCg_h

#ifdef __cplusplus
extern "C" {
#endif
#ifndef byte
	typedef unsigned char byte;
#endif

/*
RGB <-> YCoCg

Y  = [ 1/4  1/2  1/4] [R]
Co = [ 1/2    0 -1/2] [G]
CG = [-1/4  1/2 -1/4] [B]

R  = [   1    1   -1] [Y]
G  = [   1    0    1] [Co]
B  = [   1   -1   -1] [Cg]

*/

#define RGB_TO_YCOCG_Y( r, g, b )   ( ( (    r +   (g<<1) +  b     ) + 2 ) >> 2 )
#define RGB_TO_YCOCG_CO( r, g, b )  ( ( (   (r<<1)        - (b<<1) ) + 2 ) >> 2 )
#define RGB_TO_YCOCG_CG( r, g, b )  ( ( ( -  r +   (g<<1) -  b     ) + 2 ) >> 2 )

#define COCG_TO_R( co, cg )         ( co - cg )
#define COCG_TO_G( co, cg )         ( cg )
#define COCG_TO_B( co, cg )         ( - co - cg )

byte CLAMP_BYTE(int x);
void ConvertRGBToCoCg_Y(byte *image, int width, int height);
void ConvertCoCg_YToRGB(byte *image, int width, int height);

#ifdef __cplusplus
}
#endif

#endif // YCoCg_H
