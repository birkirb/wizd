// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd.h
//											$Revision: 1.37 $
//											$Date: 2006/11/02 01:15:38 $
//
//	すべて自己責任でおながいしまつ。
//  このソフトについてVertexLinkに問い合わせないでください。
// ==========================================================================
#ifndef	_WIZD_H
#define	_WIZD_H


#include	"wizd_tools.h"
#include <dvdread/dvd_reader.h>

// ======================
// define いろいろ
// ======================

#define		SERVER_NAME		"wizd v24.0"
#define		SERVER_DETAIL	"MediaWiz Server Daemon."


#ifndef TRUE
#define	TRUE	(1)
#endif
#ifndef FALSE
#define	FALSE	(0)
#endif



#define		DEFAULT_MUSICLIST	"default_music.m3u"
#define		DEFAULT_PHOTOLIST	"default_photo.pls"



#define		LISTEN_BACKLOG	(32)

#define		SEND_BUFFER_SIZE	(1024*128)


#define DEFAULT_SERVER_PORT (8000)
#define	DEFAULT_DOCUMENT_ROOT	"/"


#define	DEFAULT_FLAG_DAEMON			TRUE
#define	DEFAULT_FLAG_AUTO_DETECT	TRUE




#define	DEFAULT_MIME_TYPE		"text/plain"

#define	DEFAULT_INDEX_FILENAME	"index.html"

#ifdef NEED_CONFIG_FILE_DEFINITION
char *config_file[] = {
	"./wizd.conf",
	"/usr/local/wizd/wizd.conf",
	"/usr/local/etc/wizd.conf",
	"/etc/wizd.conf",
};
#endif


#define	DEFAULT_FLAG_DEBUG_LOG_OUTPUT	FALSE
#define	DEFAULT_DEBUG_LOG_FILENAME		""


#define	DEFAULT_FLAG_USE_SKIN		FALSE
#define	DEFAULT_SKINDATA_ROOT		"./skin"
#define	DEFAULT_SKINDATA_NAME		"default"

// execute path for CGI
#define	DEFAULT_PATH	"/usr/bin:/bin:/usr/sbin:/usr/bin"

#define	DEFAULT_FLAG_HIDE_SAME_SVI_NAME_DIRECTORY	FALSE

#define	DEFAULT_MENU_FILENAME_LENGTH_MAX		FILENAME_MAX
#define	DEFAULT_THUMB_FILENAME_LENGTH_MAX		FILENAME_MAX
#define	DEFAULT_MENU_SVI_INFO_LENGTH_MAX		FILENAME_MAX

// Maximum number of aliases allowed in wizd.conf
#define WIZD_MAX_ALIASES 50
// Maximum length of the alias name
#define WIZD_MAX_ALIAS_LENGTH 50

// Number of Favorite that can be defined
#define WIZD_MAX_FAVORITES	10

// Alternate skin name options
#define WIZD_MAX_ALT_SKIN 10
#define WIZD_MAX_SKIN_NAME_LEN 32
#define WIZD_MAX_SKIN_MATCH_LEN 64

#define DEFAULT_BUFFER_SIZE  16
#define DEFAULT_STREAM_CHUNK_SIZE 65536
#define DEFAULT_FILE_CHUNK_SIZE 65536
#define DEFAULT_SOCKET_CHUNK_SIZE 8192
#define DEFAULT_STREAM_RCVBUF 0
#define DEFAULT_STREAM_SNDBUF 0

#define WIZD_FILENAME_MAX		2048

#define	DEFAULT_FLAG_UNKNOWN_EXTENSION_FLAG_HIDE	TRUE

#define	DEFAULT_FLAG_FILENAME_CUT_PARENTHESIS_AREA	FALSE

#define	DEFAULT_FLAG_DECODE_SAMBA_HEX_AND_CAP			FALSE
#define	DEFAULT_FLAG_FILENAME_CUT_SAME_DIRECTORY_NAME	FALSE

