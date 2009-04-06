// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_param.c
//											$Revision: 1.24 $
//											$Date: 2005/01/12 13:40:23 $
//
//	���٤Ƽ�����Ǥ�Ǥ��ʤ������ޤġ�
//  ���Υ��եȤˤĤ���VertexLink���䤤��碌�ʤ��Ǥ���������
// ==========================================================================
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define NEED_CONFIG_FILE_DEFINITION
#include "wizd.h"


static int config_file_open(void);
static int config_file_read_line(int fd, unsigned char *line_buf, int line_buf_size);
static void line_buffer_clearance(unsigned char *line_buf);


// ********************************************
// MIME �ꥹ��
// �Ȥꤢ�����ΤäƤ�¤�񤤤Ƥ�����
// ********************************************
MIME_LIST_T	mime_list[] = {
//  {mime_name			,file_extension	, 	stream_type 	,	menu_file_type	},
	{"text/plain"		,	"txt"		,	TYPE_NO_STREAM	,	TYPE_DOCUMENT	},
	{"text/html"		, 	"htm"		,	TYPE_NO_STREAM	,	TYPE_DOCUMENT	},
	{"text/html"		, 	"html"		,	TYPE_NO_STREAM	,	TYPE_DOCUMENT	},
	{"image/gif"		, 	"gif"		,	TYPE_NO_STREAM	,	TYPE_IMAGE		},
	{"image/jpeg"		, 	"jpeg"		,	TYPE_NO_STREAM	,	TYPE_JPEG		},
	{"image/jpeg"		, 	"jpg"		,	TYPE_NO_STREAM	,	TYPE_JPEG		},
	{"image/png"		,	"png"		,	TYPE_NO_STREAM	,	TYPE_IMAGE		},
	{"video/mpeg"		, 	"mpeg"		,	TYPE_STREAM		,	TYPE_MOVIE		},
	{"video/mpeg"		, 	"mpg"		,	TYPE_STREAM		,	TYPE_MOVIE		},
	{"video/mpeg"		, 	"svi"		,	TYPE_STREAM		,	TYPE_SVI		},
	{"video/mpeg"		, 	"sv3"		,	TYPE_STREAM		,	TYPE_SVI		},
	{"video/mpeg"		, 	"m2p"		,	TYPE_STREAM		,	TYPE_MOVIE		},
	{"video/mpeg"		, 	"hnl"		,	TYPE_STREAM		,	TYPE_MOVIE		},
	{"video/mpeg"		, 	"nuv"		,	TYPE_STREAM		,	TYPE_MOVIE		},	/* add for MythTV */
	{"video/msvideo"	, 	"avi"		,	TYPE_STREAM		,	TYPE_SVI		},
	{"video/mpeg"		, 	"vob"		,	TYPE_STREAM		,	TYPE_MOVIE		},
	{"video/mpeg"		, 	"vro"		,	TYPE_STREAM		,	TYPE_MOVIE		},	/* add for DVD-RAM */
	{"video/quicktime"	,	"mov"		,	TYPE_STREAM		,	TYPE_MOVIE		},
	{"video/x-ms-wmv"	,	"wmv"		,	TYPE_STREAM		,	TYPE_MOVIE		},
	{"video/x-ms-wmx"	,	"asf"		,	TYPE_STREAM		,	TYPE_MOVIE		},
	{"audio/x-mpeg"		, 	"mp3"		,	TYPE_STREAM		,	TYPE_MUSIC		},
	{"audio/x-ogg"		, 	"ogg"		,	TYPE_STREAM		,	TYPE_MUSIC		},
	{"audio/x-mpeg"		, 	"mp4"		,	TYPE_STREAM		,	TYPE_MUSIC		},
	{"audio/x-ms-wma"	,	"wma"		, 	TYPE_STREAM		,	TYPE_MUSIC		},
	{"audio/x-wav"		,	"wav"		,	TYPE_STREAM		,	TYPE_MUSIC		},
	{"audio/ac3"		, 	"ac3"		,	TYPE_STREAM		,	TYPE_MUSIC		},
	{"audio/x-m4a"		, 	"m4a"		,	TYPE_STREAM		,	TYPE_MUSIC		},
	{"text/plain"		,	"plw"		,	TYPE_STREAM		,	TYPE_PLAYLIST	}, // Play List for wizd.
	{"text/plain"		,	"upl"		,	TYPE_STREAM		,	TYPE_PLAYLIST	}, // Uzu Play List��ĥ�ҤǤ�OK. �ե����뼫�Ȥθߴ���̵����
	{"text/plain"		,	"m3u"		,	TYPE_STREAM		,	TYPE_MUSICLIST	}, // m3u �Ǥ�OK?
	{"text/plain"		,	"tsv"		,	TYPE_STREAM		,	TYPE_PSEUDO_DIR	}, // tsv = ���ۥǥ��쥯�ȥ�
	{NULL, NULL, (-1), (-1) }
};



