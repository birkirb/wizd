// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd.h
//											$Revision: 1.41 $
//											$Date: 2005/01/11 04:24:53 $
//
//	���٤Ƽ�����Ǥ�Ǥ��ʤ������ޤġ�
//  ���Υ��եȤˤĤ���VertexLink���䤤��碌�ʤ��Ǥ���������
// ==========================================================================
#ifndef	_WIZD_H
#define	_WIZD_H


#include	"wizd_tools.h"

// ======================
// define ������
// ======================

#define		SERVER_NAME		"wizd 0.12h pvb.12"
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
#define	DEFAULT_DEBUG_LOG_FILENAME		"/tmp/wizd_debug.log"


#define	DEFAULT_FLAG_USE_SKIN		FALSE
#define	DEFAULT_SKINDATA_ROOT		"./skin"
#define	DEFAULT_SKINDATA_NAME		"default"

// execute path for CGI
#define	DEFAULT_PATH	"/usr/bin:/bin:/usr/sbin:/usr/bin"

#define	DEFAULT_FLAG_HIDE_SAME_SVI_NAME_DIRECTORY	FALSE

#define	DEFAULT_MENU_FILENAME_LENGTH_MAX		FILENAME_MAX
#define	DEFAULT_MENU_SVI_INFO_LENGTH_MAX		FILENAME_MAX

// Maximum number of aliases allowed in wizd.conf
#define WIZD_MAX_ALIASES 10
// Maximum length of the alias name
#define WIZD_MAX_ALIAS_LENGTH 50

#define DEFAULT_BUFFER_SIZE  16
#define DEFAULT_STREAM_CHUNK_SIZE 65536
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

#define	DEFAULT_FLAG_SPECIFIC_DIR_SORT_TYPE_FIX TRUE

// Slide show configuration
#define DEFAULT_FLAG_SLIDE_SHOW_LABELS		FALSE
#define DEFAULT_SLIDE_SHOW_SECONDS		10
#define DEFAULT_SLIDE_SHOW_TRANSITION		0

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

#define	DEFAULT_CLIENT_LANGUAGE_CODE	CODE_SJIS
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


// ������������ȥ��� ��Ͽ��ǽ��
#define		ACCESS_ALLOW_LIST_MAX	(32)


#define		ALLOW_USER_AGENT_LIST_MAX	(32)

// �����ǥ��쥯�ȥ���Ͽ��ǽ��
#define		SECRET_DIRECTORY_MAX	(4)


// MIME_LIST_T.stream_type ��
#define		TYPE_STREAM			(0)
#define		TYPE_NO_STREAM		(1)

// MIME_LIST.menu_file_type��
#define		TYPE_UNKNOWN			(0)
#define		TYPE_DIRECTORY			(1)
#define		TYPE_MOVIE				(2)
#define		TYPE_MUSIC				(3)
#define		TYPE_IMAGE				(4)
#define		TYPE_DOCUMENT			(5)
#define		TYPE_SVI				(6)
#define		TYPE_PLAYLIST			(7)
#define		TYPE_PSEUDO_DIR			(8)
#define		TYPE_MUSICLIST			(9)
#define		TYPE_JPEG				(10)
#define		TYPE_URL				(11)
#define		TYPE_DELETE				(12)
#define		TYPE_CHAPTER			(13)
// ���θĿ�
#define		MAX_TYPES				(14)


// SVI_INFO

#define	SVI_FILENAME_OFFSET		(4368)
#define	SVI_FILENAME_LENGTH		(255)

#define	SVI_INFO_OFFSET			(272)
#define	SVI_INFO_LENGTH			(1024)

#define	SVI_TOTAL_SIZE_OFFSET	(4984)
#define	SVI_TOTAL_SIZE_LENGTH	(5)

#define	SVI_REC_TIME_OFFSET		(4972)
#define	SVI_REC_TIME_LENGTH		(2)


// MIN, MAX �ޥ���
#ifndef MAX
#define MAX(A,B)	((A) > (B) ? (A):(B))
#endif
#ifndef MIN
#define MIN(A,B)	((A) < (B) ? (A):(B))
#endif


// ==========================================================================
// MIME�ꥹ����¸�ѹ�¤��
// ==========================================================================
typedef struct {
	unsigned char	*mime_name;
	unsigned char	*file_extension;
	int				stream_type;
	int				menu_file_type;
} MIME_LIST_T;