#define	DEFAULT_FLAG_ALLPLAY_FILELIST_ADJUST			FALSE
#define DEFAULT_FLAG_ALLPLAY_INCLUDES_SUBDIR		TRUE
#define DEFAULT_MAX_PLAY_LIST_ITEMS			500
#define DEFAULT_MAX_PLAY_LIST_SEARCH			2000
#define DEFAULT_BOOKMARK_THRESHOLD			10000000
#define DEFAULT_FLAG_ALLOW_DELETE			FALSE

#define	DEFAULT_FLAG_FILENAME_ADJUSTMENT_FOR_WINDOWS	FALSE

#define	DEFAULT_FLAG_EXECUTE_CGI	TRUE
#define	DEFAULT_FLAG_ALLOW_PROXY	TRUE

#define	DEFAULT_DEBUG_CGI_OUTPUT		"/dev/null"

#define	DEFAULT_USER_AGENT_PC		""

#define	DEFAULT_FLAG_SHOW_FIRST_VOB_ONLY	TRUE
#define DEFAULT_FLAG_SPLIT_VOB_CHAPTERS		FALSE
#define DEFAULT_FLAG_HIDE_SHORT_TITLES		TRUE
#define DEFAULT_FLAG_SHOW_AUDIO_INFO		FALSE
#define DEFAULT_FLAG_BOOKMARKS_ONLY_FOR_MPEG	TRUE
#define DEFAULT_FLAG_GOTO_PERCENT_VIDEO		TRUE
#define DEFAULT_FLAG_GOTO_PERCENT_AUDIO		FALSE

#define DEFAULT_FLAG_FANCY_VIDEO_TS_PAGE	FALSE
#define DEFAULT_FLAG_FANCY_MUSIC_PAGE	FALSE

#define DEFAULT_FANCY_LINE_CNT	10

#define DEFAULT_FLAG_DVDCSS_LIB	FALSE

#define	DEFAULT_FLAG_SPECIFIC_DIR_SORT_TYPE_FIX TRUE

// Slide show configuration
#define DEFAULT_FLAG_SLIDE_SHOW_LABELS		FALSE
#define DEFAULT_SLIDE_SHOW_SECONDS		10
#define DEFAULT_SLIDE_SHOW_TRANSITION		0
#define DEFAULT_MINIMUM_JPEG_SIZE		0

#define	SORT_NONE				(0)
#define	SORT_NAME_UP			(1)
#define	SORT_NAME_DOWN			(2)
#define	SORT_TIME_UP			(3)
#define	SORT_TIME_DOWN			(4)
#define	SORT_SIZE_UP			(5)
#define	SORT_SIZE_DOWN			(6)
#define	SORT_SHUFFLE			(7)
#define	SORT_DURATION			(8)

#define	DEFAULT_SORT_RULE		SORT_NONE
#define	DEFAULT_PAGE_LINE_MAX	(14)
#define DEFAULT_THUMB_ROW_MAX    (6)
#define DEFAULT_THUMB_COLUMN_MAX (4)
#define DEFAULT_FLAG_DEFAULT_THUMB  FALSE
#define DEFAULT_FLAG_DEFAULT_THUMB_DETAIL  FALSE
#define DEFAULT_MENU_ICON_TYPE		"gif"

#define	NKF_CODE_SJIS			"s"
#define	NKF_CODE_EUC			"e"


#define CODE_AUTO		(0x00)
#define CODE_SJIS		(0x01)
#define CODE_EUC		(0x02)
#define CODE_UTF8		(0x03)
#define CODE_UTF16		(0x04)
#define CODE_UNIX               (0x05)
#define CODE_WINDOWS            (0x06)
#define CODE_HEX		(0x10)
#define CODE_DISABLED		(0x20)

#define	DEFAULT_CLIENT_LANGUAGE_CODE	CODE_UNIX
#define	DEFAULT_SERVER_LANGUAGE_CODE	CODE_AUTO