// ********************************************
// ��ĥ���Ѵ��ꥹ��
// ********************************************
// WARNING: hoge.m2p -> hoge.mpg �֤ǤϤʤ���
//          hoge.m2p -> hoge.m2p.mpg �ˤʤ롣
//          hoge.m2p.mpg -> hoge.m2p �ˤʤ롣
//          hoge.SVI.mpg -> hoge.SVI �ˤʤ롣
EXTENSION_CONVERT_LIST_T extension_convert_list[] = {
//	{org_extension	,	rename_extension	}
	{"m2p"			,	"mpg"			},
	{"svi"			,	"mpg"			},
	{"sv3"			,	"mpg"			},
	{"hnl"			,	"mpg"			},
	{"nuv"			,	"mpg"			},	/* add for MythTV */
	{ NULL, NULL }
};




// ********************************************
// ���Υѥ�᡼����¤�Τμ���
// ********************************************
GLOBAL_PARAM_T	global_param;




// IP�����������ĥꥹ��
ACCESS_CHECK_LIST_T	access_allow_list[ACCESS_ALLOW_LIST_MAX];

// User-Agent ���ĥꥹ��
ACCESS_USER_AGENT_LIST_T	allow_user_agent[ALLOW_USER_AGENT_LIST_MAX];

// �����ǥ��쥯�ȥ� �ꥹ��
SECRET_DIRECTORY_T secret_directory_list[SECRET_DIRECTORY_MAX];


// global_param�ɤ߹����ѥޥ���
#define SETCONF_FLAG(A,B) \
	if (strcasecmp(A, key) == 0) { \
		global_param.B = !strcasecmp(value, "true") ? TRUE : FALSE; \
	}
#define SETCONF_NUM(A,B) \
	if (strcasecmp(A, key) == 0) { global_param.B = atoi(value); }
#define SETCONF_STR(A,B) \
	if (strcasecmp(A, key) == 0) { \
		strncpy(global_param.B,value,sizeof(global_param.B)-1); \
	}
#define SETCONF_DIR(A,B) \
	if (strcasecmp(A, key) == 0) { \
		strncpy(global_param.B,value,sizeof(global_param.B)-1); \
		if (global_param.B[strlen(global_param.B) - 1] != '/') { \
			strcat(global_param.B, "/"); \
		} \
	}


