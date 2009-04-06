//
// jpeg resize module for wizd
//
// 	wizd_resize_jpeg.c
//
// (1) Original coding by K.Tanaka	2004/08/05
// (2) revised by 349			2004/08/07
// (3) revised by K.Tanaka		2004/08/09
//
// 使用・配布・改変などすべて自己責任にてお願いします

#ifdef RESIZE_JPEG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <math.h>
#include <jpeglib.h>
#include <jerror.h>

#include "wizd.h"
#include "wizd_resize_jpeg.h"

//#define NEW_METHOD

#ifdef NEW_METHOD
#ifndef UCHAR
#define UCHAR unsigned char
#endif
#ifndef ULONG
#define ULONG unsigned long
#endif
typedef struct{
  ULONG r;
  ULONG g;
  ULONG b;
} Calc;
#endif

// static変数
// destination mgr とのやりとり用です
UCHAR	*pOutputMemory;	// JPEGデータを置いておく場所
int	nMaxMemory;	// pOutputMemoryの最大サイズ
int	nUsedMemory = 0;// 使用されたメモリ容量(作成されたJPEGサイズ)
int	nError = 0;	// 何かエラーがでた場合にセット
static struct jpeg_destination_mgr	dest_mgr;

// Destination Mgr 初期化メソッド (オンメモリ出力用)
static void init_destination (j_compress_ptr cinfo) {
    cinfo->dest->next_output_byte = pOutputMemory;
    cinfo->dest->free_in_buffer = nMaxMemory;
}

// Destination Mgr バッファfullメソッド -> errorにする
// 最初は小さめにとっておいてreallocとかするほうがいいかも
static boolean empty_output_buffer(j_compress_ptr cinfo) {
    debug_log_output("memory exhausted\n");
    nError = 1;
    cinfo->dest->next_output_byte = pOutputMemory; // とりあえずリセット
    return FALSE;
}

// Destination Mgr 終了メソッド
static void term_destination(j_compress_ptr cinfo) {
    nUsedMemory = nMaxMemory - cinfo->dest->free_in_buffer;
}

// Destination Managerの初期化
// 中間ファイルは作りたくないので、
// 生成したJPEGをオンメモリで持っておくようにする
static void
setupDestManager(j_compress_ptr cinfo, UCHAR *pBuf, int nBufMax) {
    pOutputMemory = pBuf;
    nMaxMemory = nBufMax;
    dest_mgr.init_destination = &init_destination;
    dest_mgr.empty_output_buffer = &empty_output_buffer;
    dest_mgr.term_destination = &term_destination;
    cinfo->dest = &dest_mgr;
    nError = 0;
}

// どのサイズへリサイズするかを計算する
static void
calcSize(int nOrgW, int nOrgH, int nFitW, int nFitH, double dWRatio, int nCrop, int *pnNewW, int *pnNewH, int *pnCropW, int *pnCropH) {
    double dResizeW, dResizeH, dResize;

    dResizeW = (double)nOrgW * dWRatio;
    dResizeH = (double)nOrgH;

    // Get the aspect ratio difference from source to destination
    // Example: nFitW=1600, nFitH=900   (16:9 aspect ratio destination)
    //          dResizeW=400, dResizeH=300  (4:3 aspect ratio source)
    //          nCrop = 10
    //    then  dAspect = (300*1600)/(900*400) = 4/3
    //      so  we can expand nFitH by the full 10% to allow vertical cropping
    dResize = (dResizeH*nFitW)/(nFitH*dResizeW);
    // Allow up to N% cropping to reduce side bars and/or letterboxing
    if(dResize > (1.0+0.01*nCrop)) {
	// Use the crop limit, but will retain some side bars
	*pnCropH = (int)floor(nFitH*(0.01*nCrop)+0.5);
	*pnCropW = 0;
    } else if(dResize > 1.0) {
	// Can eliminate side bars by cropping top/bottom appropriately
	*pnCropH = (int)floor(nFitH*(dResize-1.0)+0.5);
	*pnCropW = 0;
    } else if(dResize < (1.0-0.01*nCrop)) {
	// Use the crop limit, but will retain some letterboxing
	*pnCropW = (int)floor(nFitW*(0.01*nCrop)+0.5);
	*pnCropH = 0;
    } else {
	// Can eliminate letterbox by cropping left/right appropriately
	*pnCropW = (int)floor(nFitW*(1.0-dResize)+0.5);
	*pnCropH = 0;
    }

    if ((dResizeW < nFitW) && (dResizeH < nFitH)) {
	*pnNewW = dResizeW;
	*pnNewH = dResizeH;
	*pnCropW = 0;
	*pnCropH = 0;
    } else {
    	dResizeW /= (nFitW + *pnCropW);
    	dResizeH /= (nFitH + *pnCropH);

    	dResize = MAX(dResizeW, dResizeH);

    	*pnNewW = MIN(nFitW, (int)((double)nOrgW * dWRatio / dResize));
    	*pnNewH = MIN(nFitH, (int)((double)nOrgH / dResize));
    }

    // Make sure the values are multiples of 16 for proper JPEG compression
    if(*pnNewW & 0xf) {
	//fprintf(stderr, "Rounding W from %d to %d\n", *pnNewW, (*pnNewW & 0xFFFFFFF0)+0x10);
	*pnNewW = (*pnNewW & 0xFFFFFFF0)+0x10;
    }
    if(*pnNewH & 0xf) {
	//fprintf(stderr, "Rounding H from %d to %d\n", *pnNewH, (*pnNewH & 0xFFFFFFF0)+0x10);
	*pnNewH = (*pnNewH & 0xFFFFFFF0)+0x10;
    }
}

