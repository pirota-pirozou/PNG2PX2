#pragma once

#ifndef __PNGCTRL_H__
#define __PNGCTRL_H__


#include	<windows.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	"png.h"
#include	"pngctrl.h"

#define PNG_BYTES_TO_CHECK 4

#ifdef __cplusplus
extern "C" {
#endif

	// png関連
	static unsigned char * pngstream_ptr = NULL;


	static int				png_trans_num;				// 透明色の数
	static png_color_16		png_trans_col;				// 透明色
	static png_colorp		palette;                    // パレット

	// png読み込みストリーム
	static void libReadStream(png_structp png_ptr, png_bytep data, png_size_t length)
	{
		memcpy(data, pngstream_ptr, length);

		pngstream_ptr += length;
	}

	// pngストリーム初期化
	static void libInitStream(void *pptr)
	{
		pngstream_ptr = (unsigned char *)pptr;
	}

	// ビットマップ2次元配列確保
	unsigned char ** alloc_map(IMAGEDATA* img)
	{
		int width = img->width;
		int height = img->height;

		img->map = malloc(sizeof(unsigned char*) * height);
		if (img->map == NULL)
		{
			return NULL;
		}

		for (int i = 0; i < height; i++)
		{
			img->map[i] = malloc(sizeof(unsigned char) * width);
			if (img->map[i] == NULL)
			{
				return NULL;
			}
		}

		return img->map;
	}

	// ビットマップ2次元配列解放
	void free_map(IMAGEDATA* img)
	{
		int height = img->height;
		if (img->map != NULL)
		{
			for (int i = 0; i < height; i++)
			{
				free(img->map[i]);
				img->map[i] = NULL;
			}
			free(img->map);
			img->map = NULL;
		}

	}

	// pngを書き込み（インデックスカラー専用）
	int writepng(const char *filename, IMAGEDATA *img)
	{
		int result = -1;
		if (img == NULL) {
			return result;
		}
		FILE* fp = fopen(filename, "wb");
		if (fp == NULL) {
			perror(filename);
			return result;
		}
		result = write_png_stream(fp, img);
		fclose(fp);

		return 0;
	}

	// ビットマップとパレットをpngとしてセーブ
	int write_png_stream(FILE* fp, IMAGEDATA *img)
	{
		int i, x, y;
		int result = -1;
		int row_size;
		int color_type;
		png_structp png = NULL;
		png_infop info = NULL;
		png_bytep row;
		png_bytepp rows = NULL;
		png_colorp palette = NULL;
		if (img == NULL) {
			return result;
		}
		switch (img->color_type) {
		case COLOR_TYPE_INDEX:  // インデックスカラー
			color_type = PNG_COLOR_TYPE_PALETTE;
			row_size = sizeof(png_byte) * img->width;
			break;
		default:
			return result;
		}
		png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (png == NULL) {
			goto error;
		}
		info = png_create_info_struct(png);
		if (info == NULL) {
			goto error;
		}
		if (setjmp(png_jmpbuf(png))) {
			goto error;
		}
		png_init_io(png, fp);
		png_set_IHDR(png, info, img->width, img->height, 8,
			color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);
		rows = png_malloc(png, sizeof(png_bytep) * img->height);
		if (rows == NULL) {
			goto error;
		}
		png_set_rows(png, info, rows);
		memset(rows, 0, sizeof(png_bytep) * img->height);
		for (y = 0; y < img->height; y++) {
			if ((rows[y] = png_malloc(png, row_size)) == NULL) {
				goto error;
			}
		}
		switch (img->color_type) {
		case COLOR_TYPE_INDEX:  // インデックスカラー
			palette = png_malloc(png, sizeof(png_color) * img->palette_num);
			for (i = 0; i < img->palette_num; i++) {
				palette[i].red = img->palette[i].r;
				palette[i].green = img->palette[i].g;
				palette[i].blue = img->palette[i].b;
			}
			png_set_PLTE(png, info, palette, img->palette_num);
			for (i = img->palette_num - 1; i >= 0 && img->palette[i].a != 0xFF; i--);
			if (i >= 0) {
				int num_trans = i + 1;
//				png_byte trans[255];
				png_byte trans[256];
				for (i = 0; i < num_trans; i++) {
					trans[i] = img->palette[i].a;
				}
				png_set_tRNS(png, info, trans, num_trans, NULL);
			}
			png_free(png, palette);
			for (y = 0; y < img->height; y++) {
				row = rows[y];
				for (x = 0; x < img->width; x++) {
					*row++ = img->map[y][x];
				}
			}
			break;

		default:;
		}
		png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
		result = 0;
	error:
		if (rows != NULL) {
			for (y = 0; y < img->height; y++) {
				png_free(png, rows[y]);
			}
			png_free(png, rows);
		}
		png_destroy_write_struct(&png, &info);
		return result;
	}

	// pngを開きDIBに変換
	PDIB pngptr2dib(void *pptr)
	{
		png_structp     png_ptr;
		png_infop       info_ptr;
		unsigned long   width, height;
		int             bit_depth, color_type, interlace_type;
		unsigned char   **image;
		unsigned int	i;
		LPBITMAPINFO	dib_ptr = NULL;
		png_uint_32		ulRowBytes;
		png_uint_32		ulUnitBytes;
		RGBQUAD         *pcolors;


		unsigned char *dibp;
		int alc_sz;
		unsigned int num_pal;

		png_bytep		pal_trns_p = NULL;
		int				num_trns;

		png_color_16p	trans_values = NULL;

		// ファイルはPNGか？
		if (png_sig_cmp((unsigned char *)pptr, (png_size_t)0, PNG_BYTES_TO_CHECK))
			return NULL;

		//
		png_ptr = png_create_read_struct(							// png_ptr構造体を確保・初期化
			PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (png_ptr == NULL)
		{
			return NULL;
		}

		info_ptr = png_create_info_struct(png_ptr);					// info_ptr構造体を確保・初期化
		if (info_ptr == NULL)
		{
			png_destroy_read_struct(&png_ptr, NULL, NULL);
			return NULL;
		}

		libInitStream(pptr);										// ストリームを初期化
		png_set_read_fn(png_ptr, NULL, libReadStream);				// libpngにストリーム処理を設定

		png_read_info(png_ptr, info_ptr);			               // PNGファイルのヘッダを読み込み
		png_get_IHDR(png_ptr, info_ptr, &width, &height,	       // IHDRチャンク情報を取得
			&bit_depth, &color_type, &interlace_type,
			NULL, NULL);

		if (color_type == PNG_COLOR_TYPE_PALETTE)
		{
			png_get_PLTE(png_ptr, info_ptr, &palette, &num_pal);            // パレット取得
		}
		else
		{
			num_pal = 0;
		}
		/*
			if (color_type == PNG_COLOR_TYPE_PALETTE)
			{
				png_set_palette_to_rgb(png_ptr);	// これでTRNSチャンクも展開され、1dot=RGBAに展開されてしまう
			}
		//    png_set_strip_alpha(png_ptr);			// これでALPHAを展開しなくて済む

			// tell libpng to strip 16 bit/color files down to 8 bits/color
			if (bit_depth == 16)
			   png_set_strip_16(png_ptr);

			if (color_type == PNG_COLOR_TYPE_PALETTE || bit_depth < 8)
				png_set_expand(png_ptr);

			// after the transformations have been registered update info_ptr data
			png_read_update_info(png_ptr, info_ptr);

			// get again width, height and the new bit-depth and color-type
			png_get_IHDR(png_ptr, info_ptr, &width, &height,
							&bit_depth, &color_type, &interlace_type,
							NULL, NULL);

			png_set_bgr(png_ptr);
		  */


		  // 描画先の初期化
		  //   画像サイズ = width, height
		  //   色数 = bit_depth
		  //   ほか

		  // 横１ラインのバイト数
		ulRowBytes = png_get_rowbytes(png_ptr, info_ptr);
		// １ドットのバイト数
		ulUnitBytes = ulRowBytes / width;

		// 横は4dotの倍数になるように
		if (width & 3)
		{
			int adj = (4 - (width & 3));
			width += adj;
			ulRowBytes += adj * ulUnitBytes;
		}

		// 16色(Packed)の場合にパッチする
		if (ulUnitBytes == 0)
		{
			ulRowBytes *= 2;
		}

		// 確保サイズ
		alc_sz = sizeof(png_byte)* ulRowBytes * height + sizeof(BITMAPINFOHEADER) +
			sizeof(RGBQUAD)* num_pal;

		// ビットマップを確保
		dib_ptr = (LPBITMAPINFO)malloc(alc_sz);

		// BITMAPINFOHEADER をクリア
		memset(dib_ptr, 0, sizeof(BITMAPINFOHEADER));

		// 透過色チェック
		png_trans_num = 0;
		if (png_get_tRNS(png_ptr, info_ptr, NULL, &num_trns, &trans_values))
		{
			png_trans_num = num_trns;					// 透明色の数
			png_trans_col = *trans_values;				// 透明色
		}

		// ヘッダに書き込み
		dib_ptr->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		dib_ptr->bmiHeader.biBitCount = (color_type == PNG_COLOR_TYPE_PALETTE) ? 8 : 24;
		dib_ptr->bmiHeader.biWidth = width;
		dib_ptr->bmiHeader.biHeight = height;
		dib_ptr->bmiHeader.biPlanes = 1;
		dib_ptr->bmiHeader.biCompression = BI_RGB;
		dib_ptr->bmiHeader.biSizeImage = (ulRowBytes * height);
		dib_ptr->bmiHeader.biClrUsed = num_pal;

		//
		dibp = (unsigned char *)dib_ptr + sizeof(BITMAPINFOHEADER);

		if (dib_ptr->bmiHeader.biCompression != BI_BITFIELDS)
		{
			pcolors = (RGBQUAD *)dibp;                                     /* RGBQUADへのポインタ */
			(unsigned char *)dibp += sizeof(RGBQUAD) * dib_ptr->bmiHeader.biClrUsed;
		}
		else
		{
			pcolors = NULL;
			(unsigned char*)dibp += 3;
		}

		// パレットコピー
		if (color_type == PNG_COLOR_TYPE_PALETTE)
		{
			for (i = 0; i < num_pal; i++)
			{
				pcolors->rgbRed = palette->red;
				pcolors->rgbGreen = palette->green;
				pcolors->rgbBlue = palette->blue;
				pcolors->rgbReserved = 0;
				palette++;
				pcolors++;
			}
		}

		// 展開ポインタ配列を確保
		image = (png_bytepp)malloc(height * sizeof(png_bytep));

		// 16色(Packed)以外の場合（通常）
		if (ulUnitBytes != 0)
		{
			for (i = 0; i < height; i++) {
				image[i] = (png_bytep)dibp + (i * ulRowBytes);
			}
			png_read_image(png_ptr, image);							// 画像データの展開
		}
		else
		{
			// 16色(Packed)の場合
			png_bytep* imagetmp = (png_bytepp)malloc(height * width / 2);

			for (i = 0; i < height; i++) {
				image[i] = (png_bytep)imagetmp + (i * (width / 2));
			}
			png_read_image(png_ptr, image);							// 画像データの展開

			// 4bit → 8bit展開
			unsigned char* dest;
			for (int y = 0; y < height; y++)
			{
				png_bytep ptr = image[y];
				dest = (png_bytep)dibp + (y * ulRowBytes);
				for (int x = 0; x < width; x += 2)
				{
					png_byte c = *ptr;
					*dest = (c >> 4);
					*(dest+1) = (c & 0x0F);
					ptr++;
					dest += 2;
				}
			}
			free(imagetmp);
		}
		free(image);

		png_destroy_read_struct(								// libpng構造体のメモリ解放
			&png_ptr, &info_ptr, (png_infopp)NULL);

		return (PDIB)dib_ptr;
	}

#define PNG_FILE_MAPPING 0                                              // FileMapping機能を使う

	// PNGOpenFile
	PDIB PngOpenFile(LPSTR szFile)
	{
		PDIB pdib = NULL;
		PDIB png = NULL;
#if !PNG_FILE_MAPPING
		DWORD fsize = 0;
		DWORD rbytes;
#endif
		HANDLE fh; 	// File Access

		// ファイル名はPNGデータを指すポインタか？
		if (!png_sig_cmp((unsigned char *)szFile, (png_size_t)0, PNG_BYTES_TO_CHECK))
		{
			// もしそうだったら直接展開
			pdib = pngptr2dib((void *)szFile);

			return pdib;
		}

#if PNG_FILE_MAPPING
		//--------------------------------------------
		// ファイルマッピングオブジェクトを使ってみる
		//--------------------------------------------
		fh = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fh == INVALID_HANDLE_VALUE)
		{
			// 開けない
			CloseHandle(fh);
			return NULL;
		}

		HANDLE mh = CreateFileMapping(fh, NULL, PAGE_READONLY, 0, 0, NULL);
		if (mh == NULL)
		{
			// マッピングできない
			CloseHandle(fh);
			return NULL;
		}
		CloseHandle(fh);

		// マッピングする
		png = (PDIB)MapViewOfFile(mh, FILE_MAP_READ, 0, 0, 0);

#else
		// PNGファイルを丸ごと開いてメモリ確保
		fh = CreateFile(szFile, GENERIC_READ, 0, 0,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fh == INVALID_HANDLE_VALUE)
		{
			// 開けない
			CloseHandle(fh);
			return NULL;
		}
		fsize = SetFilePointer(fh, 0, NULL, FILE_END);		// ファイルサイズの取得
		SetFilePointer(fh, 0, NULL, FILE_BEGIN);			// ファイルポインタを先頭に

		// メモリ確保
		png = (PDIB)malloc(fsize);
		if (png == NULL)
		{
			CloseHandle(fh);
			return NULL;
		}

		// 読み込み
		ReadFile(fh, png, fsize, &rbytes, NULL);
		CloseHandle(fh);

		// 全部読めなかった？
		if (fsize != rbytes) return NULL;
#endif

		// PNGからDIBに展開
		pdib = pngptr2dib(png);

#if PNG_FILE_MAPPING
		// マッピング解除
		CloseHandle(mh);
		UnmapViewOfFile(png);
#else
		// PNGイメージ開放
		free(png);
#endif

		return pdib;
	}

#ifdef __cplusplus
}	// extern "C"
#endif



#endif // __PNGCTRL_H__