// ********************************************
// ���Υѥ�᡼����¤�Τν������
// �ǥե�����ͤ򥻥å�
// ********************************************
void global_param_init(void)
{

	// ��¤�ΤޤȤ�ƽ����
	memset(&global_param, 0, sizeof(global_param));
	memset(access_allow_list, 0, sizeof(access_allow_list));
	memset(allow_user_agent, 0, sizeof(allow_user_agent));
	memset(secret_directory_list, 0, sizeof(secret_directory_list));


	// �ǡ���󲽥ե饰
	global_param.flag_daemon 		= DEFAULT_FLAG_DAEMON;

	// ��ư����
	global_param.flag_auto_detect 	= DEFAULT_FLAG_AUTO_DETECT;

	// �ǥե����Server̾��gethostname()���롣
	gethostname(global_param.server_name, sizeof(global_param.server_name));


	// �ǥե����HTTP �Ԥ�����Port.
	global_param.server_port 		= DEFAULT_SERVER_PORT;

	// Document Root
	strncpy(global_param.document_root, DEFAULT_DOCUMENT_ROOT, sizeof(global_param.document_root));

	// DebugLog
	global_param.flag_debug_log_output = DEFAULT_FLAG_DEBUG_LOG_OUTPUT;
	strncpy(global_param.debug_log_filename, DEFAULT_DEBUG_LOG_FILENAME, sizeof(global_param.debug_log_filename));

	// Client(MediaWiz)�θ��쥳����
	global_param.client_language_code = DEFAULT_CLIENT_LANGUAGE_CODE;

	// Server¦�θ��쥳����
	global_param.server_language_code = DEFAULT_SERVER_LANGUAGE_CODE;



	// �����������ѥե饰
	global_param.flag_use_skin 		= DEFAULT_FLAG_USE_SKIN;

	// �������֤���
	strncpy(global_param.skin_root, DEFAULT_SKINDATA_ROOT, sizeof(global_param.skin_root));

	// ������̾
	strncpy(global_param.skin_name, DEFAULT_SKINDATA_NAME, sizeof(global_param.skin_name));

	// �ե����륽���ȤΥ롼��
	global_param.sort_rule	= DEFAULT_SORT_RULE;

	// ���ڡ�����ɽ���������Կ�
	global_param.page_line_max	= DEFAULT_PAGE_LINE_MAX;

	// �ե�����̾ɽ���κ���Ĺ
	global_param.menu_filename_length_max = DEFAULT_MENU_FILENAME_LENGTH_MAX;

	// ��˥塼�ǤΥե���ȥ�����
	global_param.menu_font_metric = 14;

	// ��˥塼�ǤΥե���ȥ������ơ��֥�(ʸ����)
	strncpy(global_param.menu_font_metric_string, "", sizeof(global_param.menu_font_metric_string));

	//samba��CAP/HEX���󥳡��ɻ��Ѥ��뤫�ե饰
	global_param.flag_decode_samba_hex_and_cap = DEFAULT_FLAG_DECODE_SAMBA_HEX_AND_CAP;

	// wizd���Τ�ʤ���ĥ�ҤΥե�����򱣤����ݤ�
	global_param.flag_unknown_extention_file_hide = DEFAULT_FLAG_UNKNOWN_EXTENSION_FLAG_HIDE;

	// ɽ���ե�����̾���顢()[]�ǰϤޤ줿��ʬ�������뤫�ݤ���
	global_param.flag_filename_cut_parenthesis_area = DEFAULT_FLAG_FILENAME_CUT_PARENTHESIS_AREA;

	// ɽ���ե�����̾�ǡ��ƥǥ��쥯�ȥ�̾��Ʊ��ʸ����������뤫�ե饰
	global_param.flag_filename_cut_same_directory_name = DEFAULT_FLAG_FILENAME_CUT_SAME_DIRECTORY_NAME;

	// Allplay�Ǥ�ʸ�������ɻ�(�ե�����̾����Ⱦ���Ѵ�)���뤫�ե饰
	global_param.flag_allplay_filelist_adjust = DEFAULT_FLAG_ALLPLAY_FILELIST_ADJUST;


	//  SVI�ե������Ʊ���̾������ĥǥ��쥯�ȥ�򱣤����ե饰
	global_param.flag_hide_same_svi_name_directory	= DEFAULT_FLAG_HIDE_SAME_SVI_NAME_DIRECTORY;

	// SVI�ե��������ɽ����MAXĹ
	global_param.menu_svi_info_length_max = DEFAULT_MENU_SVI_INFO_LENGTH_MAX;

	// Windows�Ѥ˥ץ쥤�ꥹ����Υե�����̾��Ĵ�����뤫�ե饰
	global_param.flag_filename_adjustment_for_windows = DEFAULT_FLAG_FILENAME_ADJUSTMENT_FOR_WINDOWS;

	// CGI������ץȤμ¹Ԥ���Ĥ��뤫�ե饰
	global_param.flag_execute_cgi 		= DEFAULT_FLAG_EXECUTE_CGI;

	// CGI������ץȤ�ɸ�२�顼������
	strncpy(global_param.debug_cgi_output, DEFAULT_DEBUG_CGI_OUTPUT, sizeof(global_param.debug_cgi_output));

	// �ץ�������Ĥ��뤫�ե饰
	global_param.flag_allow_proxy 		= DEFAULT_FLAG_ALLOW_PROXY;

	// PC��Ƚ�Ǥ��� User-Agent
	strncpy(global_param.user_agent_pc, DEFAULT_USER_AGENT_PC, sizeof(global_param.user_agent_pc));

	// http�ѥ����
	global_param.http_passwd[0] = '\0';

	// mp3tag ���ɤफ�ɤ���
	global_param.flag_read_mp3_tag = TRUE;

	// v0.12f3
	global_param.flag_show_first_vob_only = DEFAULT_FLAG_SHOW_FIRST_VOB_ONLY;
	// v0.12f4
	global_param.flag_specific_dir_sort_type_fix = DEFAULT_FLAG_SPECIFIC_DIR_SORT_TYPE_FIX;

	return;
}



