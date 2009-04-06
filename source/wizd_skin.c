#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "wizd.h"
#include "wizd_skin.h"

static unsigned char	skin_path[WIZD_FILENAME_MAX];

// prototype
static unsigned char *skin_file_read(unsigned char *read_filename, unsigned long *malloc_size);

SKIN_T *skin_open(char *filename)
{
	char read_filename[WIZD_FILENAME_MAX];
	SKIN_T *skin;

	skin = malloc(sizeof(SKIN_T));
	if (skin == NULL) {
		debug_log_output("open_skin: malloc error");
		return NULL;
	}

	snprintf(read_filename, sizeof(read_filename), "%s/%s/%s", global_param.skin_root, global_param.skin_name, filename);
	skin->buffer = skin_file_read(read_filename, &skin->buffer_size);
	if (skin->buffer == NULL) {
		debug_log_output("open_skin: skin_file_read() error");
		free(skin);
		return NULL;
	}

	return skin;
}

void skin_close(SKIN_T *skin)
{
	if (skin != NULL) {
		if (skin->buffer != NULL) free(skin->buffer);
		free(skin);
	}
}

void skin_read_config(char *filename)
{
	char read_filename[WIZD_FILENAME_MAX];

	snprintf(skin_path, sizeof(skin_path), "%s/%s"
		, global_param.skin_root
		, global_param.skin_name
	);
	snprintf(read_filename, sizeof(read_filename), "%s/%s", skin_path, filename);

	skin_config_file_read(read_filename);
}

unsigned char* skin_get_string(SKIN_T *skin)
{
	return skin->buffer;
}

void skin_direct_replace_string(SKIN_T *skin, unsigned char *orig, unsigned char *str)
{
	replase_character(skin->buffer, skin->buffer_size, orig, str);
}

void skin_direct_replace_format(SKIN_T *skin, unsigned char *orig, unsigned char *fmt, ...)
{
	va_list arg;
	unsigned char work_buf[1024]; // is it enough??

	va_start(arg, fmt);
	vsnprintf(work_buf, sizeof(work_buf), fmt, arg);
	va_end(arg);

	skin_direct_replace_string(skin, orig, work_buf);
}

void skin_direct_replace_global(SKIN_T *skin, SKIN_REPLASE_GLOBAL_DATA_T *rep_p)
{
	replase_skin_grobal_data(skin->buffer, skin->buffer_size, rep_p);
}

void skin_direct_cut_enclosed_words(SKIN_T *skin, unsigned char *s, unsigned char *e)
{
	cut_enclose_words(skin->buffer, skin->buffer_size, s, e);
}

int skin_direct_send(int fd, SKIN_T *skin)
{
	return send(fd, skin->buffer, strlen(skin->buffer), 0);
}

// **************************************************************************
// スキンファイルを読み込む。
//
//  ファイルサイズにあわせて、malloc()
//  文字コード変換。
// **************************************************************************
static unsigned char *skin_file_read(unsigned char *read_filename, unsigned long *malloc_size )
{
	int		fd;
	struct stat		file_stat;
	int				result;
	ssize_t			read_size;
	unsigned char 	*read_work_buf;
	unsigned char 	*read_buf;



	// ファイルサイズチェック
	result = stat(read_filename, &file_stat);
	debug_log_output("skin: head stat(%s)=%d, st_size=%lld", read_filename, result, file_stat.st_size );
	if ( result != 0 )
	{
		debug_log_output("stat(%s) error.", read_filename);
		return NULL;
	}

	// ファイルサイズの6倍の領域をmalloc(２つ)
	*malloc_size = file_stat.st_size * 6 ;

	read_work_buf = malloc( *malloc_size );
	if ( read_work_buf == NULL )
	{
		debug_log_output("maloc() error.");
		return NULL;
	}

	read_buf = malloc( *malloc_size );
	if ( read_buf == NULL )
	{
		debug_log_output("maloc() error.");
		return NULL;
	}

	memset(read_work_buf, '\0', *malloc_size);
	memset(read_buf, '\0', *malloc_size);

	// ファイル読み込み
	fd = open(read_filename, O_RDONLY );
	if ( fd < 0 )
	{
		debug_log_output("open(%s) error.",read_filename);
		free(read_buf);
		free(read_work_buf);
		return NULL;
	}

	read_size = read(fd, read_work_buf, file_stat.st_size );
	debug_log_output("skin: read() read_size=%d", read_size );

	if ( read_size != file_stat.st_size )
	{
		debug_log_output("read() error.");
		free(read_buf);
		free(read_work_buf);
		return NULL;
	}

	close( fd );

	// 読んだデータの文字コードを、MediaWiz用コードに変換
	convert_language_code(read_work_buf, read_buf, *malloc_size, CODE_AUTO, global_param.client_language_code);

	debug_log_output("skin: nkf end." );

	// ワークエリアをfree.
	free(read_work_buf);


	return read_buf;	// 正常終了
}