#define	HTTP_USER_AGENT		"User-agent:"
#define	HTTP_RANGE			"Range:"
#define	HTTP_HOST			"Host:"
#define	HTTP_AUTHORIZATION	"Authorization:"
#define	HTTP_CONTENT_LENGTH_STR	"Content-Length:"

#define	HTTP_OK 			"HTTP/1.0 200 OK\r\n"
#define	HTTP_NOT_FOUND 		"HTTP/1.0 404 File Not Found\r\n"
#define HTTP_CONTENT_LENGTH	"Content-Length: %llu\r\n"
#define	HTTP_ACCEPT_RANGES	"Accept-Ranges: bytes\r\n"
#define HTTP_CONTENT_TYPE 	"Content-Type: %s\r\n"
#define	HTTP_SERVER_NAME	"Server: %s\r\n"
#define	HTTP_CONNECTION		"Connection: Close\r\n"
#define HTTP_END			"\r\n"


// アクセスコントロール 登録可能数
#define		ACCESS_ALLOW_LIST_MAX	(32)


#define		ALLOW_USER_AGENT_LIST_MAX	(32)

// 隠しディレクトリ登録可能数
#define		SECRET_DIRECTORY_MAX	(8)


// MIME_LIST_T.stream_type 用
#define		TYPE_MASK 		0xF8
#define		FILE_BASE		0x00
#define		SPECIAL_BASE		0x08
#define		STREAMED_BASE		0x10
#define		DIRECTORY_BASE		0x18

// MIME_LIST.menu_file_type用
// FILE_BASE types are send as plain files with no special handling
#define		TYPE_UNKNOWN		(FILE_BASE  )
#define		TYPE_DOCUMENT		(FILE_BASE+1)
#define		TYPE_URL		(FILE_BASE+2)
#define		TYPE_IMAGE		(FILE_BASE+3)
#define		TYPE_JPEG		(FILE_BASE+4)

// Special cases
// SPECIAL_BASE types are special types which don't correspond to actual files
#define		TYPE_DELETE		(SPECIAL_BASE  )
#define		TYPE_SECRET		(SPECIAL_BASE+1)
#define		TYPE_ROW		(SPECIAL_BASE+2)
#define		TYPE_MP3INFO	    	(SPECIAL_BASE+3)
#define		TYPE_AVIINFO	   	(SPECIAL_BASE+4)

// STREAMED_BASE types are files sent using a playlist
#define		TYPE_MOVIE		(STREAMED_BASE)
#define		TYPE_MUSIC		(STREAMED_BASE+1)
#define		TYPE_SVI		(STREAMED_BASE+2)
#define		TYPE_PLAYLIST		(STREAMED_BASE+3)
#define		TYPE_MUSICLIST		(STREAMED_BASE+4)
#define		TYPE_CHAPTER		(STREAMED_BASE+5)
#define		TYPE_ISO		(STREAMED_BASE+6)

// DIRECTORY_BASE types are sorted as directories, can't be deleted
// and propagate the sort=, option=, dvdopt=, and title= fields
// from the incoming URL to the link URL
#define		TYPE_DIRECTORY		(DIRECTORY_BASE)
#define		TYPE_VIDEO_TS		(DIRECTORY_BASE+1)
#define		TYPE_PSEUDO_DIR		(DIRECTORY_BASE+2)

#define		MAX_TYPES		(DIRECTORY_BASE+3)


// SVI_INFO

#define	SVI_FILENAME_OFFSET		(4368)
#define	SVI_FILENAME_LENGTH		(255)

#define	SVI_INFO_OFFSET			(272)
#define	SVI_INFO_LENGTH			(1024)

#define	SVI_TOTAL_SIZE_OFFSET	(4984)
#define	SVI_TOTAL_SIZE_LENGTH	(5)

#define	SVI_REC_TIME_OFFSET		(4972)
#define	SVI_REC_TIME_LENGTH		(2)

