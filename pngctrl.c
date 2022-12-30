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

	// png�֘A
	static unsigned char * pngstream_ptr = NULL;


	static int				png_trans_num;				// �����F�̐�
	static png_color_16		png_trans_col;				// �����F
	static png_colorp		palette;                    // �p���b�g

	// png�ǂݍ��݃X�g���[��
	static void libReadStream(png_structp png_ptr, png_bytep data, png_size_t length)
	{
		memcpy(data, pngstream_ptr, length);

		pngstream_ptr += length;
	}

	// png�X�g���[��������
	static void libInitStream(void *pptr)
	{
		pngstream_ptr = (unsigned char *)pptr;
	}

	// png���J��DIB�ɕϊ�
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

		// �t�@�C����PNG���H
		if (png_sig_cmp((unsigned char *)pptr, (png_size_t)0, PNG_BYTES_TO_CHECK))
			return NULL;

		//
		png_ptr = png_create_read_struct(							// png_ptr�\���̂��m�ہE������
			PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (png_ptr == NULL)
		{
			return NULL;
		}

		info_ptr = png_create_info_struct(png_ptr);					// info_ptr�\���̂��m�ہE������
		if (info_ptr == NULL)
		{
			png_destroy_read_struct(&png_ptr, NULL, NULL);
			return NULL;
		}

		libInitStream(pptr);										// �X�g���[����������
		png_set_read_fn(png_ptr, NULL, libReadStream);				// libpng�ɃX�g���[��������ݒ�

		png_read_info(png_ptr, info_ptr);			               // PNG�t�@�C���̃w�b�_��ǂݍ���
		png_get_IHDR(png_ptr, info_ptr, &width, &height,	       // IHDR�`�����N�����擾
			&bit_depth, &color_type, &interlace_type,
			NULL, NULL);

		if (color_type == PNG_COLOR_TYPE_PALETTE)
		{
			png_get_PLTE(png_ptr, info_ptr, &palette, &num_pal);            // �p���b�g�擾
		}
		else
		{
			num_pal = 0;
		}
		/*
			if (color_type == PNG_COLOR_TYPE_PALETTE)
			{
				png_set_palette_to_rgb(png_ptr);	// �����TRNS�`�����N���W�J����A1dot=RGBA�ɓW�J����Ă��܂�
			}
		//    png_set_strip_alpha(png_ptr);			// �����ALPHA��W�J���Ȃ��čς�

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


		  // �`���̏�����
		  //   �摜�T�C�Y = width, height
		  //   �F�� = bit_depth
		  //   �ق�

		  // ���P���C���̃o�C�g��
		ulRowBytes = png_get_rowbytes(png_ptr, info_ptr);
		// �P�h�b�g�̃o�C�g��
		ulUnitBytes = ulRowBytes / width;

		// ����4dot�̔{���ɂȂ�悤��
		if (width & 3)
		{
			int adj = (4 - (width & 3));
			width += adj;
			ulRowBytes += adj * ulUnitBytes;
		}

		// �m�ۃT�C�Y
		alc_sz = sizeof(png_byte)* ulRowBytes * height + sizeof(BITMAPINFOHEADER) +
			sizeof(RGBQUAD)* num_pal;

		// �r�b�g�}�b�v���m��
		dib_ptr = (LPBITMAPINFO)malloc(alc_sz);

		// BITMAPINFOHEADER ���N���A
		ZeroMemory(dib_ptr, sizeof(BITMAPINFOHEADER));

		// ���ߐF�`�F�b�N
		png_trans_num = 0;
		if (png_get_tRNS(png_ptr, info_ptr, NULL, &num_trns, &trans_values))
		{
			png_trans_num = num_trns;					// �����F�̐�
			png_trans_col = *trans_values;				// �����F
		}

		// �w�b�_�ɏ�������
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
			pcolors = (RGBQUAD *)dibp;                                     /* RGBQUAD�ւ̃|�C���^ */
			(unsigned char *)dibp += sizeof(RGBQUAD) * dib_ptr->bmiHeader.biClrUsed;
		}
		else
		{
			pcolors = NULL;
			(unsigned char*)dibp += 3;
		}

		// �p���b�g�R�s�[
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


		// �W�J�|�C���^�z����m��
		image = (png_bytepp)malloc(height * sizeof(png_bytep));

		for (i = 0; i < height; i++) {
			//        image[height-i-1] = (png_bytep) dibp + (i * ulRowBytes);
			image[i] = (png_bytep)dibp + (i * ulRowBytes);
		}
		png_read_image(png_ptr, image);							// �摜�f�[�^�̓W�J

		free(image);

		png_destroy_read_struct(								// libpng�\���̂̃��������
			&png_ptr, &info_ptr, (png_infopp)NULL);

		return (PDIB)dib_ptr;
	}

#define PNG_FILE_MAPPING 0                                              // FileMapping�@�\���g��

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

		// �t�@�C������PNG�f�[�^���w���|�C���^���H
		if (!png_sig_cmp((unsigned char *)szFile, (png_size_t)0, PNG_BYTES_TO_CHECK))
		{
			// ���������������璼�ړW�J
			pdib = pngptr2dib((void *)szFile);

			return pdib;
		}

#if PNG_FILE_MAPPING
		//--------------------------------------------
		// �t�@�C���}�b�s���O�I�u�W�F�N�g���g���Ă݂�
		//--------------------------------------------
		fh = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fh == INVALID_HANDLE_VALUE)
		{
			// �J���Ȃ�
			CloseHandle(fh);
			return NULL;
		}

		HANDLE mh = CreateFileMapping(fh, NULL, PAGE_READONLY, 0, 0, NULL);
		if (mh == NULL)
		{
			// �}�b�s���O�ł��Ȃ�
			CloseHandle(fh);
			return NULL;
		}
		CloseHandle(fh);

		// �}�b�s���O����
		png = (PDIB)MapViewOfFile(mh, FILE_MAP_READ, 0, 0, 0);

#else
		// PNG�t�@�C�����ۂ��ƊJ���ă������m��
		fh = CreateFile(szFile, GENERIC_READ, 0, 0,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fh == INVALID_HANDLE_VALUE)
		{
			// �J���Ȃ�
			CloseHandle(fh);
			return NULL;
		}
		fsize = SetFilePointer(fh, 0, NULL, FILE_END);		// �t�@�C���T�C�Y�̎擾
		SetFilePointer(fh, 0, NULL, FILE_BEGIN);			// �t�@�C���|�C���^��擪��

		// �������m��
		png = (PDIB)malloc(fsize);
		if (png == NULL)
		{
			CloseHandle(fh);
			return NULL;
		}

		// �ǂݍ���
		ReadFile(fh, png, fsize, &rbytes, NULL);
		CloseHandle(fh);

		// �S���ǂ߂Ȃ������H
		if (fsize != rbytes) return NULL;
#endif

		// PNG����DIB�ɓW�J
		pdib = pngptr2dib(png);

#if PNG_FILE_MAPPING
		// �}�b�s���O����
		CloseHandle(mh);
		UnmapViewOfFile(png);
#else
		// PNG�C���[�W�J��
		free(png);
#endif

		return pdib;
	}

#ifdef __cplusplus
}	// extern "C"
#endif



#endif // __PNGCTRL_H__