// ==========================================================================
// ��ĥ���Ѵ��ơ��֥�
// ==========================================================================
typedef struct {
	unsigned char	*org_extension;
	unsigned char	*rename_extension;
} EXTENSION_CONVERT_LIST_T;





// ==========================================================================
// HTTP Request������¸�ѹ�¤��
// ==========================================================================
typedef struct {
	unsigned char 	recv_uri[WIZD_FILENAME_MAX];		// ��������URI(decoded)
	unsigned char 	user_agent[256];			// ��������User-Agent
	unsigned char	recv_host[256];				// ���������ۥ���̾

	unsigned char	recv_range[256];			// �������� Range
	off_t	range_start_pos;			// Range�ǡ��� ���ϰ���
	off_t	range_end_pos;				// Range�ǡ��� ��λ����

	unsigned char	mime_type[128];		//
	unsigned char	send_filename[WIZD_FILENAME_MAX];	// �ե�ѥ�


	unsigned char	action[128];	// ?action=  ������
	int				page;			// ?page=	�ǻ��ꤵ�줿ɽ���ڡ���

	unsigned char 	option[32];		// ?option= ������
	unsigned char	sort[32];		// ?sort= ������
	unsigned char	dvdopt[32];		// ?dvdopt= ������

	unsigned char 	focus[32];		// ?focus= ������

	unsigned char 	request_uri[WIZD_FILENAME_MAX];		// ������������URI
	unsigned char 	request_method[64];		// requested method

	long 	recv_content_length;		// ��������Content-Length. (�虜��long)

	int				flag_pc;		// ���饤����Ȥ� PC ���ɤ���. ��0 = PC
	int				default_file_type;	// Default file type for allplay list

	int				hi_res;			// true when using hidef menus

	unsigned char 	passwd[64];		// ��������URI(decoded)
} HTTP_RECV_INFO;



