#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "wizd.h"
#include "wizd_aviread.h"

const char *str_acodec(int wFormatTag) {
	static char buf[10];
	struct _audio_format_table {
		WORD wFormatTag;
		char *name;
	} tbl[] = {
		{ 0x0000, "null" }, // unknown.
		{ 0x0001, "PCM" },
		{ 0x0050, "MPEG" }, // mpeg layer 2 ?
		{ 0x0055, "MP3" },
		{ 0x0092, "AC3" }, // Dolby AC-3

		// the next information from MSDN web site. I dont know what it is. :p
		{ 0x0161, "WMA" }, // Windows Media Audio.
		{ 0x0162, "WMA9_PRO" }, // Windows Media Audio 9 Professional
		{ 0x0163, "WMA9_LL" }, // Windows Media Audio 9 Lossless

		{ 0x4143, "MP4A" }, // MPEG-4 Audio

		{ 0, NULL } // terminator
	};
	int i;

	for (i=0; tbl[i].name != NULL; i++) {
		if (tbl[i].wFormatTag == wFormatTag) {
			return tbl[i].name;
		}
	}

	sprintf(buf, "0x%04X", wFormatTag);

	return buf;
}

char *str_fourcc(FOURCC f) {
	static char buf[5];

	buf[0] = f & 0xff;
	buf[1] = (f >> 8) & 0xff;
	buf[2] = (f >> 16) & 0xff;
	buf[3] = (f >> 24) & 0xff;
	buf[4] = '\0';

	return buf;
}

unsigned long get_le_word(unsigned char *buf)
{
	return buf[1] * 0x100 + buf[0];
}

unsigned long get_le_dword(unsigned char *buf)
{
	return buf[3] * 0x1000000 + buf[2] * 0x10000 + buf[1] * 0x100 + buf[0];
}

unsigned long get_be_dword(unsigned char *buf)
{
	return buf[0] * 0x1000000 + buf[1] * 0x10000 + buf[2] * 0x100 + buf[3];
}

unsigned long avi_len(unsigned char *buf)
{
	return get_le_dword(buf);
}

int read_next_chunk(FILE *fp, FOURCC *fcc)
{
	unsigned char buf[8];
	int len;

	if (fread(buf, sizeof(buf), 1, fp) == 0) return -1;

	*fcc = get_le_dword(buf);
	len = avi_len(buf + 4);
	len = (len + 1) / 2 * 2;

	return len;
}

DWORD read_next_sym(FILE *fp)
{
	unsigned char buf[4];

	if (fread(buf, sizeof(buf), 1, fp) != 1) return -1;
	return get_le_dword(buf);
}

void set_avih(MainAVIHeader *avih, char *ptr)
{
	DWORD *p = (DWORD*)ptr;
	avih->dwMicroSecPerFrame    = get_le_dword((char*)(p++));
	avih->dwMaxBytesPerSec      = get_le_dword((char*)(p++));
	avih->dwReserved1           = get_le_dword((char*)(p++));
	avih->dwFlags               = get_le_dword((char*)(p++));
	avih->dwTotalFrames         = get_le_dword((char*)(p++));
	avih->dwInitialFrames       = get_le_dword((char*)(p++));
	avih->dwStreams             = get_le_dword((char*)(p++));
	avih->dwSuggestedBufferSize = get_le_dword((char*)(p++));
	avih->dwWidth               = get_le_dword((char*)(p++));
	avih->dwHeight              = get_le_dword((char*)(p++));
	avih->dwScale               = get_le_dword((char*)(p++));
	avih->dwRate                = get_le_dword((char*)(p++));
	avih->dwStart               = get_le_dword((char*)(p++));
	avih->dwLength              = get_le_dword((char*)(p++));
}

