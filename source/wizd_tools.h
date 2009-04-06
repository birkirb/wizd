// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_tools.h
//											$Revision: 1.8 $
//											$Date: 2004/10/11 05:37:52 $
//
//	���٤Ƽ�����Ǥ�Ǥ��ʤ������ޤġ�
//  ���Υ��եȤˤĤ���VertexLink���䤤��碌�ʤ��Ǥ���������
// ==========================================================================
#ifndef	_WIZD_TOOLS_H
#define	_WIZD_TOOLS_H


#define			CR		(0x0D)
#define			LF		(0x0A)

#define		OUTPUT_LOG_ROTATE_SIZE		(1024*1024*1900)	/* 1.9Gbyte */
#define		OUTPUT_LOG_ROTATE_MAX		5					/* �����rotate������� */

// uri���󥳡��ɡ��ǥ�����
extern int 		uri_encode(unsigned char *dst,  unsigned int dst_len, const unsigned char *src, int src_len);
extern int 		uri_decode(unsigned char *dst, unsigned int dst_len, const unsigned char *src, unsigned int src_len);

// �ƥ����Ƚ���������
extern void 	cut_after_character(unsigned char *sentence, unsigned char cut_char);
extern void 	cut_before_character(unsigned char *sentence, unsigned char cut_char);
extern void 	cut_character(unsigned char *sentence, unsigned char cut_char);
extern void 	cut_first_character(unsigned char *sentence, unsigned char cut_char);
extern void 	cut_before_last_character(unsigned char *sentence, unsigned char cut_char);
extern void 	cut_after_last_character(unsigned char *sentence, unsigned char cut_char);
extern int 		sentence_split(unsigned char *sentence, unsigned char cut_char, unsigned char *split1, unsigned char *split2);
extern void 	duplex_character_to_unique(unsigned char *sentence, unsigned char unique_char);
extern void 	replase_character(unsigned char *sentence, int sentence_buf_size, const unsigned char *key, const unsigned char *rep);
extern void 	replase_character_first(unsigned char *sentence, int sentence_buf_size, const unsigned char *key, const unsigned char *rep);
extern void 	make_datetime_string(unsigned char *sentence);
extern void 	conv_time_to_string(unsigned char *sentence, time_t conv_time);
extern void 	conv_time_to_date_string(unsigned char *sentence, time_t conv_time);
extern void 	conv_time_to_time_string(unsigned char *sentence, time_t conv_time);
extern void 	conv_num_to_unit_string(unsigned char *sentence, u_int64_t file_size);
extern void 	cat_before_n_length(unsigned char *sentence,  unsigned int n);
extern void 	cat_after_n_length(unsigned char *sentence,  unsigned int n);
extern void 	cut_character_at_linetail(char *sentence, char cut_char);
extern void 	filename_to_extension(unsigned char *filename, unsigned char *extension_buf, int extension_buf_size);
extern void 	han2euczen(unsigned char *src, unsigned char *dist, int dist_size);
extern void 	euc_string_cut_n_length(unsigned char *euc_sentence,  unsigned int n);
extern void 	cut_enclose_words(unsigned char *sentence, int sentence_buf_size, unsigned char *start_key, unsigned char *end_key);
extern void 	decode_samba_hex_and_cap_coding( unsigned char *sentence );
extern void 	sjis_code_thrust_replase(unsigned char *sentence, const unsigned char code );

extern unsigned char 	*buffer_distill_line(unsigned char *text_buf_p, unsigned char *line_buf_p, unsigned int line_buf_size );

extern void extension_add_rename(unsigned char *rename_filename_p, size_t rename_filename_size);
extern void extension_del_rename(unsigned char *rename_filename_p, size_t rename_filename_size);

extern char *my_strcasestr(const char *p1, const char *p2);

// path ���� ./ �� ../ ���������dir �˷�̤���Ǽ����롣
extern char *path_sanitize(char *dir, size_t dir_size);

// DebugLog ����

extern void debug_log_initialize(const unsigned char *set_debug_log_filename);
extern void debug_log_output(char *fmt, ...);

// ������
extern void jpeg_size(unsigned char *jpeg_filename, unsigned int *x, unsigned int *y);
extern void gif_size(unsigned char *gif_filename, 	unsigned int *x, unsigned int *y);
extern void png_size(unsigned char *png_filename, 	unsigned int *x, unsigned int *y);


extern void send_printf(int fd, char *fmt, ...);


#endif