// ==========================================================================
// ���Υѥ�᡼����¸�ѹ�¤��
// ==========================================================================
typedef struct {


	// -----------------
	// �����ƥ��
	// -----------------

	// �ǡ���󲽤���/���ʤ�
	char			flag_daemon;

	// �ǥХå���
	char			flag_debug_log_output;
	unsigned char	debug_log_filename[WIZD_FILENAME_MAX];


	// ư��桼����̾
	char	exec_user[32];
	char	exec_group[32];

	// -----------------
	// ��ư���з�
	// -----------------

	// �����Хۥ���̾
	unsigned char	server_name[32];

	char			flag_auto_detect;
	unsigned char	auto_detect_bind_ip_address[32];

	// --------------------
	// HTTP Server��
	// --------------------

	// HTTP Server Port
	int		server_port;

	// Document Root
	unsigned char 	document_root[WIZD_FILENAME_MAX];

	// Aliases
	int		num_aliases;
	unsigned char	alias_name[WIZD_MAX_ALIASES][WIZD_MAX_ALIAS_LENGTH];
	unsigned char	alias_path[WIZD_MAX_ALIASES][WIZD_FILENAME_MAX];
	int				alias_default_file_type[WIZD_MAX_ALIASES];

	// ----------------------
	// ɽ����
	// ----------------------

	// MediaWiz �θ��쥳����
	int	client_language_code;

	// Server�θ��쥳����
	int	server_language_code;


	// ���������Ѥ��롿���ʤ�
	char			flag_use_skin;

	// �������֤���
	unsigned char	skin_root[WIZD_FILENAME_MAX];

	// ������̾
	unsigned char	skin_name[32];

	// �ե����륽���ȤΥ롼��
	int		sort_rule;

	//�ե�����ꥹ�ȤΣ��ڡ����κ���ɽ����
	int		page_line_max;

	// �ե�����̾ɽ���κ���Ĺ
	int		menu_filename_length_max;

	//samba��CAP/HEX���󥳡��ɻ��Ѥ��뤫�ե饰
	char	flag_decode_samba_hex_and_cap;

	// wizd ���Τ�ʤ��ե�����̾�򱣤����ե饰
	char	flag_unknown_extention_file_hide;

	// ɽ���ե�����̾���顢()[]�ǰϤޤ줿��ʬ�������뤫�ե饰
	char	flag_filename_cut_parenthesis_area;

	// ɽ���ե�����̾�ǡ��ƥǥ��쥯�ȥ�̾��Ʊ��ʸ����������뤫�ե饰
	char	flag_filename_cut_same_directory_name;

	//  SVI�ե������Ʊ���̾������ĥǥ��쥯�ȥ�򱣤����ե饰
	char	flag_hide_same_svi_name_directory;

	// Allplay�Ǥ�ʸ�������ɻ�(�ե�����̾����Ⱦ���Ѵ�)���뤫�ե饰
	char	flag_allplay_filelist_adjust;

	// Allplay recurses through all subdirectories to create the list
	char	flag_allplay_includes_subdir;

	// Maximum number of entries to include in an allplay list
	int	max_play_list_items;

	// Threshold for reading before storing a bookmark
	// set to 0 to disable bookmarks
	int	bookmark_threshold;

	// Flag to enable the ability to delete files from the client
	char	flag_allow_delete;

	// Slide show configuration
	char	flag_slide_show_labels;
	int	slide_show_seconds;
	int	slide_show_transition;

	// Windows�Ѥ˥ץ쥤�ꥹ����Υե�����̾��Ĵ�����뤫�ե饰
	char	flag_filename_adjustment_for_windows;

	// SVI�ե��������ɽ����MAXĹ
	int		menu_svi_info_length_max;

	// ----------------------
	// ��ĥ��
	// ----------------------

	// Socket buffering options for streaming data
	int	stream_sndbuf;
	int	stream_rcvbuf;

	// ����å���Хåե��Υ�����(�Ŀ�)
	size_t	buffer_size;

	// Size of each buffer, in bytes
	int	stream_chunk_size;
	int	socket_chunk_size;

	// ����å���Хåե��򤹤����뤫�ե饰
	char	flag_buffer_send_asap;

	// �ץ����� User-Agent ���񤭤���ʤ餽��ʸ����
	char	user_agent_proxy_override[128];

	// �ҥץ���������
	int		max_child_count;

	// CGI������ץȤμ¹Ԥ���Ĥ��뤫�ե饰
	int		flag_execute_cgi;

	// CGI������ץȤ�ɸ�२�顼������
	unsigned char	debug_cgi_output[WIZD_FILENAME_MAX];

	// �ץ�������Ĥ��뤫�ե饰
	int		flag_allow_proxy;

	// PC��Ƚ�Ǥ��� User-Agent
	unsigned char 	user_agent_pc[64];

	int				menu_font_metric;
	unsigned char	menu_font_metric_string[128];

	char	http_passwd[64];

	// 0.12f3 ��Ƭvob�Τ�
	char	flag_show_first_vob_only;
	char	flag_split_vob_chapters;
	char	flag_hide_short_titles;
	char	flag_show_audio_info;

	// 0.12f4 ����Υǥ��쥯�ȥ�Υ�������ˡ����ꤹ�롣
	char	flag_specific_dir_sort_type_fix;

	// 0.12gr jpeg��ꥵ������������
	char    flag_resize_jpeg;
	int     target_jpeg_width;
	int     target_jpeg_height;
	double  widen_ratio;

	int		flag_read_mp3_tag;
	char	wizd_chdir[WIZD_FILENAME_MAX];
} GLOBAL_PARAM_T;



// IP���ɥ쥹������������ȥ�����
typedef struct {
	int				flag;			// �ͤ����äƤ뤫
	unsigned char 	address[4];		// ���ɥ쥹
	unsigned char 	netmask[4];		// �ͥåȥޥ���

} ACCESS_CHECK_LIST_T;


// User-Agent ������������ȥ���
typedef struct {
	unsigned char	user_agent[64];
} ACCESS_USER_AGENT_LIST_T;


// �����ǥ��쥯�ȥ�
typedef struct {
	unsigned char	dir_name[64];	// �����ǥ��쥯�ȥ�̾
	int				tvid;			// ��������TVID
} SECRET_DIRECTORY_T;




#define		JOINT_MAX	(255)

// ********************************
// JOINT����ġ��Υե��������
// ********************************
typedef struct {
	unsigned char	name[WIZD_FILENAME_MAX];
	u_int64_t		size;
} _FILE_INFO_T;