void set_avish(AVIStreamHeader *avish, char *ptr)
{
	DWORD *p = (DWORD*)ptr;
	avish->fccType               = get_le_dword((char*)(p++));
	avish->fccHandler            = get_le_dword((char*)(p++));
	avish->dwFlags               = get_le_dword((char*)(p++));
	avish->dwReserved1           = get_le_dword((char*)(p++));
	avish->dwInitialFrames       = get_le_dword((char*)(p++));
	avish->dwScale               = get_le_dword((char*)(p++));
	avish->dwRate                = get_le_dword((char*)(p++));
	avish->dwStart               = get_le_dword((char*)(p++));
	avish->dwLength              = get_le_dword((char*)(p++));
	avish->dwSuggestedBufferSize = get_le_dword((char*)(p++));
	avish->dwQuality             = get_le_dword((char*)(p++));
	avish->dwSampleSize          = get_le_dword((char*)(p++));
	//avish->rcFrame = get_le_dword((char*)(p++));
}

int read_avi_main_header(FILE *fp, MainAVIHeader *avih)
{
	unsigned long len, listlen;
	DWORD riff_type;
	FOURCC fcc;

	// we allows RIFF[0000]AVI...
	if ((len = read_next_chunk(fp, &fcc)) < 0) return -1;
	//printf("debug: %d\n", len);
	if (fcc != SYM_RIFF) return -1;

	riff_type = read_next_sym(fp);
	if (riff_type == -1 || riff_type != SYM_AVI) return -1;


	if ((listlen = read_next_chunk(fp, &fcc)) <= 0) return -1;
	if (fcc != SYM_LIST) return -1;
	//printf("debug: listlen %d\n", listlen);

	if ((riff_type = read_next_sym(fp)) == -1) return -1;
	if (riff_type != SYM_HDRL) return -1;


	while (listlen > 0) {
		if ((len = read_next_chunk(fp, &fcc)) <= 0) return -1;
		if (fcc == SYM_AVIH) {
			char *ptr = malloc(len);
			int ret = -1;
			if (fread(ptr, len, 1, fp) == 1) {
				set_avih(avih, ptr);
				ret = 0;
			}
			free(ptr);
			return ret;
		}
		fseek(fp, len, SEEK_CUR);
		listlen -= len;
	}

	return -1;
}


int read_avi_stream_header(FILE *fp, AVIStreamHeader *avish, int len)
{
	DWORD riff_type;
	FOURCC fcc;
	int chunklen;

	if ((riff_type = read_next_sym(fp)) == -1) return -1;
	if (riff_type != SYM_STRL) {
		fseek(fp, -4L, SEEK_CUR);
		return -1;
	}
	len -= sizeof(riff_type);

	while (len > 0 && (chunklen = read_next_chunk(fp, &fcc)) > 0) {
		char *ptr;

		//printf("chunklen: %d\n", chunklen);
		len -= chunklen + 8;
		switch (fcc) {
		case SYM_STRH:
			ptr = malloc(chunklen);
			fread(ptr, chunklen, 1, fp);
			set_avish(avish, ptr);
			free(ptr);
			break;
		case SYM_STRF:
			ptr = malloc(chunklen);
			fread(ptr, chunklen, 1, fp);
			if (avish->fccType == SYM_VIDS) {
				/* save biCompression into avish->dwReserved1 :p */
				BITMAPINFOHEADER *bi = (BITMAPINFOHEADER*)ptr;
				avish->dwReserved1 = get_le_dword((char*)&bi->biCompression);
			} else if (avish->fccType == SYM_AUDS) {
				/* save wFormatTag into avish->dwReserved1 :p */
				WAVEFORMATEX *wi = (WAVEFORMATEX*)ptr;
				avish->dwReserved1 = get_le_word((char*)&wi->wFormatTag);
			}
			free(ptr);
			break;
		default:
			fseek(fp, chunklen, SEEK_CUR);
			break;
		}
	}

	return 0;
}

char *format_time(time_t t)
{
	static char buf[10];
	snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", t/3600, (t/60)%60, t%60);
	return buf;
}
