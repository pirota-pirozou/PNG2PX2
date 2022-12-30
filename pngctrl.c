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

		// 確保サイズ
		alc_sz = sizeof(png_byte)* ulRowBytes * height + sizeof(BITMAPINFOHEADER) +
			sizeof(RGBQUAD)* num_pal;

		// ビットマップを確保
		dib_ptr = (LPBITMAPINFO)malloc(alc_sz);

		// BITMAPINFOHEADER をクリア
		ZeroMemory(dib_ptr, sizeof(BITMAPINFOHEADER));

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

		for (i = 0; i < height; i++) {
			//        image[height-i-1] = (png_bytep) dibp + (i * ulRowBytes);
			image[i] = (png_bytep)dibp + (i * ulRowBytes);
		}
		png_read_image(png_ptr, image);							// 画像データの展開

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
