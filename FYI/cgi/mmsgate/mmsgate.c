#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <dirent.h>

void debug_log_output(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static char *random_guid()
{
	static char guid[40]; // 32+4+1+2];
	char *p;
	int i;

	p = guid;
	*p++ = '{';
	for (i=0; i<32; i++) {
		if (i == 8 || i == 12 || i == 16 || i == 20) *p++ = '-';
		*p++ = "0123456789ABCDEF"[rand() % 16];
	}
	*p++ = '}';
	*p++ = '\0';

	return guid;
}

/* ソケットを作成し、相手に接続するラッパ. 失敗 = -1 */
static int sock_connect(char *host, int port)
{
	int sock;
	struct sockaddr_in sockadd;
	struct hostent *hent;

	debug_log_output("sock_connect: %s:%d\n", host, port);
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) return -1;
	debug_log_output("sock: %d\n", sock);

	if (NULL == (hent = gethostbyname(host))) {
		close(sock);
		return -1;
	}
	debug_log_output("hent: %p\n", hent);
	bzero((char *)&sockadd, sizeof(sockadd));
	memcpy(&sockadd.sin_addr, hent->h_addr, hent->h_length);
	sockadd.sin_port = htons(port);
	sockadd.sin_family = AF_INET;
	if (connect(sock, (struct sockaddr*)&sockadd, sizeof(sockadd)) != 0) {
		debug_log_output("connect: %s  at %s:%d\n", strerror(errno), host, port);
		close(sock);
		return -1;
	}

	return sock;
}

typedef struct _mms_header {
	unsigned short type;
	unsigned short length;
	unsigned long seq_no;
	char unknown[2];
	unsigned short length2;
} MMS_HEADER;

#define CHUNK_HEAD ('$'*0x100 + 'H') // $H
#define CHUNK_DATA ('$'*0x100 + 'D') // $D
#define CHUNK_END  ('$'*0x100 + 'E') // $E

unsigned char HDR_ID[16] = {0xa1,0xdc,0xab,0x8c,0x47,0xa9,0xcf,0x11,
                            0x8e,0xe4,0x00,0xc0,0x0c,0x20,0x53,0x65};

char *find_hdr(char *buf, int len)
{
	int i;
	for (i=0; i<len - sizeof(HDR_ID); i++) {
		if (!memcmp(buf+i, HDR_ID, sizeof(HDR_ID))) return buf+i;
	}
	return NULL;
}


