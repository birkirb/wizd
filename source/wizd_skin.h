#ifndef _WIZD_SKIN_H_
#define _WIZD_SKIN_H_

#include "wizd.h"

#define MAX_SKIN_FILESIZE (1024*16)

// ****************
// SKIN_T
// ****************
typedef struct _skin_t {
	char *buffer;
	unsigned long buffer_size;
} SKIN_T;


typedef struct _skin_mapping {
	int filetype;
	char *skin_filename;
} SKIN_MAPPING_T;


// �������ִ��ѥǡ����ʥ����Х��
typedef struct  {
	unsigned char	current_path_name[WIZD_FILENAME_MAX];	// ���ѥ� ɽ����(ʸ��������Ĵ���Ѥ�)

	unsigned char	current_directory_name[WIZD_FILENAME_MAX];	// ���ǥ��쥯�ȥ� ɽ����(ʸ��������Ĵ���Ѥ�)
	unsigned char	current_directory_link[WIZD_FILENAME_MAX];	// ���ǥ��쥯�ȥ� Link�ѡ�URI���󥳡��ɺѤߡ�
	unsigned char	current_directory_link_no_param[WIZD_FILENAME_MAX];	// ���ǥ��쥯�ȥ� Link�ѡ�URI���󥳡��ɺѤߡ�

	unsigned char	parent_directory_name[WIZD_FILENAME_MAX];	// �ƥǥ��쥯�ȥ�ɽ����
	unsigned char	parent_directory_link[WIZD_FILENAME_MAX];	// �ƥǥ��쥯�ȥ�Link�ѡ�URI���󥳡��ɺѤߡ�

	unsigned char	file_num_str[16];	// ���ǥ��쥯�ȥ�Υե������ɽ����
	unsigned char	now_page_str[16];	// ���ߤΥڡ����ֹ�ɽ����
	unsigned char	max_page_str[16];	// ����ڡ����ֹ�ɽ����

	unsigned char	start_file_num_str[16];	// ���ߥڡ�����ɽ�����ϥե������ֹ�ɽ����
	unsigned char	end_file_num_str[16];	// ���ߥڡ�����ɽ�����ϥե������ֹ�ɽ����

	unsigned char	next_page_str[16];	// ���Υڡ���(̵�����max_page)ɽ����
	unsigned char	prev_page_str[16];	// ������Υڡ�����̵����� 1)ɽ����

	unsigned char	focus[64];			// BODY������ onloadset="$focus"

	int		stream_files;	// ������ǽ�ե������


	// �����ǥ��쥯�ȥ����
	unsigned char	secret_dir_link_html[512];

	// ���饤����Ȥ�PC���ɤ���
	int		flag_pc;

} SKIN_REPLASE_GLOBAL_DATA_T;


