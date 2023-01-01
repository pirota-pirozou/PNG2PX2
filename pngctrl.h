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


#define COLOR_TYPE_INDEX 0   // インデックスカラー

typedef struct {
    unsigned char r;        // Red
    unsigned char g;        // Green
    unsigned char b;        // Blue
    unsigned char a;        // Alpha
} color_t;

typedef struct
{
    int width;         // 幅
    int height;        // 高さ
    unsigned short color_type;  // 色表現の種別
    unsigned short palette_num; // カラーパレットの数
    color_t *palette;           // カラーパレットへのポインタ
    unsigned char ** map;       // 画像データ
} IMAGEDATA;

#define DibPtr(lpbi)            ((lpbi)->biCompression == BI_BITFIELDS \
                                   ? (LPVOID)(DibColors(lpbi) + 3) \
                                   : (LPVOID)(DibColors(lpbi) + (UINT)(lpbi)->biClrUsed))
#define DibColors(lpbi)         ((RGBQUAD FAR *)((LPBYTE)(lpbi) + (int)(lpbi)->biSize))

unsigned char** alloc_map(IMAGEDATA* img);
void free_map(IMAGEDATA* img);


int writepng(const char* filename, IMAGEDATA* img);
int write_png_stream(FILE* fp, IMAGEDATA* img);


PDIB pngptr2dib(void *pptr);
PDIB PngOpenFile(LPSTR szFile);

//////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PNGCTRL_H_