#ifdef NEW_METHOD
//-------------------------------------------------------------------------
int ydesimation(UCHAR *pin ,Calc *pout,int *l,int srcy,int dsty,int srcx)
{
  static ULONG error=0;
  int x;

  for (x=0 ; x<srcx ; x++){
    pout[x].r += pin[x*3  ];
    pout[x].g += pin[x*3+1];
    pout[x].b += pin[x*3+2];
  }

  (*l)++;

  if (srcy > dsty){
    error +=dsty;
    if (error > srcy){
      error -= srcy;
      return 1;
    }
  }else{
    return 1;
  }

  return 0;
}

void xdesimation(Calc *pin,UCHAR *pout,int src,int dst , int lnum)
{
  int k,x,dx,nx,lx;
  Calc *pat  = (Calc *)calloc(src,sizeof(Calc));

  x=0;
  dx = dst;
  for(lx=0;lx<dst;lx++){
    nx =0;
    do{
      pat[lx].r += (dx * pin[x].r);
      pat[lx].g += (dx * pin[x].g);
      pat[lx].b += (dx * pin[x].b);

      nx+=dx;
      if (dx<dst)
	dx = dst;
      x++;
    }while(nx<src);

    //半端処理
    x--;
    k=(nx-src);
    pat[lx].r -= (k * pin[x].r);
    pat[lx].g -= (k * pin[x].g);
    pat[lx].b -= (k * pin[x].b);
    dx=k;

    //出力バッファにコピー
    pout[lx*3  ] = (UCHAR)(pat[lx].r / src / lnum);
    pout[lx*3+1] = (UCHAR)(pat[lx].g / src / lnum);
    pout[lx*3+2] = (UCHAR)(pat[lx].b / src / lnum);
  }

  free(pat);
}