// �������ִ��ѥǡ������ʥե������
typedef struct  {
	int				stream_type;			// ���ȥ꡼��ե����뤫�ݤ�
	int				menu_file_type;			// �ե�����μ���

	unsigned char	file_name[255];			// �ե�����̾ɽ����(ʸ��������Ĵ���Ѥ�)
	unsigned char	file_name_no_ext[255];	// ��ĥ��̵���ե�����̾ɽ����(ʸ��������Ĵ���Ѥ�)
	unsigned char	file_extension[16];	// ��ĥ�ҤΤ�(ʸ��������Ĵ���Ѥ�)

	unsigned char	file_uri_link[WIZD_FILENAME_MAX];	// �ե�����ؤ�Link(URI���󥳡��ɺѤ�)

	unsigned char	file_timestamp[32];		// �����ॹ�����ɽ����
	unsigned char	file_timestamp_date[32];	// �����ॹ�����ɽ���� ���դΤ�
	unsigned char	file_timestamp_time[32];	// �����ॹ�����ɽ���� �����Τ�

	unsigned char	file_size_string[32];	// �ե����륵����ɽ����

	unsigned char	svi_info_data[SVI_INFO_LENGTH];	// SVI �ե��������
	unsigned char	svi_rec_time_data[32];			// SVIϿ�����

	unsigned char	tvid_string[16];	// TVIDɽ����
	unsigned char	vod_string[32];		// vod="0" or vod="playlist"  ɬ�פ˱������դ�
	int				row_num;		// ���ֹ�

	unsigned char	image_width[16];	// �����ǡ��� ����
	unsigned char	image_height[16];	// �����ǡ��� �⤵


	// MP3 ID3v1 ��������
	unsigned char	mp3_id3v1_flag;			// MP3 ���� ¸�ߥե饰
	unsigned char	mp3_id3v1_title[128];	// MP3 ��̾
	unsigned char	mp3_id3v1_album[128];	// MP3 ����Х�̾
	unsigned char	mp3_id3v1_artist[128];	// MP3 �����ƥ�����
	unsigned char	mp3_id3v1_year[128];		// MP3 ����ǯ��
	unsigned char	mp3_id3v1_comment[128];	// MP3 ������

	unsigned char	mp3_id3v1_title_info[128*4];			// MP3 ��̾[����Х�̾/�����ƥ�����] �ޤȤ��ɽ��
	unsigned char	mp3_id3v1_title_info_limited[128*4];	// MP3 ��̾[����Х�̾/�����ƥ�����] �ޤȤ��ɽ��(�������¤���)

	unsigned char	avi_fps[16];
	unsigned char	avi_duration[32];
	unsigned char	avi_vcodec[128];
	unsigned char	avi_acodec[128];
	unsigned char	avi_hvcodec[128];
	unsigned char	avi_hacodec[128];
	unsigned char	avi_is_interleaved[32];

} SKIN_REPLASE_LINE_DATA_T;



// ImageViewer �ִ��ѥǡ���
typedef struct  {

	unsigned char	current_uri_name[WIZD_FILENAME_MAX];	// ��URI ɽ����(ʸ��������Ĵ���Ѥ�)
	unsigned char	current_uri_link[WIZD_FILENAME_MAX];	// ��URI Link�ѡ�URI���󥳡��ɺѤߡ�

	unsigned char	parent_directory_link[WIZD_FILENAME_MAX];	// �ƥǥ��쥯�ȥ�Link�ѡ�URI���󥳡��ɺѤߡ�

	unsigned char	now_page_str[16];	// ���ߤΥڡ����ֹ�ɽ����

	unsigned char	file_timestamp[32];			// �����ॹ�����ɽ����(����)
	unsigned char	file_timestamp_date[32];	// �����ॹ�����ɽ����(���դΤ�)
	unsigned char	file_timestamp_time[32];		// �����ॹ�����ɽ����(����Τ�)
	unsigned char	file_size_string[32];		// �ե����륵����ɽ����

	unsigned char	image_width[16];			// �����ǡ��� ����
	unsigned char	image_height[16];			// �����ǡ��� �⤵

	unsigned char	image_viewer_width[16];		// �����ǡ��� ɽ������
	unsigned char	image_viewer_height[16];	// �����ǡ��� ɽ���⤵

	unsigned char	image_viewer_mode[16];		// ɽ���⡼��

} SKIN_REPLASE_IMAGE_VIEWER_DATA_T;

void replase_skin_grobal_data(unsigned char *menu_work_p, int menu_work_buf_size, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p);
void replase_skin_line_data(unsigned char *menu_work_p, int menu_work_buf_size, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p);

SKIN_T *skin_open(char *filename);
void skin_close(SKIN_T *skin);
void skin_read_config(char *filename);
unsigned char* skin_get_string(SKIN_T *skin);
void skin_direct_replace_string(SKIN_T *skin, unsigned char *orig, unsigned char *str);
void skin_direct_replace_format(SKIN_T *skin, unsigned char *orig, unsigned char *fmt, ...);
void skin_direct_replace_global(SKIN_T *skin, SKIN_REPLASE_GLOBAL_DATA_T *rep_p);
void skin_direct_cut_enclosed_words(SKIN_T *skin, unsigned char *s, unsigned char *e);
int skin_direct_send(int fd, SKIN_T *skin);
;
void replase_skin_grobal_data(unsigned char *menu_work_p, int menu_work_buf_size, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p);
void replase_skin_line_data(unsigned char *menu_work_p, int menu_work_buf_size, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p);

