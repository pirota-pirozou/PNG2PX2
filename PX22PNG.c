/////////////////////////////////////////////////////////////////////////////
// PX22PNG.c
// 
// X68 to PNG Sprite Pattern Converter
// programmed by pirota
/////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
#include <libgen.h>
#endif
#include "BMPLoad256.h"

#include "png.h"
#include "pngctrl.h"

#define INFILE_EXT  ".PX2"
#define OUTFILE_EXT ".png"

#ifndef _MAX_PATH
#define _MAX_PATH   256
#endif
#ifndef _MAX_DRIVE
#define _MAX_DRIVE  3
#endif
#ifndef _MAX_DIR
#define _MAX_DIR	256
#endif
#ifndef _MAX_FNAME
#define _MAX_FNAME	256
#endif
#ifndef _MAX_EXT
#define _MAX_EXT	256
#endif

// PX2ファイル構造体
typedef struct
{
	unsigned short atr[256];										// ツール用アトリビュート
	unsigned short pal[256];										// X68Kパレット
	unsigned char  sprpat[0x8000];									// X68Kスプライトパターン

} PX2FILE, * pPX2FILE;

static pPX2FILE px2buf = NULL;
static IMAGEDATA *imgbuf = NULL;


static char infilename[256];
static char outfilename[256];

static int main_result = 0;

static int width;
static int height;

static int opt_d = 0;													// デバッグオプション


static int getfilesize(char *);
static int readjob(void);
static int cvjob(void);

// 使用法の表示
static void usage(void)
{
    printf("usage: PX22PNG infile[" INFILE_EXT "] OutFile\n"\
//		   "\t-d\tDIBファイルも出力（デバッグ用）\n"
		   );

    exit(0);
}

#ifndef __WIN32__
void my_splitpath(char* filename, char* drive, char *dir, char *fname, char *ext)
{
	// ファイル名のパスを取得
	char *path_copy = strdup(filename);
	drive[0] = '\0';
	strcpy(dir, dirname(path_copy));
	char *_drive = dirname(path_copy);
	char *file_copy = strdup(filename);
	char *filename_ext = basename(file_copy);
	char *p = dirname(filename);
	// Split filename and extension
	char *dot = strrchr(filename_ext, '.');
	if (dot)
	{
		strncpy(fname, filename_ext, dot - filename_ext);
		fname[dot - filename_ext] = '\0';
		strcpy(ext, dot);
	}
	else
	{
		strcpy(fname, filename_ext);
		ext[0] = '\0';
	}	
}
#endif

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

	// コマンドライン解析
	memset(infilename, 0, sizeof(infilename) );
	memset(outfilename, 0, sizeof(outfilename) );

    printf("PX2 to PNG Converter Ver0.1a " __DATE__ "," __TIME__ " Programmed by Pirota\n");

    if (argc <= 1)
	{
		usage();
	}


	for (i=1; i<argc; i++)
	{
		ch = argv[i][0];
		if (ch == '-' || ch == '/')
		{
			// スイッチ
			switch (argv[i][1])
			{
			case 'd':
				opt_d = 1;
				break;
			default:
				printf("-%c オプションが違います。\n",argv[i][1]);
				break;
			}

			continue;
		}
		// ファイル名入力
		if (!infilename[0])
		{
			strcpy(infilename, argv[i]);
#ifndef __WIN32__
			my_splitpath(infilename , drive, dir, fname, ext );
#else
			_splitpath(infilename , drive, dir, fname, ext );
#endif			
			if (ext[0]==0)
				strcat(infilename, INFILE_EXT);							// 拡張子補完

			continue;
		}

		// 出力ファイル名の作成
		if (!outfilename[0])
		{
			// 出力ファイルネーム
			strcpy(outfilename, argv[i]);
		}

	}
	// 出力ファイル名が省略されてたら
	if (!outfilename[0])
	{
		// 出力ファイルネーム
		strcpy(outfilename, fname);
	}

#ifndef __WIN32__
	my_splitpath(outfilename , drive, dir, fname, ext );
#else	
	_splitpath(outfilename, drive, dir, fname, ext);