// **************************************************************************
// 全体用データ まとめて置換
// **************************************************************************
void replase_skin_grobal_data(unsigned char *menu_work_p, int menu_work_buf_size, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p)
{
	unsigned char	current_date[32];	// 現在の日付表示用
	unsigned char	current_time[32];	// 現在の時刻表示用

	// ===============================
	// = 削除キーワードを削除する
	// ===============================
#define DELETE(a) cut_enclose_words(menu_work_p, menu_work_buf_size,\
			SKIN_KEYWORD_DEL_##a, SKIN_KEYWORD_DEL_##a##_E)

	// ルートパスの場合。
	if ( strcmp(skin_rep_data_global_p->current_path_name, "/" ) == 0 ) {
		DELETE(IS_ROOTDIR);
	}

	// １ページの場合
	if ( strcmp(skin_rep_data_global_p->now_page_str, "1" ) == 0 ) {
		DELETE(IS_NO_PAGE_PREV);
		DELETE(IS_NO_PAGE_PREV2);
	} else {
		DELETE(IS_PAGE_PREV);
	}

	// 最後のページの場合
	if ( strcmp(skin_rep_data_global_p->now_page_str, skin_rep_data_global_p->max_page_str) == 0 ) {
		DELETE(IS_NO_PAGE_NEXT);
		DELETE(IS_NO_PAGE_NEXT2);
	} else {
		DELETE(IS_PAGE_NEXT);
	}

	// 再生可能ファイル数が０の場合
	if ( skin_rep_data_global_p->stream_files == 0 ) {
		DELETE(IS_NO_STREAM_FILES);
		DELETE(IS_NO_STREAM_FILES2);
	} else {
		DELETE(IS_STREAM_FILES);
	}
	if ( skin_rep_data_global_p->music_files == 0 ) {
		DELETE(IS_NO_MUSIC_FILES);
		DELETE(IS_NO_MUSIC_FILES2);
	} else {
		DELETE(IS_MUSIC_FILES);
	}
	if ( skin_rep_data_global_p->photo_files == 0 ) {
		DELETE(IS_NO_PHOTO_FILES);
		DELETE(IS_NO_PHOTO_FILES2);
	} else {
		DELETE(IS_PHOTO_FILES);
	}

	// クライアントがPCのときとそうじゃないとき
	if ( skin_rep_data_global_p->flag_pc == 1 ) {
		DELETE(IF_CLIENT_IS_PC);
	} else {
		DELETE(IF_CLIENT_IS_NOT_PC);
	}

	// ?focus が指定されているときとそうじゃないとき
	if ( skin_rep_data_global_p->focus[0] == '\0' ) {
		// focus が指定されていないときは 削除
		DELETE(IF_FOCUS_IS_NOT_SPECIFIED);
	} else {
		// focus が指定されているときは 削除
		DELETE(IF_FOCUS_IS_SPECIFIED);
	}
#undef DELETE
	// =============
	// = 置換実行
	// =============

#define REPLACE(a,b) replase_character(menu_work_p, menu_work_buf_size,\
			SKIN_KEYWORD_##a, (b))
#define REPLACE_G(a,b) replase_character(menu_work_p, menu_work_buf_size,\
			SKIN_KEYWORD_##a, skin_rep_data_global_p->b)

	conv_time_to_date_string(current_date, time(NULL));
	conv_time_to_time_string(current_time, time(NULL));

	// SERVER_NAME
	REPLACE(SERVER_NAME, SERVER_NAME);
	REPLACE(CURRENT_DATE, current_date);
	REPLACE(CURRENT_TIME, current_time);

	if (global_param.flag_show_audio_info != 0)
		REPLACE(DVD_OPTIONS, "showaudio");
	else if (global_param.flag_split_vob_chapters == 1)
		REPLACE(DVD_OPTIONS, "splitchapters");
	else if (global_param.flag_split_vob_chapters == 2)
		REPLACE(DVD_OPTIONS, "notitlesplit");
	else
		REPLACE(DVD_OPTIONS, "none");

	REPLACE(CLIENT_CHARSET, global_param.client_language_code == CODE_SJIS ? "Shift_JIS" : "euc-jp");

	REPLACE_G(SERVER_ADDRESS, recv_host);
	REPLACE_G(CURRENT_PATH, current_path_name);
	REPLACE_G(CURRENT_DIR_NAME, current_directory_name);
	REPLACE_G(CURRENT_PATH_LINK, current_directory_link);
	REPLACE_G(CURRENT_PATH_FULL_LINK, current_directory_absolute);
	REPLACE_G(CURRENT_PATH_LINK_NO_PARAM, current_directory_link_no_param);
	REPLACE_G(PARLENT_DIR_LINK, parent_directory_link );
	REPLACE_G(CURRENT_PAGE, now_page_str);
	REPLACE_G(MAX_PAGE, max_page_str);
	REPLACE_G(NEXT_PAGE, next_page_str);
	REPLACE_G(PREV_PAGE, prev_page_str);
	REPLACE_G(FILE_NUM, file_num_str);
	REPLACE_G(START_FILE_NUM, start_file_num_str);
	REPLACE_G(END_FILE_NUM, end_file_num_str);
	REPLACE_G(ONLOADSET_FOCUS, focus);
	REPLACE_G(SECRET_DIR_LINK, secret_dir_link_html);

	REPLACE_G(DEFAULT_PHOTOLIST, default_photolist);
	REPLACE_G(DEFAULT_MUSICLIST, default_musiclist);

#undef REPLACE
#undef REPLACE_G

	return;
}

