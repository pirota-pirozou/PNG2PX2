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

// PX2ファイル構造体
typedef struct
{
	unsigned short atr[256];										// ツール用アトリビュート
	unsigned short pal[256];										// X68Kパレット
	unsigned char  sprpat[0x8000];									// X68Kスプライトパターン

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

static int opt_d = 0;													// デバッグオプション
static int opt_p = 0;												// べた書きオプション



static int getfilesize(char *);
static int readjob(void);
static int cvjob(void);

// 使用法の表示
static void usage(void)
{
    printf("usage: PNG2PX2 infile[" INFILE_EXT "] OutFile\n"\
		   "\t-p\tスプライトパターン(PAT)/パレット(PAL)ベタ出力\n"
		   "\t-d\tDIBファイルも出力（デバッグ用）\n"
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

	// コマンドライン解析
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
			// スイッチ
			switch (argv[i][1])
			{
			case 'd':
				opt_d = 1;
				break;
			case 'p':
				opt_p = 1;
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
			_splitpath(infilename , drive, dir, fname, ext );
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

	_splitpath(outfilename, drive, dir, fname, ext);
	if (ext[0] == 0)
		strcat(outfilename, OUTFILE_EXT);							// 拡張子補完

	// べた書きファイルネーム
	_splitpath(outfilename , drive, dir, fname, ext );
	strcpy(patfilename, fname);
	strcat(patfilename, OUTFILEPAT_EXT);							// 拡張子補完

	strcpy(palfilename, fname);
	strcat(palfilename, OUTFILEPAL_EXT);							// 拡張子補完



    // ファイル読み込み処理
    if (readjob()<0)
		goto cvEnd;


	// 出力バッファの確保
	outbuf = (pPX2FILE) malloc(sizeof(PX2FILE));
	if (outbuf == NULL)
	{
		printf("出力バッファは確保できません\n");
		goto cvEnd;
	}
	memset(outbuf, 0, sizeof(PX2FILE));

    // 変換処理
	if (cvjob() < 0)
	{
		goto cvEnd;
	}

cvEnd:
	// 後始末
	// PX2出力バッファ開放
	if (outbuf != NULL)
	{
		free(outbuf);
	}

	// パックバッファ開放
	if (dibbuf != NULL)
	{
		free(dibbuf);
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
	// テスト
	memset(&bf, 0, sizeof(bf));
	bf.bfType = 'MB';
	bf.bfSize = sizeof(bf);
	bf.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*bi->biClrUsed;

	width = bi->biWidth;
	height = bi->biHeight;

	if (width != 256 || height != 256)
	{
		printf("Error:画像サイズは256x256px しか受け付けません。\n");
		return -1;
	}

	if (bi->biBitCount != 8)
	{
		printf("Error:画像ファイルは インデックスカラー しか受け付けません。\n");
		return -1;
	}

	if (opt_d)
	{
		// デバッグオプションがonならDIBファイル出力
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
// パターンデータに変換処理
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

	// パターン変換処理
	outptr = outbuf->sprpat;										// スプライトパターン出力バッファ
	atrptr = (u_char*)outbuf + 1;									// アトリビュート出力バッファ

	pimg = NULL;
	for (int yl=0; yl<height; yl+=16)
	{
		for (int xl=0; xl<width; xl+=8)
		{
			if ((xl & 15) == 0)
			{
				// アトリビュート書き込み
				pimg = (u_char*)dibbuf + (sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * bi->biClrUsed) + (yl * width) + xl;
				*atrptr = (((*pimg) & 0xF0) >> 4); // エンディアン考慮
				atrptr += 2;
			}
			// ピクセル変換
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

	// パレット変換
	RGBQUAD * dibpal = (RGBQUAD *)((u_char *) dibbuf + sizeof(BITMAPINFOHEADER));
	// GGGGG_RRRRR_BBBBB_I
	for (int i=0; i<256; i++)
	{
		RGBQUAD* paltmp = &dibpal[i];
		unsigned short x68pal = ((paltmp->rgbGreen >> 3) << 11) | ((paltmp->rgbRed >> 3) << 6) | ((paltmp->rgbBlue >> 3) << 1);
		outbuf->pal[i] = (x68pal>>8)|((x68pal & 0xFF) << 8);			// エンディアン変換
	}

	if (!opt_p)
	{
		// PX2ファイル出力
		fp = fopen(outfilename,"wb");
		if (fp == NULL)
		{
			printf("Can't write '%s'.\n", outfilename);
			return -1;
		}

		// PX2パターン出力
		a = fwrite(outbuf, 1, sizeof(PX2FILE), fp);
		if (a != (sizeof(PX2FILE)))
		{
			printf("'%s' ファイルが正しく書き込めませんでした！\n", outfilename);
		}

		fclose(fp);

		// 結果出力
		printf("PX2データ '%s'を作成しました。\n", outfilename);
	}
	else
	{
		// ベタファイル出力
		fp = fopen(patfilename,"wb");
		if (fp == NULL)
		{
			printf("Can't write '%s'.\n", patfilename);
			return -1;
		}

		// PATパターン出力
		a = fwrite(outbuf->sprpat, 1, sizeof(outbuf->sprpat), fp);
		if (a != (sizeof(outbuf->sprpat)))
		{
			printf("'%s' ファイルが正しく書き込めませんでした！\n", patfilename);
		}

		fclose(fp);

		// 結果出力
		printf("パターンデータ '%s'を作成しました。\n", patfilename);

		fp = fopen(palfilename, "wb");
		if (fp == NULL)
		{
			printf("Can't write '%s'.\n", palfilename);
			return -1;
		}

		// PAL出力
		a = fwrite(outbuf->pal, 1, sizeof(outbuf->pal), fp);
		if (a != (sizeof(outbuf->pal)))
		{
			printf("'%s' ファイルが正しく書き込めませんでした！\n", palfilename);
		}

		fclose(fp);

		// 結果出力
		printf("パレットデータ '%s'を作成しました。\n", palfilename);

	}



	return 0;
}