// ****************************************************
// ������ǥ��쥯�ȥ��wizd_skin.conf �ɤࡣ
// ̵����в��⤷�ʤ�
// ****************************************************
void skin_config_file_read(unsigned char *skin_conf_filename)
{
	int		fd;

	unsigned char	line_buf[1024*4];
	int	ret;
	unsigned char	key[1024];
	unsigned char	value[1024];


	fd = open(skin_conf_filename, O_RDONLY);
	if ( fd < 0 )
	{
		debug_log_output("skin config '%s' not found.\n", skin_conf_filename);
		return;
	}


	while ( 1 )
	{
		// �����ɤࡣ
		ret = config_file_read_line(fd, line_buf, sizeof(line_buf));
		if ( ret < 0 )
		{
			debug_log_output("EOF Detect.\n");
			break;
		}

		// �ɤ���Ԥ��������롣
		line_buffer_clearance(line_buf);


		if ( strlen(line_buf) > 0 )	// �ͤ����äƤ���
		{
			// ' '�ǡ������ʬ����
			sentence_split(line_buf, ' ', key, value);
			debug_log_output("key='%s', value='%s'\n", key, value);

			// ---------------------
			// �ͤ��ɤߤȤ�¹ԡ�
			// ---------------------
			if (( strlen(key) <= 0 ) || (strlen(value) <= 0 )) continue;

			SETCONF_NUM("page_line_max", page_line_max);
			SETCONF_NUM("menu_filename_length_max", menu_filename_length_max);
			SETCONF_NUM("menu_font_metric", menu_font_metric);
			SETCONF_STR("menu_font_metric_string", menu_font_metric_string);
			SETCONF_NUM("menu_svi_info_length_max", menu_svi_info_length_max);
		}
	}

	close( fd );

	return;
}