void skin_direct_replace_image_viewer(SKIN_T *skin, SKIN_REPLASE_IMAGE_VIEWER_DATA_T *image_viewer_info_p);

// --------------------------------------------------------------------------

#define		SKIN_MENU_CONF						"wizd_skin.conf"

#define		SKIN_MENU_HEAD_HTML					"head.html"
#define		SKIN_MENU_LINE_MOVIE_FILE_HTML		"line_movie.html"
#define		SKIN_MENU_LINE_MUSIC_FILE_HTML		"line_music.html"
#define		SKIN_MENU_LINE_IMAGE_FILE_HTML		"line_image.html"
#define		SKIN_MENU_LINE_DOCUMENT_FILE_HTML	"line_document.html"
#define		SKIN_MENU_LINE_UNKNOWN_FILE_HTML	"line_unknown.html"
#define		SKIN_MENU_LINE_SVI_FILE_HTML		"line_svi_file.html"
#define		SKIN_MENU_LINE_DIR_HTML				"line_dir.html"
#define		SKIN_MENU_LINE_PSEUDO_DIR_HTML		"line_pseudo.html"
#define		SKIN_MENU_LINE_JPEG_FILE_HTML		"line_jpeg.html"
#define		SKIN_MENU_TAIL_HTML					"tail.html"

#ifdef NEED_SKIN_MAPPING_DEFINITION
SKIN_MAPPING_T skin_mapping[] = {
	{TYPE_UNKNOWN,		SKIN_MENU_LINE_UNKNOWN_FILE_HTML},
	{TYPE_DIRECTORY,	SKIN_MENU_LINE_DIR_HTML},
	{TYPE_PSEUDO_DIR,	SKIN_MENU_LINE_PSEUDO_DIR_HTML},
	{TYPE_MOVIE,		SKIN_MENU_LINE_MOVIE_FILE_HTML},
	{TYPE_PLAYLIST,		SKIN_MENU_LINE_MOVIE_FILE_HTML}, // use movie one...
	{TYPE_MUSIC,		SKIN_MENU_LINE_MUSIC_FILE_HTML},
	{TYPE_MUSICLIST,	SKIN_MENU_LINE_MUSIC_FILE_HTML}, // use music one...
	{TYPE_IMAGE,		SKIN_MENU_LINE_IMAGE_FILE_HTML},
	{TYPE_DOCUMENT,		SKIN_MENU_LINE_DOCUMENT_FILE_HTML},
	{TYPE_SVI,			SKIN_MENU_LINE_SVI_FILE_HTML},
	{TYPE_JPEG,			SKIN_MENU_LINE_JPEG_FILE_HTML},
	{-1,				NULL},
};
#else
extern SKIN_MAPPING_T skin_mapping[];
// extern, but never be used? :p
#endif

#define		SKIN_KEYWORD_SERVER_NAME		"<!--WIZD_INSERT_SERVER_NAME-->"		// ������̾���С������ɽ����
#define		SKIN_KEYWORD_CURRENT_PATH		"<!--WIZD_INSERT_CURRENT_PATH-->"		// ��PATH��ɽ����
#define		SKIN_KEYWORD_CURRENT_DIR_NAME	"<!--WIZD_INSERT_CURRENT_DIR_NAME-->"		// ���ǥ��쥯�ȥ�̾��ɽ����
#define		SKIN_KEYWORD_CURRENT_DATE		"<!--WIZD_INSERT_CURRENT_DATE-->"				// ����ɽ����
#define		SKIN_KEYWORD_CURRENT_TIME		"<!--WIZD_INSERT_CURRENT_TIME-->"				// ����ɽ����

#define		SKIN_KEYWORD_PARLENT_DIR_LINK	"<!--WIZD_INSERT_PARENT_DIR_LINK-->"	// �ƥǥ��쥯�ȥꡣLINK�� URI���󥳡��ɺѤ�
#define		SKIN_KEYWORD_PARLENT_DIR_NAME	"<!--WIZD_INSERT_PARENT_DIR_NANE-->"	// �ƥǥ��쥯�ȥꡣɽ����

#define		SKIN_KEYWORD_CURRENT_PATH_LINK	"<!--WIZD_INSERT_CURRENT_PATH_LINK-->"	// ��PATH��LINK�ѡ�URI���󥳡��ɺѤ�
#define		SKIN_KEYWORD_CURRENT_PATH_LINK_NO_PARAM	"<!--WIZD_INSERT_CURRENT_PATH_LINK_NO_PARAM-->"	// ��PATH��LINK�ѡ�URI���󥳡��ɺѤ�


#define		SKIN_KEYWORD_CURRENT_PAGE		"<!--WIZD_INSERT_CURRENT_PAGE-->"		// ���ߤΥڡ���
#define		SKIN_KEYWORD_MAX_PAGE			"<!--WIZD_INSERT_MAX_PAGE-->"			// ���ڡ�����
#define		SKIN_KEYWORD_NEXT_PAGE			"<!--WIZD_INSERT_NEXT_PAGE-->"			// ���Υڡ���
#define		SKIN_KEYWORD_PREV_PAGE			"<!--WIZD_INSERT_PREV_PAGE-->"			// ���Υڡ���
#define		SKIN_KEYWORD_FILE_NUM			"<!--WIZD_INSERT_FILE_NUM-->"			// �ǥ��쥯�ȥ�¸�ߥե������
#define		SKIN_KEYWORD_START_FILE_NUM		"<!--WIZD_INSERT_START_FILE_NUM-->"		// ���Υڡ���
#define		SKIN_KEYWORD_END_FILE_NUM		"<!--WIZD_INSERT_END_FILE_NUM-->"		// ���Υڡ���

#define		SKIN_KEYWORD_ONLOADSET_FOCUS		"<!--WIZD_INSERT_ONLOADSET_FOCUS-->"		// BODY������ onloadset="$focus"

#define		SKIN_KEYWORD_CLIENT_CHARSET		"<!--WIZD_INSERT_CLIENT_CHARSET-->"		// ���饤����Ȥδ���������




#define		SKIN_KEYWORD_LINE_FILE_NAME			"<!--WIZD_INSERT_LINE_FILE_NAME-->"			// �ե�����̾ ɽ����
#define		SKIN_KEYWORD_LINE_FILE_NAME_NO_EXT	"<!--WIZD_INSERT_LINE_FILE_NAME_NO_EXT-->"	// �ե�����̾(��ĥ��̵��) ɽ����
#define		SKIN_KEYWORD_LINE_FILE_EXT			"<!--WIZD_INSERT_LINE_FILE_EXT-->"			// �ե������ĥ�� ɽ����
#define		SKIN_KEYWORD_LINE_FILE_LINK			"<!--WIZD_INSERT_LINE_FILE_LINK-->"			// �ե�����̾ ����� URI���󥳡���

#define		SKIN_KEYWORD_LINE_TIMESTAMP		"<!--WIZD_INSERT_LINE_TIMESTAMP-->"		// �����ॹ����� ����(YYYY/MM/DD HH:MM) ɽ����

#define		SKIN_KEYWORD_LINE_FILE_DATE		"<!--WIZD_INSERT_LINE_FILE_DATE-->"		// �����ॹ����� ���դΤ�(YYYY/MM/DD) ɽ����
#define		SKIN_KEYWORD_LINE_FILE_TIME		"<!--WIZD_INSERT_LINE_FILE_TIME-->"		// �����ॹ����� ����Τ�(HH:MM) ɽ����


#define		SKIN_KEYWORD_LINE_COLUMN_NUM	"<!--WIZD_INSERT_LINE_COLUMN_NUM-->"	// ���ֹ�
#define		SKIN_KEYWORD_LINE_ROW_NUM	"<!--WIZD_INSERT_LINE_ROW_NUM-->"	// ���ֹ�

#define		SKIN_KEYWORD_LINE_TVID			"<!--WIZD_INSERT_LINE_TVID-->"			// TVID

#define		SKIN_KEYWORD_LINE_FILE_VOD		"<!--WIZD_INSERT_LINE_FILE_VOD-->"		// vod="0"  ɬ�פ˱������դ���
#define		SKIN_KEYWORD_LINE_FILE_SIZE		"<!--WIZD_INSERT_LINE_FILE_SIZE-->"		// �ե����륵���� ɽ����

#define		SKIN_KEYWORD_LINE_SVI_INFO		"<!--WIZD_INSERT_LINE_SVI_INFO-->"		// SVI�ե����뤫���ɤ������ ɽ����
#define		SKIN_KEYWORD_LINE_SVI_REC_TIME	"<!--WIZD_INSERT_LINE_SVI_REC_TIME-->"	// SVI�ե������Ͽ�����

#define		SKIN_KEYWORD_LINE_IMAGE_WIDTH	"<!--WIZD_INSERT_LINE_IMAGE_WIDTH-->"	// �����β���
#define		SKIN_KEYWORD_LINE_IMAGE_HEIGHT	"<!--WIZD_INSERT_LINE_IMAGE_HEIGHT-->"	// �����ι⤵

#define		SKIN_KEYWORD_SECRET_DIR_LINK	"<!--WIZD_INSERT_SECRET_DIR_LINK-->"	// �����ǥ��쥯�ȥ�


#define		SKIN_KEYWORD_LINE_MP3TAG_TITLE		"<!--WIZD_INSERT_LINE_MP3TAG_TITLE-->"		// MP3���� �����ȥ�
#define		SKIN_KEYWORD_LINE_MP3TAG_ALBUM		"<!--WIZD_INSERT_LINE_MP3TAG_ALBUM-->"		// MP3���� ����Х�̾
#define		SKIN_KEYWORD_LINE_MP3TAG_ARTIST		"<!--WIZD_INSERT_LINE_MP3TAG_ARTIST-->"		// MP3���� �����ƥ�����
#define		SKIN_KEYWORD_LINE_MP3TAG_YEAR		"<!--WIZD_INSERT_LINE_MP3TAG_YEAR-->"		// MP3���� ����ǯ��
#define		SKIN_KEYWORD_LINE_MP3TAG_COMMENT	"<!--WIZD_INSERT_LINE_MP3TAG_COMMENT-->"	// MP3���� ������

#define		SKIN_KEYWORD_LINE_MP3TAG_TITLE_INFO	"<!--WIZD_INSERT_LINE_MP3TAG_TITLE_INFO-->"	// MP3���� �����ȥ�[����Х�̾/�����ƥ�����] ɽ��(menu_filename_length_max�ˤ�����¤����)

#define		SKIN_KEYWORD_LINE_AVI_FPS		"<!--WIZD_INSERT_LINE_AVI_FPS-->"	// AVI��FPS

#define		SKIN_KEYWORD_LINE_AVI_DURATION	"<!--WIZD_INSERT_LINE_AVI_DURATION-->"	// AVI�κ�������

#define		SKIN_KEYWORD_LINE_AVI_VCODEC	"<!--WIZD_INSERT_LINE_AVI_VCODEC-->"	// AVI��ư�襳���ǥå�

#define		SKIN_KEYWORD_LINE_AVI_ACODEC	"<!--WIZD_INSERT_LINE_AVI_ACODEC-->"	// AVI�β��������ǥå�

#define		SKIN_KEYWORD_LINE_AVI_HVCODEC	"<!--WIZD_INSERT_LINE_AVI_HVCODEC-->"	// AVI��ư�襳���ǥå�(in the avi stream header)

#define		SKIN_KEYWORD_LINE_AVI_HACODEC	"<!--WIZD_INSERT_LINE_AVI_HACODEC-->"	// AVI�β��������ǥå�(in the avi stream header)

#define		SKIN_KEYWORD_LINE_AVI_IS_INTERLEAVED	"<!--WIZD_INSERT_LINE_AVI_IS_INTERLEAVED-->"	// AVI�����󥿡��꡼�֤���Ƥ��뤫


// �ʲ��Υ�����ɤǶ��ޤ줿���ꥢ�ϡ������פ����Ȥ��˺������롣

// �롼�ȥǥ��쥯�ȥ�ξ�� (HEAD/TAIL�Τ�)
#define		SKIN_KEYWORD_DEL_IS_ROOTDIR				"<!--WIZD_DELETE_IS_ROOTDIR-->"
#define		SKIN_KEYWORD_DEL_IS_ROOTDIR_E			"<!--/WIZD_DELETE_IS_ROOTDIR-->"

