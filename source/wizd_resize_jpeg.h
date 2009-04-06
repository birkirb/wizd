//
// jpeg resize tool for wizd
//
//	wizd_resize_jpeg.h
//
// (1) Original coding by K.Tanaka 2004/08/05
//
// 使用・配布・改変などすべて自己責任にてお願いします

// 以下の矩形サイズに収まるよう変換します(default)
#define	TARGET_JPEG_WIDTH	720
#define TARGET_JPEG_HEIGHT	480

// 画像を横長にします(1.0=等倍) (default)
#define WIDEN_RATIO		1.0

// とりあえず上限サイズを決めておきます
#define MAX_JPEG_WIDTH	50000
#define MAX_JPEG_HEIGHT	50000

//#define TYPE_JPEG		"image/jpeg"

typedef unsigned char UCHAR;