// ****************************************************
// wizd.conf �ɤ�
// ****************************************************
void config_file_read(void)
{
	int		fd;

	unsigned char	line_buf[1024*4];
	int	count_access_allow = 0;
	int	count_allow_user_agent = 0;
	int	count_secret_directory = 0;
	int	i;

	unsigned char	key[1024];
	unsigned char	value[1024];

	unsigned char	work1[32];
	unsigned char	work2[32];
	unsigned char	work3[32];
	unsigned char	work4[32];



	// =======================
	// Config�ե�����OPEN
	// =======================
	fd = config_file_open();
	if ( fd < 0 )
		return;


	// =====================
	// �����ɤ߹���
	// =====================
	while (config_file_read_line(fd, line_buf, sizeof(line_buf)) >= 0) {

		// �ɤ���Ԥ��������롣
		line_buffer_clearance(line_buf);

		// ���Ԥ��ä���continue
		if (strlen(line_buf) <= 0) continue;

		// ' '�ǡ������ʬ����
		sentence_split(line_buf, ' ', key, value);
		fprintf(stderr, "key='%s', value='%s'\n", key, value);

		// �ɤ��餫�����ʤ� continue
		if (strlen(key) <= 0 || strlen(value) <= 0) continue;

		// ---------------------
		// �ͤ��ɤߤȤ�¹ԡ�
		// ---------------------
		SETCONF_FLAG("flag_daemon", flag_daemon);
		SETCONF_FLAG("flag_auto_detect", flag_auto_detect);
		SETCONF_FLAG("flag_debug_log_output", flag_debug_log_output);
		SETCONF_STR("debug_log_filename", debug_log_filename);
		SETCONF_STR("exec_user", exec_user);
		SETCONF_STR("exec_group", exec_group);
		SETCONF_STR("auto_detect_bind_ip_address", auto_detect_bind_ip_address);
		SETCONF_STR("server_name", server_name);
		SETCONF_NUM("server_port", server_port);
		SETCONF_DIR("document_root", document_root);

		// client_language_code
		if (strcasecmp("client_language_code", key) == 0) {
			if (strcasecmp(value ,"sjis") == 0)
				global_param.client_language_code = CODE_SJIS;
			else if (strcasecmp(value ,"euc") == 0)
				global_param.client_language_code = CODE_EUC;
		}

		// server_language_code
		if (strcasecmp("server_language_code", key) == 0) {
			if (strcasecmp(value ,"auto") == 0)
				global_param.server_language_code = CODE_AUTO;
			else if (strcasecmp(value ,"sjis") == 0)
				global_param.server_language_code = CODE_SJIS;
			else if (strcasecmp(value ,"euc") == 0)
				global_param.server_language_code = CODE_EUC;
			else if (!strcasecmp(value ,"utf8") || !strcasecmp(value ,"utf-8"))
				global_param.server_language_code = CODE_UTF8;
			else if (!strcasecmp(value ,"utf16") || !strcasecmp(value ,"utf-16"))
				global_param.server_language_code = CODE_UTF16;
		}


		SETCONF_FLAG("flag_use_skin", flag_use_skin);
		SETCONF_DIR("skin_root", skin_root);
		SETCONF_DIR("skin_name", skin_name);

		// sort_rule
		if ( strcasecmp("sort_rule", key) == 0 )
		{
			if (strcasecmp(value ,"none") == 0 )
				global_param.sort_rule = SORT_NONE;
			else if (strcasecmp(value ,"name_up") == 0 )
				global_param.sort_rule = SORT_NAME_UP;
			else if (strcasecmp(value ,"name_down") == 0 )
				global_param.sort_rule = SORT_NAME_DOWN;
			else if (strcasecmp(value ,"time_up") == 0 )
				global_param.sort_rule = SORT_TIME_UP;
			else if (strcasecmp(value ,"time_down") == 0 )
				global_param.sort_rule = SORT_TIME_DOWN;
			else if (strcasecmp(value ,"size_up") == 0 )
				global_param.sort_rule = SORT_SIZE_UP;
			else if (strcasecmp(value ,"size_down") == 0 )
				global_param.sort_rule = SORT_SIZE_DOWN;
		}

		SETCONF_NUM("page_line_max", page_line_max);
		SETCONF_NUM("menu_filename_length_max", menu_filename_length_max);

		SETCONF_FLAG("flag_hide_same_svi_name_directory", flag_hide_same_svi_name_directory);
		SETCONF_NUM("menu_svi_info_length_max", menu_svi_info_length_max);
		SETCONF_FLAG("flag_decode_samba_hex_and_cap", flag_decode_samba_hex_and_cap);
		SETCONF_FLAG("flag_unknown_extention_file_hide", flag_unknown_extention_file_hide);
		SETCONF_FLAG("flag_filename_cut_parenthesis_area", flag_filename_cut_parenthesis_area);
		SETCONF_FLAG("flag_filename_cut_same_directory_name", flag_filename_cut_same_directory_name);
		SETCONF_FLAG("flag_allplay_filelist_adjust", flag_allplay_filelist_adjust);

		SETCONF_NUM("buffer_size", buffer_size);
		SETCONF_FLAG("flag_buffer_send_asap", flag_buffer_send_asap);
		SETCONF_STR("user_agent_proxy_override", user_agent_proxy_override);
		SETCONF_STR("user_agent_pc", user_agent_pc);
		SETCONF_NUM("max_child_count", max_child_count);
		SETCONF_FLAG("flag_execute_cgi", flag_execute_cgi);
		SETCONF_STR("debug_cgi_output", debug_cgi_output);
		SETCONF_FLAG("flag_allow_proxy", flag_allow_proxy);
		SETCONF_STR("http_passwd", http_passwd);
		SETCONF_FLAG("flag_read_mp3_tag", flag_read_mp3_tag);
		SETCONF_DIR("wizd_chdir", wizd_chdir);


		// access_allow
		if ( strcasecmp("access_allow", key) == 0 )
		{
			if (count_access_allow < ACCESS_ALLOW_LIST_MAX )
			{
				// value��'/'��ʬ��
				sentence_split(value, '/', work1, work2);

				access_allow_list[count_access_allow].flag = TRUE;

				// adddressʸ�����'.'��ʬ�䤷�����줾���atoi()
				strncat(work1, ".", sizeof(work1) - strlen(work1) ); // ʬ������Τ��ᡢ�Ǹ��"."��­���Ƥ���
				for (i=0; i<4; i++ )
				{
					sentence_split(work1, '.', work3, work4);
					access_allow_list[count_access_allow].address[i] = (unsigned char)atoi(work3);
					strncpy(work1, work4, sizeof(work1));
				}


				// netmaskʸ�����'.'��ʬ�䤷�����줾���atoi()
				strncat(work2, ".", sizeof(work2) - strlen(work2) ); // ʬ������Τ��ᡢ�Ǹ��"."��­���Ƥ���
				for (i=0; i<4; i++ )
				{
					sentence_split(work2, '.', work3, work4);
					access_allow_list[count_access_allow].netmask[i] = (unsigned char)atoi(work3);
					strncpy(work2, work4, sizeof(work1));
				}

				fprintf(stderr, "[%d] address=[%d.%d.%d.%d/%d.%d.%d.%d]\n",count_access_allow,
																access_allow_list[count_access_allow].address[0],
																access_allow_list[count_access_allow].address[1],
																access_allow_list[count_access_allow].address[2],
																access_allow_list[count_access_allow].address[3],
																access_allow_list[count_access_allow].netmask[0],
																access_allow_list[count_access_allow].netmask[1],
																access_allow_list[count_access_allow].netmask[2],
																access_allow_list[count_access_allow].netmask[3]	);

				// address��netmask�� and �黻�����㤦��
				for ( i=0; i<4; i++ )
				{
					access_allow_list[count_access_allow].address[i] &= access_allow_list[count_access_allow].netmask[i];
				}

				count_access_allow++;
			}
		}


		// allow_user_agent
		if ( strcasecmp("allow_user_agent", key) == 0 )
		{
			if (count_allow_user_agent < ALLOW_USER_AGENT_LIST_MAX )
			{
				strncpy(allow_user_agent[count_allow_user_agent].user_agent, value, sizeof(allow_user_agent[count_allow_user_agent].user_agent) );
				fprintf(stderr, "[%d] allow_user_agent='%s'\n", count_allow_user_agent, allow_user_agent[count_allow_user_agent].user_agent);

				count_allow_user_agent++;
			}
		}

		// secret_directory_list
		if ( strcasecmp("secret_directory", key) == 0 )
		{
			if (count_secret_directory < SECRET_DIRECTORY_MAX )
			{
				// value��' '��ʬ��
				sentence_split(value, ' ', work1, work2);

				strncpy(secret_directory_list[count_secret_directory].dir_name, work1, sizeof(secret_directory_list[count_secret_directory].dir_name) );
				secret_directory_list[count_secret_directory].tvid = atoi(work2);

				printf("[%d] secret_dir='%s', tvid=%d\n", count_secret_directory, secret_directory_list[count_secret_directory].dir_name, secret_directory_list[count_secret_directory].tvid);

				count_secret_directory++;
			}
		}


		SETCONF_FLAG("flag_filename_adjustment_for_windows", flag_filename_adjustment_for_windows);
		SETCONF_FLAG("flag_show_first_vob_only", flag_show_first_vob_only);
		SETCONF_FLAG("flag_specific_dir_sort_type_fix", flag_specific_dir_sort_type_fix);
		SETCONF_FLAG("flag_resize_jpeg", flag_resize_jpeg);
		SETCONF_NUM("target_jpeg_width", target_jpeg_width);
		SETCONF_NUM("target_jpeg_height", target_jpeg_height);
		if (strcasecmp("widen_ratio", key) == 0) {
			global_param.widen_ratio = atof(value);
		}
	}
	fprintf(stderr, "EOF Detect.\n");

	close( fd );


	return;
}










