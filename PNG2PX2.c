/////////////////////////////////////////////////////////////////////////////
// PNG2PX2.c
// 
// PNG to X68 Sprite Pattern Converter
// programmed by pirota
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "png.h"
#include "pngctrl.h"

#define INFILE_EXT  ".png"
#define OUTFILE_EXT ".PX2"
#define OUTFILEPAT_EXT ".PAT"
#define OUTFILEPAL_EXT ".PAL"

// PX2�t�@�C���\����
typedef struct
{
	unsigned short atr[256];										// �c�[���p�A�g���r���[�g
	unsigned short pal[256];										// X68K�p���b�g
	unsigned char  sprpat[0x8000];									// X68K�X�v���C�g�p�^�[��

} PX2FILE, * pPX2FILE;

static pPX2FILE outbuf = NULL;

static char infilename[256];
static char outfilename[256];
static char patfilename[256];
static char palfilename[256];

static int main_result = 0;

static int width;
static int height;

PDIB dibbuf = NULL;

static int opt_d = 0;													// �f�o�b�O�I�v�V����
static int opt_p = 0;												// �ׂ������I�v�V����



static int getfilesize(char *);
static int readjob(void);
static int cvjob(void);

// �g�p�@�̕\��
static void usage(void)
{
    printf("usage: PNG2PX2 infile[" INFILE_EXT "] OutFile\n"\
		   "\t-p\t�X�v���C�g�p�^�[��(PAT)/�p���b�g(PAL)�x�^�o��\n"
		   "\t-d\tDIB�t�@�C�����o�́i�f�o�b�O�p�j\n"
		   );

    exit(0);
}

/////////////////////////////////////
// main
/////////////////////////////////////
int main(int argc, char *argv[])
{
	int i;
	int ch;

    char drive[ _MAX_DRIVE ];
    char dir[ _MAX_DIR ];
    char fname[ _MAX_FNAME ];
    char ext[ _MAX_EXT ];

	// �R�}���h���C�����
	memset(infilename, 0, sizeof(infilename) );
	memset(outfilename, 0, sizeof(outfilename) );
	memset(patfilename, 0, sizeof(patfilename) );
	memset(palfilename, 0, sizeof(palfilename) );

    printf("PNG to PX2 Converter Ver0.11a " __DATE__ "," __TIME__ " Programmed by Pirota\n");

    if (argc <= 1)
	{
		usage();
	}


	for (i=1; i<argc; i++)
	{
		ch = argv[i][0];
		if (ch == '-' || ch == '/')
		{
			// �X�C�b�`
			switch (argv[i][1])
			{
			case 'd':
				opt_d = 1;
				break;
			case 'p':
				opt_p = 1;
				break;
			default:
				printf("-%c �I�v�V�������Ⴂ�܂��B\n",argv[i][1]);
				break;
			}

			continue;
		}
		// �t�@�C��������
		if (!infilename[0])
		{
			strcpy(infilename, argv[i]);
			_splitpath(infilename , drive, dir, fname, ext );
			if (ext[0]==0)
				strcat(infilename, INFILE_EXT);							// �g���q�⊮

			continue;
		}

		// �o�̓t�@�C�����̍쐬
		if (!outfilename[0])
		{
			// �o�̓t�@�C���l�[��
			strcpy(outfilename, argv[i]);
		}

	}
	// �o�̓t�@�C�������ȗ�����Ă���
	if (!outfilename[0])
	{
		// �o�̓t�@�C���l�[��
		strcpy(outfilename, fname);
	}

	_splitpath(outfilename, drive, dir, fname, ext);
	if (ext[0] == 0)
		strcat(outfilename, OUTFILE_EXT);							// �g���q�⊮

	// �ׂ������t�@�C���l�[��
	_splitpath(outfilename , drive, dir, fname, ext );
	strcpy(patfilename, fname);
	strcat(patfilename, OUTFILEPAT_EXT);							// �g���q�⊮

	strcpy(palfilename, fname);
	strcat(palfilename, OUTFILEPAL_EXT);							// �g���q�⊮



    // �t�@�C���ǂݍ��ݏ���
    if (readjob()<0)
		goto cvEnd;


	// �o�̓o�b�t�@�̊m��
	outbuf = (pPX2FILE) malloc(sizeof(PX2FILE));
	if (outbuf == NULL)
	{
		printf("�o�̓o�b�t�@�͊m�ۂł��܂���\n");
		goto cvEnd;
	}
	memset(outbuf, 0, sizeof(PX2FILE));

    // �ϊ�����
	if (cvjob() < 0)
	{
		goto cvEnd;
	}

cvEnd:
	// ��n��
	// PX2�o�̓o�b�t�@�J��
	if (outbuf != NULL)
	{
		free(outbuf);
	}

	// �p�b�N�o�b�t�@�J��
	if (dibbuf != NULL)
	{
		free(dibbuf);
	}

	return main_result;
}

//----------------------
// �t�@�C���T�C�Y���擾
//----------------------
static int getfilesize(char *fname)
{
    int result = -1;
    FILE * fp;

    fp = fopen(fname, "rb");
    if (fp == NULL)
        return result;

    fseek(fp, 0, SEEK_END);
    result = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fclose(fp);

    return result;
}