static int
resize(j_decompress_ptr cinfoIN, j_compress_ptr cinfoOUT, int nWidth, int nHeight)
{
    int		y;
    JSAMPROW	in_row_ptr[1], out_row_ptr[1];
    UCHAR	*pin, *pout;
    int		nScaleX, nScaleY;
    int		i , l;
    Calc *px;

#ifdef DEBUG
    int   x;
    FILE *fpw;
    fpw = fopen("./result.ppm","wb");
#endif

    //このへんいじると画質と速度が変わります
    //適当にtuneしてください
    //詳細は jpeglib.hをみてね
    //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    cinfoIN->dither_mode = JDITHER_ORDERED;
    cinfoIN->dct_method = JDCT_IFAST;
    cinfoIN->do_block_smoothing = FALSE;
    cinfoOUT->dct_method = JDCT_IFAST;
    //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    cinfoOUT->image_width = nWidth;
    cinfoOUT->image_height = nHeight;
    cinfoOUT->input_components = 3;
    cinfoOUT->in_color_space = JCS_RGB;

    jpeg_set_defaults(cinfoOUT);
    jpeg_start_decompress(cinfoIN);
    jpeg_start_compress(cinfoOUT, TRUE);

    nScaleX = cinfoIN->image_width;
    nScaleY = cinfoIN->image_height;

    pin = (UCHAR *)malloc(nScaleX * 3);
    in_row_ptr[0] = pin;
    pout = (UCHAR *)calloc(nWidth * 3,sizeof(UCHAR));
    out_row_ptr[0] = pout;

#ifdef DEBUG
    /* PPM Header */
    fprintf(fpw,"P6%c%03d %03d%c255%c",0x0a,nWidth,nHeight,0x0a,0x0a);
#endif

    for(y=0;y<nScaleY;){
      px   = (Calc *)calloc(nScaleX,sizeof(Calc));

      for(i=0,l=0;(l==0)&(y<nScaleY);y++){

	/* line read */
	jpeg_read_scanlines(cinfoIN, in_row_ptr, 1);

	/* Y desimation */
	l=ydesimation(pin , px , &i , nScaleY , nHeight , nScaleX);
      }

      /* X desimation */
      xdesimation(px ,pout , nScaleX , nWidth ,i);
      free(px);

      /* line write */
      jpeg_write_scanlines(cinfoOUT, out_row_ptr, 1);

#ifdef DEBUG
      for (x=0 ; x<nWidth ; x++){
	fputc(pout[x*3  ],fpw);
	fputc(pout[x*3+1],fpw);
	fputc(pout[x*3+2],fpw);
      }
#endif
    }

#ifdef DEBUG
    fclose(fpw);
#endif

    free(pin);
    free(pout);

    return 0;
}
//-------------------------------------------------------------------------
#else /* NEW_METHOD */

// リサイズ処理実行
// 1024倍して固定小数点演算のつもり
static int
resize(j_decompress_ptr cinfoIN, j_compress_ptr cinfoOUT, int nWidth, int nHeight, int nCropW, int nCropH) {
    int		x, y, orgX, orgY, srcY, srcX, srcXK;
    JSAMPROW	in_row_ptr[1], out_row_ptr[1];
    UCHAR	*pin1, *pin2, *pout, *p;
    UCHAR	*pline[2];
    int		nScaleX, nScaleY;
    int		dx, dy, idx0, idx1, i;
    int		weight00, weight01, weight10, weight11, weight;
    int		nStartX,nEndX,nStartY,nEndY;

    if (cinfoIN->num_components != 3 ||
	cinfoIN->image_width > MAX_JPEG_WIDTH ||
	cinfoIN->image_height > MAX_JPEG_HEIGHT) {

	debug_log_output("not supported type of jpeg file\n");
	return -1;
    }
    cinfoOUT->image_width = nWidth;
    cinfoOUT->image_height = nHeight;
    cinfoOUT->input_components = 3;
    cinfoOUT->in_color_space = JCS_RGB;
    jpeg_set_defaults(cinfoOUT);
    jpeg_start_decompress(cinfoIN);
    jpeg_start_compress(cinfoOUT, TRUE);

    pin1 = (UCHAR *)malloc(cinfoIN->image_width * 3);
    pin2 = (UCHAR *)malloc(cinfoIN->image_width * 3);
    pout = (UCHAR *)malloc(nWidth * 3);
    if (pin1 == NULL || pin2 == NULL || pout == NULL) {
	debug_log_output("scan buffer malloc error\n");
	return -1;
    }
    in_row_ptr[0] = pin1;
    out_row_ptr[0] = pout;
    // Set up the cropping
    nStartX = nCropW/2;
    nEndX = nStartX+nWidth;
    nStartY = nCropH/2;
    nEndY = nStartY+nHeight;
    nScaleX = cinfoIN->image_width * 1024 / (nWidth+nCropW);
    nScaleY = cinfoIN->image_height * 1024 / (nHeight+nCropH);
    jpeg_read_scanlines(cinfoIN, in_row_ptr, 1);
    for (x = y = orgX = orgY = 0; y < nEndY; y++) {
	srcY = (y * nScaleY) >> 10;
	dy = y * nScaleY - (srcY << 10); // 次へ はみ出してるサイズ(1024倍済)
	// ターゲット行と、その次の行を読むまでスキップ
	while (orgY <= srcY && orgY < cinfoIN->image_height - 1) {
	    // pin1/pin2を交互に使用..
	    if (in_row_ptr[0] == pin1)
		in_row_ptr[0] = pin2;
	    else
		in_row_ptr[0] = pin1;
	    jpeg_read_scanlines(cinfoIN, in_row_ptr, 1);
	    orgY++;
	}
	// pin1/pin2どっちが上の行にあたるか...
	if (in_row_ptr[0] == pin1) {
	    pline[0] = pin2;
	    pline[1] = pin1;
	} else {
	    pline[0] = pin1;
	    pline[1] = pin2;
	}
	// 1ライン分のリサイズ処理(最右ピクセルはループ後設定)
	// Only output the columns which aren't cropped
	for (x = nStartX, srcXK = 0, p = pout; x < (nEndX-1); x++, srcXK += nScaleX) {
	    srcX = srcXK >> 10;
	    // dx = 右隣のピクセルにはみ出してるサイズ(1024倍済)
	    dx = srcXK - (srcX << 10); // 右隣へ はみ出してるサイズ(1024倍済)
	    // 面積比例配分
	    weight00 = (1024 - dx) * (1024 - dy);
	    weight01 = (dx       ) * (1024 - dy);
	    weight10 = (1024 - dx) * (dy       );
	    weight11 = (       dx) * (dy       );
	    weight   = weight00 + weight01 + weight10 + weight11;
	    idx0 = (srcX + 0) * 3;
	    idx1 = (srcX + 1) * 3;
	    for (i = 0; i < 3; i++, idx0++, idx1++) {
	    	*p++ =
		    (pline[0][idx0] * weight00 +
		     pline[0][idx1] * weight01 +
		     pline[1][idx0] * weight10 +
		     pline[1][idx1] * weight11) / weight;
	    }
	}
	// 最右ピクセル（はみ出すとイヤだし面倒なので補完なし）
	srcX = srcXK >> 10;
	*p++ = pline[0][srcX * 3];
	*p++ = pline[0][srcX * 3 + 1];
	*p++ = pline[0][srcX * 3 + 2];
	// Only output the lines which aren't cropped
	if((y>=nStartY) && (y<nEndY))
		jpeg_write_scanlines(cinfoOUT, out_row_ptr, 1);
    }
    // 読み残しを全部読んでしまう -> 無駄なのでやめときます(finishしちゃだめ!)
//    while (cinfoIN->output_scanline < cinfoIN->image_height) {
//	jpeg_read_scanlines(cinfoIN, in_row_ptr, 1);
//    }
    free(pin1);
    free(pin2);
    free(pout);
    return 0;
}