// MAX_SEARCH_HITS default
#define DEFAULT_MAX_SEARCH_HITS	10000


// MIN, MAX マクロ
#ifndef MAX
#define MAX(A,B)	((A) > (B) ? (A):(B))
#endif
#ifndef MIN
#define MIN(A,B)	((A) < (B) ? (A):(B))
#endif


// ==========================================================================
// File check return types
//			 1: Directory
//			 2:SVI
//			 3:plw/upl
//			 4:tsv
//			 5:VOB video
//			 6:CGI script
//			 7:ISO image
// ==========================================================================

typedef enum {
	GENERAL_FILE_TYPE = 0,
	DIRECTORY_TYPE,
	SVI_TYPE,
	PLW_UPL_TYPE,
	TSV_TYPE,
	VOB_TYPE,
	CGI_TYPE,
	ISO_TYPE
} FILE_CHECK_TYPE_T;

 // ==========================================================================
// MIMEリスト保存用構造体
// ==========================================================================
typedef struct {
	unsigned char	*mime_name;
	unsigned char	*file_extension;
	int				menu_file_type;
} MIME_LIST_T;


// ==========================================================================
// 拡張子変換テーブル
// ==========================================================================
typedef struct {
	unsigned char	*org_extension;
	unsigned char	*rename_extension;
} EXTENSION_CONVERT_LIST_T;





// ==========================================================================
// HTTP Request情報保存用構造体
// ==========================================================================
typedef struct {
	unsigned char 	recv_uri[WIZD_FILENAME_MAX];		// 受信したURI(decoded)
	unsigned char 	user_agent[256];			// 受信したUser-Agent
	unsigned char	recv_host[256];				// 受信したホスト名

	unsigned char	recv_range[256];			// 受信した Range
	off_t	range_start_pos;			// Rangeデータ 開始位置
	off_t	range_end_pos;				// Rangeデータ 終了位置
	off_t	file_size;					// Size of the file on the hard drive

	int file_type;		// Type of file processing to perform (see FILE_CHECK_TYPE_T, above)
	int menu_file_type;	// Numerical version of the mime type (see TYPE_* defines, above)
	unsigned char	mime_type[128];		// character string to return for the mime type
	unsigned char	send_filename[WIZD_FILENAME_MAX];	// フルパス


	unsigned char	action[128];	// ?action=  の内容
	int				page;			// ?page=	で指定された表示ページ
	int				menupage;		// ?menupage=	で指定された表示ページ
	int			    title;			// ?title=  DVD title selection

	unsigned char 	option[32];		// ?option= の内容
	unsigned char	sort[32];		// ?sort= の内容
	unsigned char	dvdopt[32];		// ?dvdopt= の内容
	unsigned char	alias[32];		// ?alias= の内容
	unsigned char	search[64];		// ?search= の内容
	unsigned char	search_str[64];		// ?search= の内容
	unsigned char	lsearch[256];	// 

	unsigned char 	focus[32];		// ?focus= の内容

	unsigned char 	request_uri[WIZD_FILENAME_MAX];		// 受信した生のURI
	unsigned char 	request_method[64];		// requested method

	long 	recv_content_length;		// 受信したContent-Length. (わざとlong)

	int				search_type;	// 1=movie,2=music,3=photo,else all
	int				flag_pc;		// クライアントが PC かどうか. 非0 = PC
	int				flag_hd;		// True when the client is using HD resolution
	int				default_file_type;	// Default file type for allplay list

	unsigned char 	passwd[64];		// 受信したURI(decoded)
} HTTP_RECV_INFO;