// ****************************************
// JOINT�ե�������� (SVI/VOB���Ͼ���)
// ****************************************
typedef struct {
	int				file_num;		// ���ե������
	off_t			total_size;		// ���ե�������byte��

	_FILE_INFO_T		file[JOINT_MAX];	// JOINT�ե��������

	unsigned int	current_file_num;			// �Ȥꤢ����VOB����
} JOINT_FILE_INFO_T;




// ======================
// extern ������
// ======================

// ------------------
// �Ƽ�ꥹ��
// ------------------
extern GLOBAL_PARAM_T	global_param;
extern MIME_LIST_T	mime_list[];
extern EXTENSION_CONVERT_LIST_T extension_convert_list[];


// �����������ĥꥹ��
extern ACCESS_CHECK_LIST_T	access_allow_list[ACCESS_ALLOW_LIST_MAX];

// User-Agent ���ĥꥹ��
extern ACCESS_USER_AGENT_LIST_T	allow_user_agent[ALLOW_USER_AGENT_LIST_MAX];

// �����ǥ��쥯�ȥ� �ꥹ��
extern SECRET_DIRECTORY_T secret_directory_list[SECRET_DIRECTORY_MAX];

// ------------------
// �����Х�ؿ�
// ------------------


// wizd �����
extern void global_param_init(void);

// config_file(wizd.conf) �ɤ߹�����
extern void config_file_read(void);

extern void config_sanity_check(void);

extern void skin_config_file_read(unsigned char *skin_conf_filename);


// MediaWiz ��ư��Ͽ��
extern void	server_detect(void);


// HTTP �Ԥ�������
extern void server_listen(void);


// HTTP������
extern void server_http_process(int accept_socket);


// HTTP_OK�Υإå���������������
extern void http_send_ok_header(int accept_socket, unsigned long long content_length, char *content_type);


// �Хåե���󥰤��ʤ��� in_fd ���� out_fd �� �ǡ�����ž��
extern off_t copy_descriptors(int in_fd, int out_fd, off_t content_length, JOINT_FILE_INFO_T *joint_file_info_p);


// SVI�ե�������ϡ��ֿ�
extern int http_joint_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

extern int read_svi_info(unsigned char *svi_filename, unsigned char *svi_info, int svi_info_size, unsigned int *rec_time );
extern u_int64_t svi_file_total_size(unsigned char *svi_filename);

extern int analyze_vob_file(unsigned char *vob_filename, JOINT_FILE_INFO_T *joint_file_info_p );


// Proxy���ϡ��ֿ�
extern int http_proxy_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

// VOB�ե�������ϡ��ֿ�
extern int http_vob_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

// CGI���ϡ��ֿ�
extern int http_cgi_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

// �ե���������ֿ�
extern int http_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);


// ��˥塼��
extern void http_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info, int flag_pseudo);


// Image Viewer�ʲ���ɽ����)
extern void http_image_viewer(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// Option Menu(Sort�⡼���ѹ�)
extern void http_option_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// Music Single Play.(���ڥե����� ����ʬ�α��եǡ�������)
extern void http_music_single_play(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// wizd play list(*.plw)�ե����������
extern void http_listfile_to_playlist_create(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// MIME
extern void check_file_extension_to_mime_type(const unsigned char *file_extension, unsigned char *mime_type, int mime_type_size );

extern MIME_LIST_T *lookup_mime_by_ext(char *file_extension);


// ���ܸ�ʸ���������Ѵ�(NKF��åѡ���
extern void convert_language_code(const unsigned char *in, unsigned char *out, size_t len, int in_flag, int out_flag);

extern unsigned char *base64(unsigned char *instr);

// ========================================================
// ʸ���������Ѵ���
// libnkf�򤽤Τޤ޻��ѡ�����ͤ˴��ա�(������)��
// http://www.mr.hum.titech.ac.jp/~morimoto/libnkf/
// ========================================================
extern int nkf(const char *, char *, size_t, const char *);


// JPEG �Υꥵ����
#ifdef RESIZE_JPEG
extern int http_send_resized_jpeg(int fd, HTTP_RECV_INFO *http_recv_info_p);
#endif

// �ƥǥ��쥯�ȥ���н���
extern char *get_parent_path(char *dst, char *src, size_t len);

// strcasestr
extern char *my_strcasestr(const char *p1, const char *p2);

#endif