//----------------------------
// �o�m�f�ǂݍ��݂c�h�a�ɕϊ�
//----------------------------
// Out: 0=�n�j
//      -1 = �G���[
static int readjob(void)
{
	FILE *fp;
	BITMAPFILEHEADER bf;
	BITMAPINFOHEADER *bi;
	int bytes;

	dibbuf = PngOpenFile(infilename);
	if (dibbuf == NULL)
	{
		printf("Can't open '%s'.\n", infilename);
		return -1;
	}

	bi = (BITMAPINFOHEADER *)dibbuf;
	// �e�X�g
	memset(&bf, 0, sizeof(bf));
	bf.bfType = 'MB';
	bf.bfSize = sizeof(bf);
	bf.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*bi->biClrUsed;

	width = bi->biWidth;
	height = bi->biHeight;

	if (width != 256 || height != 256)
	{
		printf("Error:�摜�T�C�Y��256x256px �����󂯕t���܂���B\n");
		return -1;
	}

	if (bi->biBitCount != 8)
	{
		printf("Error:�摜�t�@�C���� �C���f�b�N�X�J���[ �����󂯕t���܂���B\n");
		return -1;
	}

	if (opt_d)
	{
		// �f�o�b�O�I�v�V������on�Ȃ�DIB�t�@�C���o��
		fp = fopen("test.bmp" , "wb");
		fwrite(&bf, 1, sizeof(bf), fp);

		bytes = bi->biSizeImage + (bi->biClrUsed * sizeof(RGBQUAD)) + sizeof(BITMAPINFOHEADER);

		fwrite(dibbuf, 1, bytes, fp);

		fclose(fp);
		printf("debug: 'test.bmp' wrote.\n");
	}

	return 0;
}


///////////////////////////////
// �p�^�[���f�[�^�ɕϊ�����
///////////////////////////////
static int cvjob(void)
{
	BITMAPINFOHEADER *bi;
	size_t a;
	u_char *pimg;
	u_char *outptr;
	u_char* atrptr;
	FILE *fp;

	bi = (BITMAPINFOHEADER *)dibbuf;

	// �p�^�[���ϊ�����
	outptr = outbuf->sprpat;										// �X�v���C�g�p�^�[���o�̓o�b�t�@
	atrptr = (u_char*)outbuf + 1;									// �A�g���r���[�g�o�̓o�b�t�@

	pimg = NULL;
	for (int yl=0; yl<height; yl+=16)
	{
		for (int xl=0; xl<width; xl+=8)
		{
			if ((xl & 15) == 0)
			{
				// �A�g���r���[�g��������
				pimg = (u_char*)dibbuf + (sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * bi->biClrUsed) + (yl * width) + xl;
				*atrptr = (((*pimg) & 0xF0) >> 4); // �G���f�B�A���l��
				atrptr += 2;
			}
			// �s�N�Z���ϊ�
			for (int y = 0; y < 16; y++)
			{
				pimg = (u_char*)dibbuf + (sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * bi->biClrUsed) + ((yl+y) * width) + xl;
				for (int x = 0; x < 8; x += 2)
				{
					unsigned char dot2 = (((*pimg) & 0x0F) << 4) | (*(pimg + 1) & 0x0F);
					pimg += 2;
					*(outptr++) = dot2;
				} // x
			} // y
		} // xl

	} // yl

	// �p���b�g�ϊ�
	RGBQUAD * dibpal = (RGBQUAD *)((u_char *) dibbuf + sizeof(BITMAPINFOHEADER));
	// GGGGG_RRRRR_BBBBB_I
	for (int i=0; i<256; i++)
	{
		RGBQUAD* paltmp = &dibpal[i];
		unsigned short x68pal = ((paltmp->rgbGreen >> 3) << 11) | ((paltmp->rgbRed >> 3) << 6) | ((paltmp->rgbBlue >> 3) << 1);
		outbuf->pal[i] = (x68pal>>8)|((x68pal & 0xFF) << 8);			// �G���f�B�A���ϊ�
	}

	if (!opt_p)
	{
		// PX2�t�@�C���o��
		fp = fopen(outfilename,"wb");
		if (fp == NULL)
		{
			printf("Can't write '%s'.\n", outfilename);
			return -1;
		}

		// PX2�p�^�[���o��
		a = fwrite(outbuf, 1, sizeof(PX2FILE), fp);
		if (a != (sizeof(PX2FILE)))
		{
			printf("'%s' �t�@�C�����������������߂܂���ł����I\n", outfilename);
		}

		fclose(fp);

		// ���ʏo��
		printf("PX2�f�[�^ '%s'���쐬���܂����B\n", outfilename);
	}
	else
	{
		// �x�^�t�@�C���o��
		fp = fopen(patfilename,"wb");
		if (fp == NULL)
		{
			printf("Can't write '%s'.\n", patfilename);
			return -1;
		}

		// PAT�p�^�[���o��
		a = fwrite(outbuf->sprpat, 1, sizeof(outbuf->sprpat), fp);
		if (a != (sizeof(outbuf->sprpat)))
		{
			printf("'%s' �t�@�C�����������������߂܂���ł����I\n", patfilename);
		}

		fclose(fp);

		// ���ʏo��
		printf("�p�^�[���f�[�^ '%s'���쐬���܂����B\n", patfilename);

		fp = fopen(palfilename, "wb");
		if (fp == NULL)
		{
			printf("Can't write '%s'.\n", palfilename);
			return -1;
		}

		// PAL�o��
		a = fwrite(outbuf->pal, 1, sizeof(outbuf->pal), fp);
		if (a != (sizeof(outbuf->pal)))
		{
			printf("'%s' �t�@�C�����������������߂܂���ł����I\n", palfilename);
		}

		fclose(fp);

		// ���ʏo��
		printf("�p���b�g�f�[�^ '%s'���쐬���܂����B\n", palfilename);

	}



	return 0;
}