/* �����ߴ� */
// ���ڡ�����¸�ߤ��ʤ���� (HEAD/TAIL�Τ�)
#define		SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV		"<!--WIZD_DELETE_IS_NO_PAGE_PREV-->"
#define		SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV_E		"<!--/WIZD_DELETE_IS_NO_PAGE_PREV-->"

// ���ڡ�����¸�ߤ��ʤ���� (HEAD/TAIL�Τ�)
#define		SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT		"<!--WIZD_DELETE_IS_NO_PAGE_NEXT-->"
#define		SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT_E		"<!--/WIZD_DELETE_IS_NO_PAGE_NEXT-->"

// ������ǽ�ե����뤬¸�ߤ��ʤ���� (HEAD/TAIL�Τ�)
#define		SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES		"<!--WIZD_DELETE_IS_NO_STREAM_FILES-->"
#define		SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES_E	"<!--/WIZD_DELETE_IS_NO_STREAM_FILES-->"

/* ����, DELETE (IF THERE) IS NO... ���ɤ� */
// ���ڡ�����¸�ߤ��ʤ���� (HEAD/TAIL�Τ�)
#define		SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV2		"<!--WIZD_IF_PAGE_PREV-->"
#define		SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV2_E		"<!--/WIZD_IF_PAGE_PREV-->"

// ���ڡ�����¸�ߤ��ʤ���� (HEAD/TAIL�Τ�)
#define		SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT2		"<!--WIZD_IF_PAGE_NEXT-->"
#define		SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT2_E		"<!--/WIZD_IF_PAGE_NEXT-->"

// ������ǽ�ե����뤬¸�ߤ��ʤ���� (HEAD/TAIL�Τ�)
#define		SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES2	"<!--WIZD_IF_STREAM_FILES-->"
#define		SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES2_E	"<!--/WIZD_IF_STREAM_FILES-->"

// ���ڡ�����¸�ߤ������� (HEAD/TAIL�Τ�)
#define		SKIN_KEYWORD_DEL_IS_PAGE_PREV		"<!--WIZD_IF_NO_PAGE_PREV-->"
#define		SKIN_KEYWORD_DEL_IS_PAGE_PREV_E		"<!--/WIZD_IF_NO_PAGE_PREV-->"

// ���ڡ�����¸�ߤ������� (HEAD/TAIL�Τ�)
#define		SKIN_KEYWORD_DEL_IS_PAGE_NEXT		"<!--WIZD_IF_NO_PAGE_NEXT-->"
#define		SKIN_KEYWORD_DEL_IS_PAGE_NEXT_E		"<!--/WIZD_IF_NO_PAGE_NEXT-->"

// ������ǽ�ե����뤬¸�ߤ������� (HEAD/TAIL�Τ�)
#define		SKIN_KEYWORD_DEL_IS_STREAM_FILES	"<!--WIZD_IF_NO_STREAM_FILES-->"
#define		SKIN_KEYWORD_DEL_IS_STREAM_FILES_E	"<!--/WIZD_IF_NO_STREAM_FILES-->"


// MP3������¸�ߤ��ʤ��Ȥ� (LINE�Τ�)
#define		SKIN_KEYWORD_DEL_IS_NO_MP3_TAGS			"<!--WIZD_DELETE_IS_NO_MP3_TAGS-->"
#define		SKIN_KEYWORD_DEL_IS_NO_MP3_TAGS_E		"<!--/WIZD_DELETE_IS_NO_MP3_TAGS-->"

// MP3������¸�ߤ���Ȥ� (LINE�Τ�)
#define		SKIN_KEYWORD_DEL_IS_HAVE_MP3_TAGS		"<!--WIZD_DELETE_IS_HAVE_MP3_TAGS-->"
#define		SKIN_KEYWORD_DEL_IS_HAVE_MP3_TAGS_E		"<!--/WIZD_DELETE_IS_HAVE_MP3_TAGS-->"