// **************************************************************************
// ライン用データ まとめて置換
// **************************************************************************
void replase_skin_line_data(unsigned char *menu_work_p, int menu_work_buf_size, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p)
{
	unsigned char	row_string[16];	// 行番号

	// ===============================
	// = 削除キーワードを削除する
	// ===============================
#define DELETE(a) cut_enclose_words(menu_work_p, menu_work_buf_size,\
			SKIN_KEYWORD_DEL_##a, SKIN_KEYWORD_DEL_##a##_E)

	if ( skin_rep_data_line_p->mp3_id3v1_flag == 0 ) {
		DELETE(IS_NO_MP3_TAGS);		// MP3タグ無い場合
	} else {
		DELETE(IS_HAVE_MP3_TAGS);	// MP3タグ存在してる場合
	}

	if ( skin_rep_data_line_p->row_num % 2 == 0 ) {
		DELETE(IF_LINE_IS_EVEN);	// 偶数行
	} else {
		DELETE(IF_LINE_IS_ODD);		// 奇数行
	}
#undef DELETE

	// =============
	//  置換実行
	// =============

#define REPLACE(a,b) replase_character(menu_work_p, menu_work_buf_size,\
			SKIN_KEYWORD_LINE_##a, (b))
#define REPLACE_L(a,b) replase_character(menu_work_p, menu_work_buf_size,\
			SKIN_KEYWORD_LINE_##a, skin_rep_data_line_p->b)

	snprintf(row_string, sizeof(row_string), "%d"
		, skin_rep_data_line_p->row_num );

	REPLACE(COLUMN_NUM, row_string);	// oops. backward compatibility :(
	REPLACE(ROW_NUM, row_string);		// 行番号

	REPLACE_L(FILE_NAME, file_name);
	REPLACE_L(FILE_NAME_NO_EXT, file_name_no_ext);
	REPLACE_L(FILE_EXT, file_extension);

	REPLACE_L(FILE_LINK, file_uri_link);
	REPLACE_L(CHAPTER_LINK, chapter_link);
	REPLACE_L(CHAPTER_STR, chapter_str);
	REPLACE_L(TIMESTAMP, file_timestamp);
	REPLACE_L(FILE_DATE, file_timestamp_date);
	REPLACE_L(FILE_TIME, file_timestamp_time);
	REPLACE_L(FILE_DURATION, file_duration);
	REPLACE_L(TVID, tvid_string);

	REPLACE_L(FILE_VOD, vod_string);
	REPLACE_L(FILE_SIZE, file_size_string);

	REPLACE_L(SVI_INFO, svi_info_data);
	REPLACE_L(SVI_REC_TIME, svi_rec_time_data);

	REPLACE_L(IMAGE_WIDTH, image_width);
	REPLACE_L(IMAGE_HEIGHT, image_height);

	REPLACE_L(MP3TAG_TITLE, mp3_id3v1_title);
	REPLACE_L(MP3TAG_ALBUM, mp3_id3v1_album);
	REPLACE_L(MP3TAG_ARTIST, mp3_id3v1_artist);
	REPLACE_L(MP3TAG_YEAR, mp3_id3v1_year);
	REPLACE_L(MP3TAG_COMMENT,mp3_id3v1_comment);
	REPLACE_L(MP3TAG_TITLE_INFO, mp3_id3v1_title_info_limited);

	REPLACE_L(AVI_FPS, avi_fps);
	REPLACE_L(AVI_DURATION, avi_duration);
	REPLACE_L(AVI_VCODEC, avi_vcodec);
	REPLACE_L(AVI_ACODEC, avi_acodec);
	REPLACE_L(AVI_HVCODEC, avi_hvcodec);
	REPLACE_L(AVI_HACODEC, avi_hacodec);
	REPLACE_L(AVI_IS_INTERLEAVED, avi_is_interleaved);

#undef REPLACE
#undef REPLACE_L
	return;
}