#endif /* NEW_METHOD */

// 指定矩形(nFitW, nFitH)に収まるようJPEGを変換
// 同時に横方向へ変形(dWidenRatio倍する)
static int
resizeJpeg(			// RET:JPEGデータサイズ(0=error)
    char	*pszOrgJpeg,	// IN: 元JPEGファイル名
    int		nFitW,		// IN: ターゲットJPEG横幅
    int		nFitH,		// IN: ターゲットJPEG高さ
    UCHAR	*pBuf,		// OUT:変換後JPEGデータ格納場所
    int		nBufMax,	// IN: pBufのサイズ
    int		nCrop,		// IN: Allowable amount of cropping, in percent
    double	dWidenRatio)	// IN: 横方向拡大率(1.0=等倍)
{
    // libjpeg関連変数
    struct jpeg_decompress_struct	cinfoIN;
    struct jpeg_compress_struct		cinfoOUT;
    struct jpeg_error_mgr		jerrIN, jerrOUT;
    // その他
    FILE	*fpIN;
    int		nResizedW, nResizedH;
    int		nCropW, nCropH;
    unsigned char tag[2];
    int		ret;

    cinfoIN.err = jpeg_std_error(&jerrIN);
    cinfoOUT.err = jpeg_std_error(&jerrOUT);
    jpeg_create_decompress(&cinfoIN);
    jpeg_create_compress(&cinfoOUT);
    if (NULL == (fpIN = fopen(pszOrgJpeg, "rb"))) {
	debug_log_output("cannot open src jpeg (%s)\n", pszOrgJpeg);
	return 0;
    }
    // Double-check to make sure this is a JPEG image
    // because libjpeg crashes if it isn't
    fread(tag, 1, 2, fpIN);
    fseek(fpIN, 0, SEEK_SET);
    debug_log_output("src jpeg (%s) tag is %02x, %02x\n", pszOrgJpeg, tag[0], tag[1]);
	
    if((tag[0] != 0xFF) || (tag[1] != 0xd8)) {
 	debug_log_output("src jpeg (%s) is not valid, tag is %02x, %02x\n", pszOrgJpeg, tag[0], tag[1]);
	return 0;
    }
   
    jpeg_stdio_src(&cinfoIN, fpIN);
    ret = jpeg_read_header(&cinfoIN, TRUE);
    if(ret < 0) {
 	debug_log_output("jpeg_read_header(%s) returned %d\n", pszOrgJpeg, ret);
	return 0;
    }
    debug_log_output("Original size (%dx%d)\n", cinfoIN.image_width, cinfoIN.image_height);

    setupDestManager(&cinfoOUT, pBuf, nBufMax);
    calcSize(cinfoIN.image_width, cinfoIN.image_height,
	     nFitW, nFitH, dWidenRatio, nCrop, &nResizedW, &nResizedH, &nCropW, &nCropH);
    debug_log_output("(%d,%d)->(%d,%d)\n",
	    cinfoIN.image_width, cinfoIN.image_height, nResizedW, nResizedH);

    if (0 != resize(&cinfoIN, &cinfoOUT, nResizedW, nResizedH, nCropW, nCropH)) {
	debug_log_output("resize error\n");
	return 0;
    }

//  decompressは中途半端にやめる可能性があるので...
//    jpeg_finish_decompress(&cinfoIN);
    jpeg_finish_compress(&cinfoOUT);

    jpeg_destroy_decompress(&cinfoIN);
    jpeg_destroy_compress(&cinfoOUT);

    if (nError == 0)
	return nUsedMemory;
    else
	return 0;
}