main()
{
	int sock;
	char *p_target_host_name, *p_uri_string;
	char uri_string[2048];
	char send_http_header_buf[2048];
	char *p_url, *p;
	char *p_auth = NULL;
	char *guid;
	int port = 80;
	unsigned long long content_length = 0;
	int len = 0;
	FILE *fp;

	MMS_HEADER hdr;
	char buff[1024*64];
	int i;
	unsigned long seq_offset = 0xffffffffUL;
	unsigned long t, start_t = 0xffffffffUL;
	unsigned long chunklen = 0;
	unsigned long outlen = 0;

	p_uri_string = getenv("QUERY_STRING");
	if (p_uri_string == NULL) return -1;

	strncpy(uri_string, p_uri_string, sizeof(uri_string));
	p_uri_string = uri_string;
	/* parse_url */
	// skip 'mms://'
	p_target_host_name = p_uri_string + 6;

	//strncpy(base_url, p_uri_string, 2048);
	//p = strrchr(base_url, '/');
	//p[1] = '\0';

	p_url = strchr(p_target_host_name, '/');
	if (p_url == NULL) return -1;
	*p_url++ = '\0';

	p = strchr(p_target_host_name, '@');
	if (p != NULL) {
		// there is a user name.
		p_auth = p_target_host_name;
		p_target_host_name = p + 1;
		*p = '\0';
	}
	p = strchr(p_target_host_name, ':');
	if (p != NULL) {
		port = atoi(p+1);
		*p = '\0';
	}

	debug_log_output("proxy:target_host_name: %s\n", p_target_host_name);
	debug_log_output("proxy:authenticate: %s\n", p_auth ? p_auth : "NULL");
	debug_log_output("proxy:url: %s\n", p_url);
	debug_log_output("proxy:port: %d\n", port);

	sock = sock_connect(p_target_host_name, port);
	if (sock < 0) {
		debug_log_output("error: %s\n", strerror(errno));
		debug_log_output("sock: %d\n", sock);
		return -1;
	}
	fp = fdopen(sock, "r+");
	if (fp == NULL) {
		debug_log_output("fdopen error: %s\n", strerror(errno));
		return -1;
	}

	fprintf(fp, "GET /%s HTTP/1.0\r\n", p_url);
	fprintf(fp, "Host: %s:%u\r\n", p_target_host_name, port);
	fprintf(fp, "User-Agent: NSPlayer/9.0.0.2980\r\n");
	fprintf(fp, "Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=0:0,request-context=2,max-duration=0\r\n");
	fprintf(fp, "Pragma: xPlayStrm=1\r\n");
	guid = random_guid();
	fprintf(fp, "Pragma: xClientGUID=%s\r\n", guid);
	fprintf(fp, "Pragma: stream-switch-count=2\r\n");
	fprintf(fp, "Pragma: stream-switch-entry=ffff:1:0 ffff:2:0\r\n");
	fprintf(fp, "Accept: */*\r\n");
	fprintf(fp, "Connection: close\r\n");
	fprintf(fp, "\r\n");

	while (fgets(buff, sizeof(buff), fp) != NULL) {
		debug_log_output("recv: %s", buff);
		//if (!strncmp(buff, "Content-Type:", 13)) {
		//	snprintf(buff, sizeof(buff), "Content-Type: audio/x-ms-wma\r\n");
		//}
		//fprintf(stdout, "%s", buff);
		//fflush(stdout);
		if (!strncmp(buff, "\r\n", 2)) break;
	}

	// この場合のヘッダは、結局自作とオリジナルデータ、どっちがいいんだろう。
	// 面倒くなって テストしてないんでわかりません。
	// たぶん オリジナルに戻しても動くとは思うけど

	fprintf(stdout, "HTTP/1.0 200 OK\r\n");
	fprintf(stdout, "Connection: Close\r\n");
	fprintf(stdout, "Content-Type: audio/x-ms-wma\r\n");
	fprintf(stdout, "\r\n");
	fflush(stdout);

	debug_log_output("header received.\n");

	while (fread(&hdr, 1, sizeof(MMS_HEADER), fp) > 0) {
		// byte order change.
		hdr.type = htons(hdr.type);

		//fprintf(stderr, "CHUNK '%.2s', len %lu, seq %lu\n", (char*)&hdr.type, hdr.length, hdr.seq_no);
		len = hdr.length +4 - sizeof(MMS_HEADER);
		fprintf(stderr, "%08X: CHUNK '%.2s', len %lu, seq %lu\n", outlen, (char*)&hdr.type, len, hdr.seq_no);
//		printf("read: %d\n", len);
		fread(buff, 1, len, fp);

		if (hdr.type == CHUNK_DATA || (hdr.type & 0xff) == 'D') {
			//if (seq_offset == 0xffffffffUL) seq_offset = hdr.seq_no;
			//fwrite(buff, 1, len, stdout);
			int offset = 5;
			offset += (buff[3] & 0x08 ? 1 : 0);
			offset += (buff[3] & 0x10 ? 2 : 0) ;
			offset += (buff[3] & 0x40 ? 2 : 0) ;
			t = *(unsigned long*)(buff + offset);
			if (start_t == 0xffffffffUL) {
				start_t = t;
				fprintf(stderr, "start_t: %08X, ", t);
			}
			if (start_t <= t) {
				*(unsigned long*)(buff + offset) -= start_t;
			}
			fprintf(stderr, "t: %08X, fixed: %08X\n", t, *(unsigned long*)(buff + offset));

			if (buff[3] & 0x01) {
				for (i=0; i < (buff[offset+6] & 0x1f); i++) {
				}
			}


			outlen += fwrite(buff, 1, len, stdout);
			while (len++ < chunklen) outlen ++,fputc(0, stdout);
		} else if (hdr.type == CHUNK_HEAD) {
			char *ptr;
			if ((ptr = find_hdr(buff, len)) != NULL) {
				chunklen = *(unsigned long*)(ptr+0x5c);
				fprintf(stderr, "chunklen: %d\n", chunklen);
			}
			outlen += fwrite(buff, 1, len, stdout);
		} else {
			fprintf(stderr, "unknown chunk!\n");
			//outlen += fwrite(buff, 1, len, stdout);
		}
		fflush(stdout);
	}
}