#endif	
	if (ext[0] == 0)
		strcat(outfilename, OUTFILE_EXT);							// 拡張子補完

	// 入力バッファの確保
	px2buf = (pPX2FILE)malloc(sizeof(PX2FILE));
	if (px2buf == NULL)
	{
		printf("入力バッファは確保できません\n");
		goto cvEnd;
	}
	memset(px2buf, 0, sizeof(PX2FILE));

	// 変換元バッファの確保
	width = 256;
	height = 256;
	imgbuf = (IMAGEDATA *)malloc(width * height);
	if (imgbuf == NULL)
	{
		printf("出力バッファは確保できません\n");
		goto cvEnd;
	}
	memset(imgbuf, 0, sizeof(IMAGEDATA));
	imgbuf->width = width;
	imgbuf->height = height;
	imgbuf->palette_num = 256;
	imgbuf->palette = malloc(sizeof(color_t) * 256);
	imgbuf->map = alloc_map(imgbuf);
	if (imgbuf->map == NULL)
	{
		printf("出力バッファは確保できません\n");
		goto cvEnd;
	}

	// ファイル読み込み処理
	if (readjob() < 0)
	{
		goto cvEnd;
	}

    // 変換処理
	if (cvjob() < 0)
	{
		goto cvEnd;
	}

cvEnd:
	if (imgbuf->map != NULL)
	{
		free_map(imgbuf);
		imgbuf->map = NULL;
	}

	// 後始末
	// 入力バッファ開放
	if (px2buf != NULL)
	{
		free(px2buf);
	}

	// イメージバッファ開放
	if (imgbuf != NULL)
	{
		free(imgbuf);
	}

	return main_result;
}

//----------------------
// ファイルサイズを取得
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
// ＰＮＧ読み込みＤＩＢに変換
//----------------------------
// Out: 0=ＯＫ
//      -1 = エラー
static int readjob(void)
{
	FILE* fp;
	size_t a;

	// PX2ファイル入力
	fp = fopen(infilename, "rb");
	if (fp == NULL)
	{
		printf("Can't read '%s'.\n", infilename);
		return -1;
	}

	// PX2パターン入力
	a = fread(px2buf, 1, sizeof(PX2FILE), fp);
	if (a != (sizeof(PX2FILE)))
	{
		printf("'%s' ファイルが正しく読み込めませんでした！\n", infilename);
	}

	fclose(fp);

	return 0;
}


///////////////////////////////
// パターンデータに変換処理
///////////////////////////////
static int cvjob(void)
{
	u_char* inptr;
	u_char* atrptr;

	width = imgbuf->width;
	height = imgbuf->height;

	// パターン変換処理
	inptr = px2buf->sprpat;											// スプライトパターンバッファ
	atrptr = (u_char*)px2buf + 1;									// アトリビュートバッファ

	unsigned char aval = 0;
	for (int yl=0; yl<height; yl+=16)
	{
		for (int xl=0; xl<width; xl+=8)
		{
			if ((xl & 15) == 0)
			{
				// アトリビュート読み込み
				aval = ((*atrptr) & 0x0F) << 4;
				atrptr += 2;
			}
			// ピクセル変換
			for (int y = 0; y < 16; y++)
			{
				for (int x = 0; x < 8; x += 2)
				{
					unsigned char dot1 = ((*inptr) & 0xF0) >> 4;
					unsigned char dot2 = (*inptr) & 0x0F;
					imgbuf->map[yl + y][xl + x] = aval | dot1;
					imgbuf->map[yl + y][xl + x + 1] = aval | dot2;
					inptr++;
				} // x
			} // y
		} // xl

	} // yl

	// パレット変換
	// GGGGG_RRRRR_BBBBB_I
	for (int i=0; i<256; i++)
	{
		color_t* pngpal = &imgbuf->palette[i];
		unsigned short x68pal = ((px2buf->pal[i]) >> 8) | ((px2buf->pal[i] & 0x00FF) << 8); // エンディアン変換
		x68pal >>= 1;		// 輝度ビット削除
		pngpal->a = 255;
		pngpal->b = (x68pal & 31) << 3;
		pngpal->r = (x68pal & (31 << 5)) >> 2;
		pngpal->g = (x68pal & (31 << 10)) >> 7;		// 0x4000 | 0x2000 | 0x1000
	}
	imgbuf->palette[0].a = 0;

	// pngを書き込み（インデックスカラー専用）
	int r = writepng(outfilename, imgbuf);
	if (r == -1)
	{
		printf("%s を書き込みできませんでした。\n", outfilename);
		return -1;
	}
	printf("%s を書き込みました。\n", outfilename);

	return 0;
}