static void
setupTargetSize(int *pnTargetW, int *pnTargetH, int *pnCrop, double *pdWidenRatio) {
    *pnTargetW = (global_param.target_jpeg_width != 0 ) ?
	global_param.target_jpeg_width : TARGET_JPEG_WIDTH;
    *pnTargetH = (global_param.target_jpeg_height != 0 ) ?
	global_param.target_jpeg_height : TARGET_JPEG_HEIGHT;
    *pnCrop = global_param.allow_crop;
    *pdWidenRatio = (global_param.widen_ratio != 0.0 ) ?
	global_param.widen_ratio : WIDEN_RATIO;
	
}

// 指定されたJPEGファイルをリサイズして、
// fdで指定されたdescriptorへ出力します。ヘッダもまとめて出力します。
// ターゲットサイズは global_param を見ます。
// -> target_jpeg_width, target_jpeg_height, widen_ratio
int
http_send_resized_jpeg(int fd, HTTP_RECV_INFO *http_recv_info_p) {
    UCHAR	*pNewJpeg;
    int		nTargetWidth, nTargetHeight;
    double	dWidenRatio;
    int		nCrop;
    int		nMaxBuf, nJpegSize;

    // if (http_recv_info_p->mime_  wanna mime check here...

    setupTargetSize(&nTargetWidth, &nTargetHeight, &nCrop, &dWidenRatio);
    // 生成したJPEGを置いておくバッファ。
    // いくら何でも圧縮率50%くらいにはなるでしょうという甘い考え。
    nMaxBuf = (nTargetWidth * nTargetHeight * 3) / 2;
    debug_log_output("Target (%d,%d) widening (%.2f%%) :",
	    nTargetWidth, nTargetHeight, dWidenRatio * 100);
    pNewJpeg = (UCHAR *)malloc(nMaxBuf);
    if (NULL == pNewJpeg) {
	debug_log_output("memory allocation error\n");
	return 0;
    }
    nJpegSize = resizeJpeg(http_recv_info_p->send_filename,
			   nTargetWidth, nTargetHeight,
			   pNewJpeg, nMaxBuf, nCrop, dWidenRatio);
    if (nJpegSize == 0) {
	debug_log_output("cannot resize\n");
	free(pNewJpeg);
	return 0;
    }

    http_send_ok_header(fd, nJpegSize, "image/jpeg");
    send(fd, pNewJpeg, nJpegSize, 0);

    free(pNewJpeg);
    return 1; // Success
}

#endif // RESIZE_JPEG
