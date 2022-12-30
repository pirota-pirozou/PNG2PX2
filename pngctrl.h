//

#pragma once

#ifndef _PNGCTRL_H_
#define _PNGCTRL_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//////////////////////////////////////////////////////////////////////////////////////////

#ifndef PDIB
typedef LPBITMAPINFOHEADER		PDIB;
#endif

/*
//
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned long u_long;

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
*/

#define DibPtr(lpbi)            ((lpbi)->biCompression == BI_BITFIELDS \
                                   ? (LPVOID)(DibColors(lpbi) + 3) \
                                   : (LPVOID)(DibColors(lpbi) + (UINT)(lpbi)->biClrUsed))
#define DibColors(lpbi)         ((RGBQUAD FAR *)((LPBYTE)(lpbi) + (int)(lpbi)->biSize))


PDIB pngptr2dib(void *pptr);
PDIB PngOpenFile(LPSTR szFile);

//////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PNGCTRL_H_