// �Ԥ����/�����ΤȤ� (LINE�Τ�)
#define		SKIN_KEYWORD_DEL_IF_LINE_IS_ODD			"<!--WIZD_IF_LINE_IS_EVEN-->"	// �Ԥ�����ΤȤ����
#define		SKIN_KEYWORD_DEL_IF_LINE_IS_ODD_E		"<!--/WIZD_IF_LINE_IS_EVEN-->"	// �Ԥ�����ΤȤ����

#define		SKIN_KEYWORD_DEL_IF_LINE_IS_EVEN		"<!--WIZD_IF_LINE_IS_ODD-->"	// �Ԥ������ΤȤ����
#define		SKIN_KEYWORD_DEL_IF_LINE_IS_EVEN_E		"<!--/WIZD_IF_LINE_IS_ODD-->"	// �Ԥ������ΤȤ����


// ���饤����Ȥ�PC�ΤȤ����
#define		SKIN_KEYWORD_DEL_IF_CLIENT_IS_PC		"<!--WIZD_IF_CLIENT_IS_NOT_PC-->"
#define		SKIN_KEYWORD_DEL_IF_CLIENT_IS_PC_E		"<!--/WIZD_IF_CLIENT_IS_NOT_PC-->"

// ���饤����Ȥ�PC�ǤϤʤ��Ȥ����
#define		SKIN_KEYWORD_DEL_IF_CLIENT_IS_NOT_PC		"<!--WIZD_IF_CLIENT_IS_PC-->"
#define		SKIN_KEYWORD_DEL_IF_CLIENT_IS_NOT_PC_E		"<!--/WIZD_IF_CLIENT_IS_PC-->"


// focus �����ꤵ��Ƥ���Ȥ����
#define		SKIN_KEYWORD_DEL_IF_FOCUS_IS_SPECIFIED		"<!--WIZD_IF_FOCUS_IS_NOT_SPECIFIED-->"
#define		SKIN_KEYWORD_DEL_IF_FOCUS_IS_SPECIFIED_E	"<!--/WIZD_IF_FOCUS_IS_NOT_SPECIFIED-->"

// focus �����ꤵ��Ƥ��ʤ��Ȥ����
#define		SKIN_KEYWORD_DEL_IF_FOCUS_IS_NOT_SPECIFIED		"<!--WIZD_IF_FOCUS_IS_SPECIFIED-->"
#define		SKIN_KEYWORD_DEL_IF_FOCUS_IS_NOT_SPECIFIED_E	"<!--/WIZD_IF_FOCUS_IS_SPECIFIED-->"


#define		SKIN_IMAGE_VIEWER_HTML 	"image_viewer.html"	// ImageViewer�Υ�����
#define		SKIN_JPEG_IMAGE_VIEWER_HTML 	"jpeg_image_viewer.html"	// ImageViewer�Υ�����

#define		SKIN_KEYWORD_IMAGE_VIEWER_WIDTH		"<!--WIZD_INSERT_IMAGE_VIEWER_WIDTH-->"		// ImageViewer��ɽ������
#define		SKIN_KEYWORD_IMAGE_VIEWER_HEIGHT	"<!--WIZD_INSERT_IMAGE_VIEWER_HEIGHT-->"	// ImageViewer��ɽ���⤵
#define		SKIN_KEYWORD_IMAGE_VIEWER_MODE		"<!--WIZD_INSERT_IMAGE_VIEWER_MODE-->"		// ImageViewer�θ��ߤΥ⡼��



// FIT�⡼�ɤλ����ʲ����ϰϤ���
#define		SKIN_KEYWORD_DEL_IS_FIT_MODE	"<!--WIZD_DELETE_IS_FIT_MODE-->"
#define		SKIN_KEYWORD_DEL_IS_FIT_MODE_E	"<!--/WIZD_DELETE_IS_FIT_MODE-->"


// FIT�⡼�ɰʳ��λ����ʲ����ϰϤ���
#define		SKIN_KEYWORD_DEL_IS_NO_FIT_MODE		"<!--WIZD_DELETE_IS_NO_FIT_MODE-->"
#define		SKIN_KEYWORD_DEL_IS_NO_FIT_MODE_E	"<!--/WIZD_DELETE_IS_NO_FIT_MODE-->"


#endif