void skin_direct_replace_image_viewer(SKIN_T *skin, SKIN_REPLASE_IMAGE_VIEWER_DATA_T *image_viewer_info_p)
{
	skin_direct_replace_string(skin, SKIN_KEYWORD_SERVER_NAME, SERVER_NAME);

#define REPLACE_I(a, b) skin_direct_replace_string(skin, SKIN_KEYWORD_##a,\
			image_viewer_info_p->b)
	REPLACE_I(CURRENT_PATH, current_uri_name);
	REPLACE_I(CURRENT_PATH_LINK, current_uri_link);
	REPLACE_I(PARLENT_DIR_LINK, parent_directory_link );
	REPLACE_I(CURRENT_PAGE, now_page_str);
	REPLACE_I(LINE_TIMESTAMP, file_timestamp );
	REPLACE_I(LINE_FILE_DATE, file_timestamp_date );
	REPLACE_I(LINE_FILE_TIME, file_timestamp_time );
	REPLACE_I(LINE_FILE_SIZE, file_size_string );
	REPLACE_I(LINE_IMAGE_WIDTH, image_width );
	REPLACE_I(LINE_IMAGE_HEIGHT, image_height );
	REPLACE_I(IMAGE_VIEWER_WIDTH, image_viewer_width );
	REPLACE_I(IMAGE_VIEWER_HEIGHT, image_viewer_height );
	REPLACE_I(IMAGE_VIEWER_MODE, image_viewer_mode );
#undef REPLACE_I
}