// *****************************************************
// wizd.conf ���飱���ɤ߹���
// �ɤ߹����ʸ������return����롣
// �Ǹ�ޤ��ɤ���顢-1����롣
// *****************************************************
static int config_file_read_line( int fd, unsigned char *line_buf, int line_buf_size)
{
	int read_len;
	int	total_read_len;
	unsigned char	read_char;
	unsigned char *p;

	p = line_buf;
	total_read_len = 0;

	while ( 1 )
	{
		// ��ʸ��read.
		read_len  = read(fd, &read_char, 1);
		if ( read_len <= 0 ) // EOF����
		{
			return ( -1 );
		}
		else if ( read_char == '\r' )
		{
			continue;
		}
		else if ( read_char == '\n' )
		{
			break;
		}

		*p = read_char;
		p++;
		total_read_len++;

		if ( total_read_len >= line_buf_size )
		{
			break;
		}
	}

	*p = '\0';
	return total_read_len;
}


// ****************************************************
// wizd_conf �򳫤�
// �����ʤ��ä��� -1
// ****************************************************
static int config_file_open(void)
{
	int		fd;
	int		i;

	for (i=0; i<sizeof(config_file)/sizeof(char*); i++) {
		fd = open(config_file[i], O_RDONLY);
		if (fd >= 0) {
			fprintf(stderr, "config '%s' open.\n", config_file[i]);
			return fd;
		}
	}

	fprintf(stderr, "No config file is open. (use default settings at all)\n");
	return -1;
}