// ==========================================================================
// 全体パラメータ保存用構造体
// ==========================================================================
typedef struct {


	// -----------------
	// システム系
	// -----------------

	// デーモン化する/しない
	char			flag_daemon;

	// デバッグログ
	char			flag_debug_log_output;
	unsigned char	debug_log_filename[WIZD_FILENAME_MAX];


	// 動作ユーザー名
	char	exec_user[32];
	char	exec_group[32];

	// -----------------
	// 自動検出系
	// -----------------

	// サーバホスト名
	unsigned char	server_name[32];

	char			flag_auto_detect;
	unsigned char	auto_detect_bind_ip_address[32];

	// --------------------
	// HTTP Server系
	// --------------------

	// HTTP Server Port
	int		server_port;

	// Document Root
	unsigned char 	document_root[WIZD_FILENAME_MAX];

	// Aliases
	int		num_aliases;
	int		num_real_aliases;
	unsigned char	alias_name[WIZD_MAX_ALIASES][WIZD_MAX_ALIAS_LENGTH];
	unsigned char	alias_path[WIZD_MAX_ALIASES][WIZD_FILENAME_MAX];
	int				alias_default_file_type[WIZD_MAX_ALIASES];

	// ----------------------
	// 表示系
	// ----------------------

	// MediaWiz の言語コード
	int	client_language_code;

	// Serverの言語コード
	int	server_language_code;


	// スキンを使用する／しない
	char			flag_use_skin;

	// スキン置き場
	unsigned char	skin_root[WIZD_FILENAME_MAX];

	// スキン名
	unsigned char	skin_name[WIZD_MAX_SKIN_NAME_LEN];

	int		alternate_skin_count;
	unsigned char	alternate_skin_match[WIZD_MAX_ALT_SKIN][WIZD_MAX_SKIN_MATCH_LEN];
	unsigned char	alternate_skin_name[WIZD_MAX_ALT_SKIN][WIZD_MAX_SKIN_NAME_LEN];

	// ファイルソートのルール
	int		sort_rule;

	//ファイルリストの１ページの最大表示数
	int		page_line_max;
	int		thumb_column_max;
	int		thumb_row_max;
	char		flag_default_thumb;
	char		flag_thumb_in_details;

	// ファイル名表示の最大長
	int		menu_filename_length_max;
	int		thumb_filename_length_max;

	// Default extension for menu icons
	char		menu_icon_type[8];

	//sambaのCAP/HEXエンコード使用するかフラグ
	char	flag_decode_samba_hex_and_cap;

	// wizd が知らないファイル名を隠すかフラグ
	char	flag_unknown_extention_file_hide;

	// 表示ファイル名から、()[]で囲まれた部分を削除するかフラグ
	char	flag_filename_cut_parenthesis_area;

	// 表示ファイル名で、親ディレクトリ名と同一文字列を削除するかフラグ
	char	flag_filename_cut_same_directory_name;

	//  SVIファイルと同一の名前を持つディレクトリを隠すかフラグ
	char	flag_hide_same_svi_name_directory;

	// Allplayでの文字化け防止(ファイル名の全半角変換)するかフラグ
	char	flag_allplay_filelist_adjust;

	// Allplay recurses through all subdirectories to create the list
	char	flag_allplay_includes_subdir;

	// Maximum number of entries to include in an allplay list sent to the client
	int	max_play_list_items;
	// maximum number of searched files for shuffling (must be higher than max_play_list_items)
	int	max_play_list_search;

	// Threshold for reading before storing a bookmark
	// set to 0 to disable bookmarks
	int	bookmark_threshold;

	// Flag to enable the ability to delete files from the client
	char	flag_allow_delete;

	// Slide show configuration
	char	flag_slide_show_labels;
	int	slide_show_seconds;
	int	slide_show_transition;
	int	minimum_jpeg_size;

	// Force use of the DVDOpen command from libdvdread
	// When false, wizd will directly read from the VOB files
	char	flag_always_use_dvdopen;

	// Windows用にプレイリスト内のファイル名を調整するかフラグ
	char	flag_filename_adjustment_for_windows;

	// SVIファイル情報表示のMAX長
	int		menu_svi_info_length_max;

	// ----------------------
	// 拡張系
	// ----------------------

	// Socket buffering options for streaming data
	int	stream_sndbuf;
	int	stream_rcvbuf;

	// キャッシュバッファのサイズ(個数)
	size_t	buffer_size;

	// Size of each buffer, in bytes
	int	stream_chunk_size;

	// Size of file read calls, in bytes
	int file_chunk_size;
	// Size of stream send calls, in bytes
	int	socket_chunk_size;

	// キャッシュバッファをすぐ送るかフラグ
	char	flag_buffer_send_asap;

	// プロクシで User-Agent を上書きするならその文字列
	char	user_agent_proxy_override[128];

	// 子プロセスの制限
	int		max_child_count;

	// CGIスクリプトの実行を許可するかフラグ
	int		flag_execute_cgi;

	// CGIスクリプトの標準エラー出力先
	unsigned char	debug_cgi_output[WIZD_FILENAME_MAX];

	// プロクシを許可するかフラグ
	int		flag_allow_proxy;

	// PCと判断する User-Agent
	unsigned char 	user_agent_pc[64];

	int				menu_font_metric;
	unsigned char	menu_font_metric_string[128];

	char	http_passwd[64];

	// 0.12f3 先頭vobのみ
	char	flag_show_first_vob_only;
	char	flag_split_vob_chapters;
	char	flag_hide_short_titles;
	char	flag_show_audio_info;
	char	flag_bookmarks_only_for_mpeg;
	char	flag_goto_percent_video;
	char	flag_goto_percent_audio;

	char	flag_fancy_video_ts_page;
	char	flag_fancy_music_page;

	int		fancy_line_cnt;

	char	flag_dvdcss_lib;

	// 0.12f4 特定のディレクトリのソート方法を固定する。
	char	flag_specific_dir_sort_type_fix;

	// 0.12gr jpegをリサイズする設定
	char    flag_resize_jpeg;
	int     target_jpeg_width;
	int     target_jpeg_height;
	int	allow_crop;
	double  widen_ratio;

	int	dummy_chapter_length;
	int		flag_read_mp3_tag;
	char	wizd_chdir[WIZD_FILENAME_MAX];
	int		flag_read_avi_tag;
	int		flag_sort_dir;

	int		flag_use_index;

	int		flag_default_search_by_alias;
	char	default_search_alias[WIZD_MAX_ALIAS_LENGTH];

	int		max_search_hits;

	char	search[64];

	char	favorites[WIZD_MAX_FAVORITES][WIZD_FILENAME_MAX];

	char	moviecollector[WIZD_FILENAME_MAX];
	char	musiccollector[WIZD_FILENAME_MAX];
} GLOBAL_PARAM_T;



