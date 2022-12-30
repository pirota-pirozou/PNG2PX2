# PNG2PX2.c

Version 0.1

## for Windows

このプログラムは、pngファイルからX68000用スプライトエディタ「ぴくせる君」向けスプライト・パターンデータ「PX2」ファイルに変換するものです。使っていた人は少ないと思いますので解説しますと、

PX2ファイルは基本的に、パターンデータとパレットデータが結合されたものです。構造体で表すと下記のようになります。

```
// PX2ファイル構造体
typedef struct
{
	unsigned short atr[256];	// ツール用アトリビュート
	unsigned short pal[256];	// X68Kパレット
	unsigned char  sprpat[0x8000];	// X68Kスプライトパターン
} PX2FILE, * pPX2FILE;
```

### ビルドに必要なもの

Visual Studio 2022と、zlib、libpngを用意してください。

### 使用法

png2px2 [オプション] filename[.png]

### オプション

-p  PX2ファイルの代わりに、X68000用スプライトパターン２５６個(PAT)、２５６色分のパレット(PAL)のベタファイルを書きだします。

### 注意

- pngファイルのフォーマットはインデックスカラー(256色)、横256ピクセル、縦256ピクセル決め打ちです。
EDGEなどのドット絵ツールを使って作成すると良いでしょう。

- X68000のスプライトは１枚につき１６色（透明色＋１５色）です。 EDGEのパレット横１ライン１６色が、スプライトのパレットとして使えます。全部で１６本のパレットがあります。

- 先頭の１６色パレットはテキスト画面と共用となりますので注意してください。

### おまけ

PXLOOK.R

X68000環境で使うPX2ファイルビューアーです。

---

By Pirota