// ****************************************************
// �ɤ���Ԥ��������롣
// ****************************************************
static void line_buffer_clearance(unsigned char *line_buf)
{

	// '#'����������
	cut_after_character(line_buf, '#');

	// '\t'��' '���ִ�
	replase_character(line_buf, sizeof(line_buf), "\t", " ");

	// ' '���ŤʤäƤ���Ȥ������
	duplex_character_to_unique(line_buf, ' ');

	// Ƭ��' '������������
	cut_first_character(line_buf, ' ');

	// �Ǹ�� ' '������������
	cut_character_at_linetail(line_buf, ' ');

	return;
}


MIME_LIST_T *lookup_mime_by_ext(char *file_extension)
{
	int i;

	if (file_extension == NULL || strlen(file_extension) == 0) return NULL;

	for (i=0; mime_list[i].mime_name != NULL; i++) {
		if ( strcasecmp(mime_list[i].file_extension, file_extension) == 0 ) {
			return &mime_list[i];
		}
	}

	return NULL;
}


//========================================================
// ��ĥ�Ҥ��Ϥ��ȡ�Content-type �ȡ�file_type���֤���
//========================================================
void check_file_extension_to_mime_type(const unsigned char *file_extension, unsigned char *mime_type, int mime_type_size )
{
	int		i;

	strncpy(mime_type, DEFAULT_MIME_TYPE, mime_type_size);

	debug_log_output("file_extension='%s'\n", file_extension);


	// -------------------------------------------
	// �ե�����γ�ĥ����ӡ�Content-type �����
	// -------------------------------------------
	for (i=0;;i++)
	{
		if ( mime_list[i].mime_name == NULL )
			break;

		if ( strcasecmp(mime_list[i].file_extension, file_extension) == 0 )
		{
			strncpy(mime_type, mime_list[i].mime_name, mime_type_size);
			break;
		}
	}
	debug_log_output("mime_type='%s'\n", mime_type);

	return;
}

void config_sanity_check()
{
	struct stat sb;
	char cwd[WIZD_FILENAME_MAX];
	char buf[WIZD_FILENAME_MAX];

	if (global_param.document_root[0] != '/') {
		if (getcwd(cwd, sizeof(cwd)) == NULL) {
			debug_log_output("document_root: getcwd(): %s", strerror(errno));
			exit(-1);
		}
		snprintf(buf, sizeof(buf), "%s/%s", cwd, global_param.document_root);
		strncpy(global_param.document_root, buf, sizeof(global_param.document_root));
		debug_log_output("concatenated document_root: '%s'", global_param.document_root);
	}
	if (path_sanitize(global_param.document_root, sizeof(global_param.document_root)) == NULL) {
		debug_log_output("WARNING! weird path has been specified.");
		debug_log_output("falling back to the default document root.");
		strncpy(global_param.document_root, DEFAULT_DOCUMENT_ROOT
			, sizeof(global_param.document_root));
	}
	if (stat(global_param.document_root, &sb) != 0) {
		debug_log_output("document_root: %s: %s", global_param.document_root, strerror(errno));
		exit(-1);
	}
	if (!S_ISDIR(sb.st_mode)) {
		debug_log_output("document_root: %s: is not a directory.", global_param.document_root);
		exit(-1);
	}
	debug_log_output("document_root: '%s'", global_param.document_root);
}