// IPアドレスアクセスコントロール用
typedef struct {
	int				flag;			// 値が入ってるか
	unsigned char 	address[4];		// アドレス
	unsigned char 	netmask[4];		// ネットマスク

} ACCESS_CHECK_LIST_T;


// User-Agent アクセスコントロール
typedef struct {
	unsigned char	user_agent[64];
} ACCESS_USER_AGENT_LIST_T;


// 隠しディレクトリ
typedef struct {
	unsigned char	dir_name[64];	// 隠しディレクトリ名
	int				tvid;			// アクセスTVID
} SECRET_DIRECTORY_T;




#define		JOINT_MAX	(255)

// ********************************
// JOINTする個々のファイル情報
// ********************************
typedef struct {
	unsigned char	name[WIZD_FILENAME_MAX];
	u_int64_t		size;
	off_t			start_pos;		// starting position in file
} _FILE_INFO_T;


// ****************************************
// JOINTファイル情報 (SVI/VOB解析情報)
// ****************************************
typedef struct {
	int		file_num;		// 全ファイル数
	u_int64_t	total_size;		// 全ファイル総byte数

	_FILE_INFO_T	file[JOINT_MAX];	// JOINTファイル情報

	unsigned char	iso_name[WIZD_FILENAME_MAX];
	u_int64_t	iso_seek;
	dvd_file_t*	dvd_file;

	unsigned int	current_file_num;			// とりあえずVOB専用
} JOINT_FILE_INFO_T;




