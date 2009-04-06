#ifndef _WIZD_AVIHEADERS_H_
#define _WIZD_AVIHEADERS_H_

// XXX: ... I know, I know. it's a very ad-hoc way...
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned long FOURCC;
typedef unsigned long long RECT;

typedef struct {
	DWORD   biSize;
	DWORD   biWidth;
	DWORD   biHeight;
	WORD    biPlanes;
	WORD    biBitCount;
	DWORD   biCompression;
	DWORD   biSizeImage;
	DWORD   biXPelsPerMeter;
	DWORD   biYPelsPerMeter;
	DWORD   biClrUsed;
	DWORD   biClrImportant;
} BITMAPINFOHEADER;

typedef struct {
	WORD wFormatTag;
	WORD nChannels;
	DWORD nSamplesPerSec;
	DWORD nAvgBytesPerSec;
	WORD nBlockAlign;
	WORD wBitsPerSample;
	WORD cbSize;
} WAVEFORMATEX;

typedef struct  {
	DWORD  dwMicroSecPerFrame;
	DWORD  dwMaxBytesPerSec;
	DWORD  dwReserved1;
	DWORD  dwFlags;
	DWORD  dwTotalFrames;
	DWORD  dwInitialFrames;
	DWORD  dwStreams;
	DWORD  dwSuggestedBufferSize;
	DWORD  dwWidth;
	DWORD  dwHeight;
	DWORD  dwScale;
	DWORD  dwRate;
	DWORD  dwStart;
	DWORD  dwLength;
} MainAVIHeader;

typedef struct {
	FOURCC  fccType;
	FOURCC  fccHandler;
	DWORD   dwFlags;
	DWORD   dwReserved1;
	DWORD   dwInitialFrames;
	DWORD   dwScale;
	DWORD   dwRate;
	DWORD   dwStart;
	DWORD   dwLength;
	DWORD   dwSuggestedBufferSize;
	DWORD   dwQuality;
	DWORD   dwSampleSize;
	RECT	rcFrame;
} AVIStreamHeader;

#define SYM_RIFF 0x46464952 //'RIFF'
#define SYM_AVI  0x20495641 //'AVI '
#define SYM_LIST 0x5453494C //'LIST'
#define SYM_HDRL 0x6C726468 //'hrdl'
#define SYM_AVIH 0x68697661 //'avih'
#define SYM_STRL 0x6C727473 //'strl'
#define SYM_STRH 0x68727473 //'strh'
#define SYM_STRF 0x66727473 //'strf'
#define SYM_STRD 0x64727473 //'strd'
#define SYM_VIDS 0x73646976 //'vids'
#define SYM_AUDS 0x73647561 //'auds'
#define SYM_IDX1 0x31786469 //'idx1'
#define SYM_MOVI 0x69766f6d //'movi'

const char *str_acodec(int wFormatTag);
char *str_fourcc(FOURCC fcc);
int read_next_chunk(FILE *fp, FOURCC *fcc);
DWORD read_next_sym(FILE *fp);
int read_avi_main_header(FILE *fp, MainAVIHeader *avih);
int read_avi_stream_header(FILE *fp, AVIStreamHeader *avish, int len);
char *format_time(time_t t);

#endif /* _WIZD_AVIHEADERS_H_ */