// ======================
// extern いろいろ
// ======================

// ------------------
// 各種リスト
// ------------------
extern GLOBAL_PARAM_T	global_param;
extern MIME_LIST_T	mime_list[];
extern EXTENSION_CONVERT_LIST_T extension_convert_list[];


// アクセス許可リスト
extern ACCESS_CHECK_LIST_T	access_allow_list[ACCESS_ALLOW_LIST_MAX];

// User-Agent 許可リスト
extern ACCESS_USER_AGENT_LIST_T	allow_user_agent[ALLOW_USER_AGENT_LIST_MAX];

// 隠しディレクトリ リスト
extern SECRET_DIRECTORY_T secret_directory_list[SECRET_DIRECTORY_MAX];

// ------------------
// グローバル関数
// ------------------


// wizd 初期化
extern void global_param_init(void);

// config_file(wizd.conf) 読み込み部
extern void config_file_read(void);

extern void config_sanity_check(void);

extern void skin_config_file_read(unsigned char *skin_conf_filename);


// MediaWiz 自動登録部
extern void	server_detect(void);


// HTTP 待ち受け部
extern void server_listen(void);


// HTTP処理部
extern void server_http_process(int accept_socket);


// HTTP_OKのヘッダを生成して送信
extern void http_send_ok_header(int accept_socket, unsigned long long content_length, char *content_type);


// バッファリングしながら in_fd から out_fd へ データを転送
extern off_t copy_descriptors(int in_fd, int out_fd, off_t content_length, JOINT_FILE_INFO_T *joint_file_info_p);
extern off_t http_simple_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos );


// SVIファイル解析＆返信
extern int http_joint_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

extern int read_svi_info(unsigned char *svi_filename, unsigned char *svi_info, int svi_info_size, unsigned int *rec_time );
extern u_int64_t svi_file_total_size(unsigned char *svi_filename);

extern int analyze_vob_file(unsigned char *vob_filename, JOINT_FILE_INFO_T *joint_file_info_p );


// Proxy解析＆返信
extern int http_proxy_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

// VOBファイル解析＆返信
extern int http_vob_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

// Send the decoded contents of an ISO image
//extern int http_iso_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

// CGI解析＆返信
extern int http_cgi_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

// ファイル実体返信
extern int http_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);


// メニュー部
extern void http_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info);

// Image Viewer（画像表示部)
extern void http_image_viewer(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// Option Menu(Sortモード変更)
extern void http_option_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// Music Single Play.(音楽ファイル １曲分の演奏データ送信)
extern void http_music_single_play(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// wizd play list(*.plw)ファイル処理。
extern void http_listfile_to_playlist_create(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// MIME
extern int check_file_extension_to_mime_type(const unsigned char *file_extension, unsigned char *mime_type, int mime_type_size );

extern MIME_LIST_T *lookup_mime_by_ext(char *file_extension);


// 日本語文字コード変換(NKFラッパー）
extern void convert_language_code(const unsigned char *in, unsigned char *out, size_t len, int in_flag, int out_flag);

extern unsigned char *base64(unsigned char *instr);

// ========================================================
// 文字コード変換。
// libnkfをそのまま使用。作者様に感謝ヽ(´ー｀)ノ
// http://www.mr.hum.titech.ac.jp/~morimoto/libnkf/
// ========================================================
extern int nkf(const char *, char *, size_t, const char *);


// JPEG のリサイズ
#ifdef RESIZE_JPEG
extern int http_send_resized_jpeg(int fd, HTTP_RECV_INFO *http_recv_info_p);
#endif

// 親ディレクトリ抽出処理
extern char *get_parent_path(char *dst, char *src, size_t len);

// strcasestr
extern char *my_strcasestr(const char *p1, const char *p2);

#endif

