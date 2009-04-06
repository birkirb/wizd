//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_menu.c
//											$Revision: 1.53 $
//											$Date: 2004/12/18 15:24:50 $
//
//	���٤Ƽ�����Ǥ�Ǥ��ʤ������ޤġ�
//  ���Υ��եȤˤĤ���VertexLink���䤤��碌�ʤ��Ǥ���������
// ==========================================================================
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>


#include "wizd.h"
#include "wizd_aviread.h"

#define NEED_SKIN_MAPPING_DEFINITION
#include "wizd_skin.h"


#define	SORT_FILE_MASK			( 0x000000FF )
#define	SORT_DIR_MASK			( 0x00000F00 )
#define	SORT_DIR_FLAG(_A_)		( ( _A_ & SORT_DIR_MASK ) >> 8 )
#define	SORT_DIR_UP				( 0x00000100 )
#define	SORT_DIR_DOWN			( 0x00000200 )
#define	DEFAULT_SORT_RULE		SORT_NONE






typedef struct {
	unsigned char	name[WIZD_FILENAME_MAX];		// ɽ���ѥե�����̾(EUC)
	unsigned char	org_name[WIZD_FILENAME_MAX];	// ���ꥸ�ʥ�ե�����̾
	unsigned char	ext[32];						// ���ꥸ�ʥ�ե������ĥ��
	mode_t			type;			// ����
	off_t			size;			// ������
	// use off_t instead of size_t, since it contains st_size of stat.
	time_t			time;			// ����
} FILE_INFO_T;





#define		FILEMENU_BUF_SIZE	(1024*16)


static int count_file_num(unsigned char *path);
static int directory_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num);
static int count_file_num_in_tsv(unsigned char *path);
static int tsv_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num);

static int file_ignoral_check(unsigned char *name, unsigned char *path);
static int directory_same_check_svi_name(unsigned char *name);

static void send_skin_filemenu(int accept_socket, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p, int lines);

//static void create_system_filemenu(HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, unsigned char *send_filemenu_buf, int buf_size);
static void create_skin_filemenu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num);
static void create_all_play_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num);
static char* create_1line_playlist(HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p);


static void  mp3_id3_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static void  mp3_id3v1_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static int  mp3_id3v2_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static void generate_mp3_title_info( SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static void playlist_filename_adjustment(unsigned char *src_name, unsigned char *dist_name, int dist_size, int input_code );

static int file_read_line( int fd, unsigned char *line_buf, int line_buf_size);

static void filename_adjustment_for_windows(unsigned char *filename, const unsigned char *pathname_plw);

static int read_avi_info(char *fname, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p);

static void 	file_info_sort( FILE_INFO_T *p, int num, unsigned long type );
static int 		_file_info_dir_sort( const void *in_a, const void *in_b, int order );
static int	 	_file_info_dir_sort_order_up( const void *in_a, const void *in_b );
static int 		_file_info_dir_sort_order_down( const void *in_a, const void *in_b );
static int		_file_info_name_sort( const void *in_a, const void *in_b, int order );
static int 		_file_info_name_sort_order_up( const void *in_a, const void *in_b );
static int 		_file_info_name_sort_order_down( const void *in_a, const void *in_b );
static int 		_file_info_size_sort( const void *in_a, const void *in_b, int order );
static int 		_file_info_size_sort_order_up( const void *in_a, const void *in_b );
static int 		_file_info_size_sort_order_down( const void *in_a, const void *in_b );
static int 		_file_info_time_sort( const void *in_a, const void *in_b, int order );
static int 		_file_info_time_sort_order_up( const void *in_a, const void *in_b );
static int 		_file_info_time_sort_order_down( const void *in_a, const void *in_b );
static int 		_file_info_shuffle( const void *in_a, const void *in_b );


// �ե����륽�����Ѵؿ�����
static void * file_sort_api[] = {
	NULL,
	_file_info_name_sort_order_up,
	_file_info_name_sort_order_down,
	_file_info_time_sort_order_up,
	_file_info_time_sort_order_down,
	_file_info_size_sort_order_up,
	_file_info_size_sort_order_down,
	_file_info_shuffle,
};


// �ǥ��쥯�ȥ꥽�����Ѵؿ�����
static void * dir_sort_api[] = {
	NULL,
	_file_info_dir_sort_order_up,
	_file_info_dir_sort_order_down
};




// **************************************************************************
// �ե�����ꥹ�Ȥ����������ֿ�
// **************************************************************************
void http_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, int flag_pseudo)
{
	int				file_num;	// DIR��Υե������
	int				sort_rule;  // temp�֤�������
	unsigned char	*file_info_malloc_p;
	FILE_INFO_T		*file_info_p;

	if (!flag_pseudo) {
		// recv_uri �κǸ夬'/'�Ǥʤ��ä��顢'/'���ɲ�
		if (( strlen(http_recv_info_p->recv_uri) > 0 ) &&
			( http_recv_info_p->recv_uri[strlen(http_recv_info_p->recv_uri)-1] != '/' ))
		{
			strncat(http_recv_info_p->recv_uri, "/", sizeof(http_recv_info_p->recv_uri) - strlen(http_recv_info_p->recv_uri) );
		}


		//  http_recv_info_p->send_filename �κǸ夬'/'�Ǥʤ��ä��顢'/'���ɲ�
		if (( strlen(http_recv_info_p->send_filename) > 0 ) &&
			( http_recv_info_p->send_filename[strlen(http_recv_info_p->send_filename)-1] != '/' ))
		{
			strncat(http_recv_info_p->send_filename, "/", sizeof(http_recv_info_p->send_filename) - strlen(http_recv_info_p->send_filename) );
		}
	}


	// ==================================
	// �ǥ��쥯�ȥ�����ǣţ�
	// ==================================

	if (flag_pseudo) {
		// recv_uri �ǥ��쥯�ȥ�Υե������������롣
		file_num = count_file_num_in_tsv( http_recv_info_p->send_filename );
	} else {
		// recv_uri �ǥ��쥯�ȥ�Υե������������롣
		file_num = count_file_num( http_recv_info_p->send_filename );
	}

	debug_log_output("file_num = %d", file_num);
	if ( file_num < 0 )
	{
		return;
	}


	// ɬ�פʿ��������ե����������¸���ꥢ��malloc()
	file_info_malloc_p = malloc( sizeof(FILE_INFO_T)*file_num );
	if ( file_info_malloc_p == NULL )
	{
		debug_log_output("malloc() error");
		return;
	}


	memset(file_info_malloc_p, 0, sizeof(FILE_INFO_T)*file_num);
	file_info_p = (FILE_INFO_T *)file_info_malloc_p;



	// -----------------------------------------------------
	// �ե����������¸���ꥢ�ˡ��ǥ��쥯�ȥ������ɤ߹��ࡣ
	// -----------------------------------------------------
	if (flag_pseudo) {
		file_num = tsv_stat(http_recv_info_p->send_filename, file_info_p, file_num);
	} else {
		file_num = directory_stat(http_recv_info_p->send_filename, file_info_p, file_num);
	}
	debug_log_output("file_num = %d", file_num);


	// �ǥХå���file_info_malloc_p ɽ��
	//for ( i=0; i<file_num; i++ )
	//{
	//	debug_log_output("file_info[%d] name='%s'", i, file_info_p[i].name );
	//	debug_log_output("file_info[%d] size='%d'", i, file_info_p[i].size );
	//	debug_log_output("file_info[%d] time='%d'", i, file_info_p[i].time );
	//}


	// ---------------------------------------------------
	// sort=�ǥ����Ȥ��ؼ�����Ƥ��뤫��ǧ
	// �ؼ�����Ƥ����顢�����global_param����
	// ---------------------------------------------------
	if ( strlen(http_recv_info_p->sort) > 0 )
	{
		if (strcasecmp(http_recv_info_p->sort ,"none") == 0 )
			global_param.sort_rule = SORT_NONE;
		else if (strcasecmp(http_recv_info_p->sort ,"name_up") == 0 )
			global_param.sort_rule = SORT_NAME_UP;
		else if (strcasecmp(http_recv_info_p->sort ,"name_down") == 0 )
			global_param.sort_rule = SORT_NAME_DOWN;
		else if (strcasecmp(http_recv_info_p->sort ,"time_up") == 0 )
			global_param.sort_rule = SORT_TIME_UP;
		else if (strcasecmp(http_recv_info_p->sort ,"time_down") == 0 )
			global_param.sort_rule = SORT_TIME_DOWN;
		else if (strcasecmp(http_recv_info_p->sort ,"size_up") == 0 )
			global_param.sort_rule = SORT_SIZE_UP;
		else if (strcasecmp(http_recv_info_p->sort ,"size_down") == 0 )
			global_param.sort_rule = SORT_SIZE_DOWN;
		else if (strcasecmp(http_recv_info_p->sort ,"shuffle") == 0 )
			global_param.sort_rule = SORT_SHUFFLE;
	} else if (flag_pseudo) global_param.sort_rule = SORT_NONE;

	sort_rule = global_param.sort_rule;

	// 0.12f4
	if (global_param.flag_specific_dir_sort_type_fix == TRUE) {
	// ����ǥ��쥯�ȥ�Ǥ�SORT��ˡ�����
		unsigned char	*p;
		if ((p=strstr(http_recv_info_p->send_filename,"/video_ts/"))!=NULL) {
			if (*(p+strlen("/video_ts/"))=='\0')	//����?
				sort_rule = SORT_NAME_UP;
		}
		if ((p=strstr(http_recv_info_p->send_filename,"/TIMESHIFT/"))!=NULL) {
			if (*(p+strlen("/TIMESHIFT/"))=='\0')	//����?
				sort_rule = SORT_TIME_UP;
		}
		//if (strcmp(http_recv_info_p->send_filename,"video")==0)
		//	sort_rule = SORT_TIME_DOWN;
		// �����ƥ����ȡ�ǯ������Х�̾���ȥ�å�+Sort���饤�ƥꥢ
		//
	}

	// ɬ�פʤ�С������ȼ¹�
	if ( sort_rule != SORT_NONE )
	{
		file_info_sort( file_info_p, file_num, sort_rule | SORT_DIR_UP );
	}

	// -------------------------------------------
	// ��ư����
	// -------------------------------------------
	if ( strcasecmp(http_recv_info_p->action, "allplay") == 0 )
	{
		create_all_play_list(accept_socket, http_recv_info_p, file_info_p, file_num);
		debug_log_output("AllPlay List Create End!!! ");
		return ;
	}


	// -------------------------------------------
	// �ե������˥塼����������
	// -------------------------------------------
	create_skin_filemenu(accept_socket, http_recv_info_p, file_info_p, file_num);

	free(file_info_malloc_p);

	return;
}





// ****************************************************************************************
// ��˥塼�����Ѥ�define������
// ****************************************************************************************



// **************************************************************************
// ���������Ѥ����ե������˥塼������
// **************************************************************************
static void create_skin_filemenu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num)
{
	int		i;

	unsigned char	work_filename[WIZD_FILENAME_MAX];

	unsigned char	work_data[WIZD_FILENAME_MAX];
	unsigned char	work_data2[WIZD_FILENAME_MAX];

	int		now_page;		// ���ߤΥڡ����ֹ�
	int		max_page;		// ����ڡ����ֹ�
	int		now_page_line;	// ���ߤΥڡ�����ɽ���Կ�
	int		start_file_num;	// ���ߥڡ�����ɽ�����ϥե������ֹ�
	int		end_file_num;	// ���ߥڡ�����ɽ����λ�ե������ֹ�

	int		next_page;		// ���Υڡ���(̵�����max_page)
	int		prev_page;		// ������Υڡ�����̵����� 1)


	SKIN_REPLASE_GLOBAL_DATA_T	*skin_rep_data_global_p;
	SKIN_REPLASE_LINE_DATA_T	*skin_rep_data_line_p;

	int skin_rep_line_malloc_size;

	unsigned int	rec_time;
	int	count;

	unsigned int	image_width, image_height;

	struct	stat	dir_stat;
	int				result;

	// ==========================================
	// SKIN����ե����ե������ɤ߹���
	// ==========================================

	skin_read_config(SKIN_MENU_CONF);


	// ==========================================
	// HTML�������� �Ƽ�׻���
	// ==========================================

	// �ǥ��쥯�ȥ�¸�ߥե������
	debug_log_output("file_num = %d", file_num);

	// ����ڡ������׻�
	if ( file_num == 0 )
	{
		max_page = 1;
	}
	else if ( (file_num % global_param.page_line_max) == 0 )
	{
		max_page = (file_num / global_param.page_line_max);
	}
	else
	{
		max_page = (file_num / global_param.page_line_max) + 1;
	}
	debug_log_output("max_page = %d", max_page);

	// ����ɽ���ڡ����ֹ� �׻���
	if ( (http_recv_info_p->page <= 1 ) || (max_page < http_recv_info_p->page ) )
		now_page = 1;
	else
		now_page = http_recv_info_p->page;

	debug_log_output("now_page = %d", now_page);

	// ����ɽ���ڡ�����ɽ���Կ��׻���
	if ( max_page == now_page ) // �Ǹ�Υڡ���
		now_page_line = file_num - (global_param.page_line_max * (max_page-1));
	else	// �Ǹ�ʳ��ʤ顢ɽ���������
		now_page_line = global_param.page_line_max;
	debug_log_output("now_page_line = %d", now_page_line);


	// ɽ�����ϥե������ֹ�׻�
	start_file_num = ((now_page - 1) * global_param.page_line_max);
	debug_log_output("start_file_num = %d", start_file_num);

	if ( max_page == now_page ) // �Ǹ�Υڡ���
		end_file_num = file_num;
	else // �Ǹ�Υڡ����ǤϤʤ��ä��顣
		end_file_num = (start_file_num + global_param.page_line_max);
	debug_log_output("start_file_num = %d", start_file_num);




	// ���ڡ����ֹ� �׻�
	prev_page =  1 ;
	if ( now_page > 1 )
		prev_page = now_page - 1;

	// ���ڡ����ֹ� �׻�
	next_page = max_page ;
	if ( max_page > now_page )
		next_page = now_page + 1;

	debug_log_output("prev_page=%d  next_page=%d", prev_page ,next_page);



	// ===============================
	// �������ִ��ѥǡ��������
	// ===============================

	// ��ȥ��ꥢ����
	skin_rep_data_global_p 	= malloc( sizeof(SKIN_REPLASE_GLOBAL_DATA_T) );
	if ( skin_rep_data_global_p == NULL )
	{
		debug_log_output("malloc() error.");
		return ;
	}
	memset(skin_rep_data_global_p, '\0', sizeof(SKIN_REPLASE_GLOBAL_DATA_T));

	skin_rep_line_malloc_size = sizeof(SKIN_REPLASE_LINE_DATA_T) * (global_param.page_line_max + 1);
	skin_rep_data_line_p 	= malloc( skin_rep_line_malloc_size );
	if ( skin_rep_data_line_p == NULL )
	{
		debug_log_output("malloc() error.");
		return ;
	}
	memset(skin_rep_data_line_p, '\0', sizeof(skin_rep_line_malloc_size));


	// ---------------------------------
	// �����Х� ɽ���Ѿ��� ��������
	// ---------------------------------

	// ���ѥ�̾ ɽ��������(recv_uri -> current_path_name [client������])
	convert_language_code(	http_recv_info_p->recv_uri,
							skin_rep_data_global_p->current_path_name,
							sizeof(skin_rep_data_global_p->current_path_name),
							global_param.server_language_code | CODE_HEX,
							global_param.client_language_code );
	debug_log_output("current_path = '%s'", skin_rep_data_global_p->current_path_name );

	// -----------------------------------------------
	// �ƥǥ��쥯�ȥ��Ѥ�����
	// -----------------------------------------------
	if (get_parent_path(work_data, http_recv_info_p->recv_uri, sizeof(work_data)) == NULL) {
		debug_log_output("FATAL ERROR! too long recv_uri.");
		return ;
	}
	debug_log_output("parent_directory='%s'", work_data);

	// ��ľ�Υǥ��쥯�ȥ�ѥ���URI���󥳡��ɡ�
	uri_encode(skin_rep_data_global_p->parent_directory_link, sizeof(skin_rep_data_global_p->parent_directory_link), work_data, strlen(work_data));
	// '?'���ɲ�
	strncat(skin_rep_data_global_p->parent_directory_link, "?", sizeof(skin_rep_data_global_p->parent_directory_link) - strlen(skin_rep_data_global_p->parent_directory_link));
	// sort=���ؼ�����Ƥ�����硢���������Ѥ���
	if ( strlen(http_recv_info_p->sort) > 0 ) {
		snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
		strncat(skin_rep_data_global_p->parent_directory_link, work_data, sizeof(skin_rep_data_global_p->parent_directory_link) - strlen(skin_rep_data_global_p->parent_directory_link));
	}
	debug_log_output("parent_directory_link='%s'", skin_rep_data_global_p->parent_directory_link);

	// �ƥǥ��쥯�ȥ�̾ ɽ����
	convert_language_code(	work_data,
							skin_rep_data_global_p->parent_directory_name,
							sizeof(skin_rep_data_global_p->parent_directory_name),
							global_param.server_language_code | CODE_HEX,
							global_param.client_language_code );
	debug_log_output("parent_directory_name='%s'", skin_rep_data_global_p->parent_directory_link);


	// -----------------------------------------------
	// ���˸��ǥ��쥯�ȥ��Ѥ�����
	// -----------------------------------------------

	// ���ǥ��쥯�ȥ�̾ ���������ޤ���EUC���Ѵ���
	convert_language_code(	http_recv_info_p->recv_uri,
							work_data,
							sizeof(work_data),
							global_param.server_language_code | CODE_HEX,
							CODE_EUC );

	// �Ǹ��'/'���դ��Ƥ�������
	cut_character_at_linetail(work_data, '/');
	// '/'���������
	cut_before_last_character(work_data, '/');
	// '/'���ɲá�
	strncat(work_data, "/", sizeof(work_data) - strlen(work_data) );

	// CUT�¹�
	euc_string_cut_n_length(work_data, global_param.menu_filename_length_max);

	// ���ǥ��쥯�ȥ�̾ ɽ��������(ʸ���������Ѵ�)
	convert_language_code(	work_data,
							skin_rep_data_global_p->current_directory_name,
							sizeof(skin_rep_data_global_p->current_directory_name),
							CODE_EUC,
							global_param.client_language_code );

	debug_log_output("current_dir = '%s'", skin_rep_data_global_p->current_directory_name );


	// ���ѥ�̾ Link��������URI���󥳡��� from recv_uri��
	uri_encode(skin_rep_data_global_p->current_directory_link_no_param
		, sizeof(skin_rep_data_global_p->current_directory_link_no_param)
		, http_recv_info_p->recv_uri
		, strlen(http_recv_info_p->recv_uri)
	);
	strncpy(skin_rep_data_global_p->current_directory_link
		, skin_rep_data_global_p->current_directory_link_no_param
		, sizeof(skin_rep_data_global_p->current_directory_link)
	);

	// ?���ɲ�
	strncat(skin_rep_data_global_p->current_directory_link, "?", sizeof(skin_rep_data_global_p->current_directory_link) - strlen(skin_rep_data_global_p->current_directory_link)); // '?'���ɲ�
	// sort=���ؼ�����Ƥ�����硢���������Ѥ���
	if ( strlen(http_recv_info_p->sort) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
		strncat(skin_rep_data_global_p->current_directory_link, work_data, sizeof(skin_rep_data_global_p->current_directory_link) - strlen(skin_rep_data_global_p->current_directory_link));
	}
	debug_log_output("current_directory_link='%s'", skin_rep_data_global_p->current_directory_link);

	// ------------------------------------ �ǥ��쥯�ȥ�ɽ���ѽ��� �����ޤ�

	// �ǥ��쥯�ȥ�¸�ߥե������ ɽ����
	snprintf(skin_rep_data_global_p->file_num_str, sizeof(skin_rep_data_global_p->file_num_str), "%d", file_num );

	// 	���ߤΥڡ��� ɽ����
	snprintf(skin_rep_data_global_p->now_page_str, sizeof(skin_rep_data_global_p->now_page_str), "%d", now_page );

	// ���ڡ����� ɽ����
	snprintf(skin_rep_data_global_p->max_page_str, sizeof(skin_rep_data_global_p->max_page_str), "%d", max_page );

	// ���Υڡ��� ɽ����
	snprintf(skin_rep_data_global_p->next_page_str, sizeof(skin_rep_data_global_p->next_page_str), "%d", next_page );

	// ���Υڡ��� ɽ����
	snprintf(skin_rep_data_global_p->prev_page_str, sizeof(skin_rep_data_global_p->prev_page_str), "%d", prev_page );

	// ���ϥե������ֹ�ɽ����
	if ( file_num == 0 )
		snprintf(skin_rep_data_global_p->start_file_num_str, sizeof(skin_rep_data_global_p->start_file_num_str), "%d", start_file_num );
	else
		snprintf(skin_rep_data_global_p->start_file_num_str, sizeof(skin_rep_data_global_p->start_file_num_str), "%d", start_file_num +1 );

	// ��λ�ե������ֹ�ɽ����
	snprintf(skin_rep_data_global_p->end_file_num_str, sizeof(skin_rep_data_global_p->end_file_num_str), "%d", end_file_num  );


	skin_rep_data_global_p->stream_files = 0;	// ������ǽ�ե���������������

	// PC���ɤ����򥹥����ִ�������ɲ�
	skin_rep_data_global_p->flag_pc = http_recv_info_p->flag_pc;

	// BODY������ onloadset="$focus"
	if (http_recv_info_p->focus[0]) {
		// �����Τ��� �ޤ� uri_encode ���롣
		// (to prevent, $focus = "start\"><a href=\"...."; hack. ;p)
		uri_encode(work_data, sizeof(work_data)
			, http_recv_info_p->focus, strlen(http_recv_info_p->focus));
		snprintf(skin_rep_data_global_p->focus
			, sizeof(skin_rep_data_global_p->focus)
			, " onloadset=\"%s\"", work_data);

	} else {
		skin_rep_data_global_p->focus[0] = '\0'; /* nothing :) */
	}

	// ---------------------------------
	// �ե�����ɽ���Ѿ��� ��������
	// ---------------------------------
	for ( i=start_file_num, count=0; i<(start_file_num + now_page_line) ; i++, count++ )
	{
		debug_log_output("-----< file info generate, count = %d >-----", count);

		// file_info_p[i].name �� EUC (�ѹ� wizd 0.12h)

		// Ĺ�����¤�Ķ���Ƥ�����Cut
		strncpy(work_data, file_info_p[i].name, sizeof(work_data));
		euc_string_cut_n_length(work_data, global_param.menu_filename_length_max);
		debug_log_output("file_name(cut)='%s'\n", work_data);

		// MediaWizʸ�������ɤ�
		convert_language_code(	work_data,
								skin_rep_data_line_p[count].file_name_no_ext,
								sizeof(skin_rep_data_line_p[count].file_name_no_ext),
								CODE_EUC,
								global_param.client_language_code);

		debug_log_output("file_name_no_ext='%s'\n", skin_rep_data_line_p[count].file_name_no_ext);

		// --------------------------------------
		// �ե�����̾(��ĥ��̵��) ������λ
		// --------------------------------------

		// --------------------------------------------------------------------------------
		// ��ĥ�Ҥ�������
		// --------------------------------------------------------------------------------
		strncpy(skin_rep_data_line_p[count].file_extension, file_info_p[i].ext
			, sizeof(skin_rep_data_line_p[count].file_extension));
		debug_log_output("file_extension='%s'\n", skin_rep_data_line_p[count].file_extension);

		// --------------------------------------------------------------------------------
		// �ե�����̾ ���� (ɽ����)  (no_ext��ext�򤯤äĤ���)
		// --------------------------------------------------------------------------------
		strncpy(skin_rep_data_line_p[count].file_name, skin_rep_data_line_p[count].file_name_no_ext, sizeof(skin_rep_data_line_p[count].file_name));
		if ( strlen(skin_rep_data_line_p[count].file_extension) > 0 ) {
			strncat(skin_rep_data_line_p[count].file_name, ".", sizeof(skin_rep_data_line_p[count].file_name));
			strncat(skin_rep_data_line_p[count].file_name, skin_rep_data_line_p[count].file_extension, sizeof(skin_rep_data_line_p[count].file_name));
		}

		// --------------------------------------------------------------------------------
		// Link��URI(���󥳡��ɺѤ�) ������
		// --------------------------------------------------------------------------------
		//strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );	// xxx
		//strncat(work_data, file_info_p[i].org_name, sizeof(work_data) - strlen(work_data) );
		strncpy(work_data, file_info_p[i].org_name, sizeof(work_data) );
		uri_encode(skin_rep_data_line_p[count].file_uri_link, sizeof(skin_rep_data_line_p[count].file_uri_link), work_data, strlen(work_data) );
		debug_log_output("file_uri_link='%s'\n", skin_rep_data_line_p[count].file_uri_link);


		// --------------------------------------------------------------------------------
		// �ե����륹����פ�����ʸ�����������
		// --------------------------------------------------------------------------------
		conv_time_to_string(skin_rep_data_line_p[count].file_timestamp, file_info_p[i].time );
		conv_time_to_date_string(skin_rep_data_line_p[count].file_timestamp_date, file_info_p[i].time );
		conv_time_to_time_string(skin_rep_data_line_p[count].file_timestamp_time, file_info_p[i].time );


		// --------------------------------------------------------------------------------
		// �ե����륵����ɽ����ʸ��������
		// --------------------------------------------------------------------------------
		conv_num_to_unit_string(skin_rep_data_line_p[count].file_size_string, file_info_p[i].size );
		debug_log_output("file_size=%llu", file_info_p[i].size );
		debug_log_output("file_size_string='%s'", skin_rep_data_line_p[count].file_size_string );

		// --------------------------------------------------------------------------------
		// tvid ɽ����ʸ��������
		// --------------------------------------------------------------------------------
		snprintf(skin_rep_data_line_p[count].tvid_string, sizeof(skin_rep_data_line_p[count].tvid_string), "%d", i+1 );

		// --------------------------------------------------------------------------------
		// vod_string ɽ����ʸ���� �Ȥꤢ������""��
		// --------------------------------------------------------------------------------
		strncpy(skin_rep_data_line_p[count].vod_string, "", sizeof(skin_rep_data_line_p[count].vod_string) );

		// --------------------------------------------------------------------------------
		// ���ֹ� ����
		// --------------------------------------------------------------------------------
		skin_rep_data_line_p[count].row_num = count+1;

		// =========================================================
		// �ե����륿����Ƚ�����
		// =========================================================
		if ( S_ISDIR( file_info_p[i].type ) != 0 )
		{
			// �ǥ��쥯�ȥ�
			skin_rep_data_line_p[count].stream_type = TYPE_NO_STREAM;
			skin_rep_data_line_p[count].menu_file_type = TYPE_DIRECTORY;
		} else {
			// �ǥ��쥯�ȥ�ʳ�
			MIME_LIST_T *mime;

			if ((mime = lookup_mime_by_ext(skin_rep_data_line_p[count].file_extension)) == NULL) {
				skin_rep_data_line_p[count].stream_type = TYPE_NO_STREAM;
				skin_rep_data_line_p[count].menu_file_type = TYPE_UNKNOWN;
			} else {
				skin_rep_data_line_p[count].stream_type = mime->stream_type;
				skin_rep_data_line_p[count].menu_file_type = mime->menu_file_type;
			}
		}
		debug_log_output("menu_file_type=%d\n", skin_rep_data_line_p[count].menu_file_type);


		// =========================================================
		// �ե����륿�������ɬ�פ�ʸ������ɲä�����
		// =========================================================

		// ----------------------------
		// �ǥ��쥯�ȥ� �������
		// ----------------------------
		if ( skin_rep_data_line_p[count].menu_file_type == TYPE_DIRECTORY ||
			 skin_rep_data_line_p[count].menu_file_type == TYPE_PSEUDO_DIR )
		{
			// '?'���ɲä��롣
			strncat(skin_rep_data_line_p[count].file_uri_link, "?", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));

			// sort=���ؼ�����Ƥ�����硢���������Ѥ���
			if ( strlen(http_recv_info_p->sort) > 0 )
			{
				snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
				strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			}

		}

		// -------------------------------------
		// ���ȥ꡼��ե����� �������
		// -------------------------------------
		if ( skin_rep_data_line_p[count].stream_type == TYPE_STREAM )
		{
			// vod_string �� vod="0" �򥻥å�
			strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"0\"", sizeof(skin_rep_data_line_p[count].vod_string) );

			// ��ĥ���֤�����������
			extension_add_rename(skin_rep_data_line_p[count].file_uri_link, sizeof(skin_rep_data_line_p[count].file_uri_link));

			switch (skin_rep_data_line_p[count].menu_file_type) {
			case TYPE_SVI:
				// ------------------------------
				// SVI�����������ȴ����
				// ------------------------------

				// SVI�ե�����Υե�ѥ�����
				strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
				if ( work_filename[strlen(work_filename)-1] != '/' )
				{
					strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
				}
				strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
				filename_to_extension(work_filename, work_data, sizeof(work_data));
				if (strcasecmp(work_data, "svi") != 0) {
					cut_after_last_character(work_filename, '.');
					strncat(work_filename, ".svi", sizeof(work_filename) - strlen(work_filename));
					debug_log_output("extension rename (%s -> svi): %s", work_data, work_filename);
				}
				// --------------------------------------------------
				// svi�ե����뤬��������ʤ�����sv3���ѹ� 0.12f3
				// --------------------------------------------------
				if (access(work_filename, O_RDONLY) != 0) {
					char *p = work_filename;
					while(*p++);	// ������õ��
					if (*(p-2) == 'i')	// �Ǹ夬svi��'i'�ʤ�'3'�ˤ���
						*(p-2) = '3';
				}

				// --------------------------------------------------
				// SVI�ե����뤫�����򥲥åȤ��ơ�ʸ���������Ѵ���
				// Ĺ�����¤˹�碌��Cut
				// �� EUC���Ѵ� �� Cut �� MediaWizʸ�������ɤ��Ѵ�
				// --------------------------------------------------
				if (read_svi_info(work_filename, skin_rep_data_line_p[count].svi_info_data, sizeof(skin_rep_data_line_p[count].svi_info_data),  &rec_time ) == 0) {
					if ( strlen(skin_rep_data_line_p[count].svi_info_data) > 0 )
					{
						convert_language_code(	skin_rep_data_line_p[count].svi_info_data,
												work_data,
												sizeof(work_data),
												CODE_AUTO,
												CODE_EUC );

						// ()[]�κ���ե饰�����å�
						// �ե饰��TRUE�ǡ��ե����뤬�ǥ��쥯�ȥ�Ǥʤ���С���̤������롣

						if ( global_param.flag_filename_cut_parenthesis_area == TRUE )
						{
							cut_enclose_words(work_data, sizeof(work_data), "(", ")");
							cut_enclose_words(work_data, sizeof(work_data), "[", "]");
							debug_log_output("svi_info_data(enclose_words)='%s'\n", work_data);
						}

						// CUT�¹�
						euc_string_cut_n_length(work_data, global_param.menu_svi_info_length_max);

						// MediaWizʸ�������ɤ�
						convert_language_code(	work_data,
												skin_rep_data_line_p[count].svi_info_data,
												sizeof(skin_rep_data_line_p[count].svi_info_data),
												CODE_EUC,
												global_param.client_language_code);
					}
					debug_log_output("svi_info_data='%s'\n", skin_rep_data_line_p[count].svi_info_data);

					// SVI�����ɤ��Ͽ����֤�ʸ����ˡ�
					snprintf(skin_rep_data_line_p[count].svi_rec_time_data, sizeof(skin_rep_data_line_p[count].svi_rec_time_data),
								"%02d:%02d:%02d", rec_time /3600, (rec_time % 3600) / 60, rec_time % 60 );

					skin_rep_data_line_p[count].menu_file_type = TYPE_SVI;
				} else {
					//debug_log_output("no svi");
					skin_rep_data_line_p[count].svi_info_data[0] = '\0';
					skin_rep_data_line_p[count].svi_rec_time_data[0] = '\0';
					skin_rep_data_line_p[count].menu_file_type = TYPE_MOVIE;
				}
				/* FALLTHRU */
			case TYPE_MOVIE:
			case TYPE_MUSIC:
				// ������ǽ�ե����륫�����
				skin_rep_data_global_p->stream_files++;
				if (http_recv_info_p->flag_pc) {
					uri_encode(work_data, sizeof(work_data), file_info_p[i].org_name, strlen(file_info_p[i].org_name));

					snprintf(skin_rep_data_line_p[count].file_uri_link
						, sizeof(skin_rep_data_line_p[count].file_uri_link)
						, "/-.-playlist.pls?http://%s%s%s"
						, http_recv_info_p->recv_host
						, http_recv_info_p->recv_uri
						, work_data
					);
					debug_log_output("file_uri_link(pc)='%s'\n", skin_rep_data_line_p[count].file_uri_link);
				} else if (strncmp(file_info_p[i].org_name, "/-.-", 4)) {
					// SinglePlay �⡼�ɤˤ��롣 �Ȥ������ʤ����� ����2�Ԥ�Фä�����
					strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
					strncat(skin_rep_data_line_p[count].file_uri_link, "?action=SinglePlay&", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
				}

				break;

			case TYPE_PLAYLIST:
			case TYPE_MUSICLIST:
				// ----------------------------
				// playlist�ե����� �������
				// ----------------------------

				// vod_string �� vod="playlist"�򥻥å�
				strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
				break;

			default:
				// vod_string �� ���
				skin_rep_data_line_p[count].vod_string[0] = '\0';
				debug_log_output("unknown type");
				break;
			}
		}


		// ----------------------------
		// IMAGE�ե������������
		// ----------------------------
		if ( skin_rep_data_line_p[count].menu_file_type == TYPE_IMAGE
		 ||  skin_rep_data_line_p[count].menu_file_type == TYPE_JPEG  )
		{
			// ----------------------------------
			// ���᡼���ե�����Υե�ѥ�����
			// ----------------------------------
			strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
			if ( work_filename[strlen(work_filename)-1] != '/' )
			{
				strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
			}
			strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
			debug_log_output("work_filename(image) = %s", work_filename);



			// ------------------------
			// ���᡼���Υ�������GET
			// ------------------------

			image_width = 0;
			image_height = 0;

			// ��ĥ�Ҥ�ʬ��
			if ( (strcasecmp( skin_rep_data_line_p[count].file_extension, "jpg" ) == 0 ) ||
				 (strcasecmp( skin_rep_data_line_p[count].file_extension, "jpeg" ) == 0 ))
			{
				// JPEG�ե�����Υ�������GET
				jpeg_size( work_filename, &image_width, &image_height );
			}
			else if (strcasecmp( skin_rep_data_line_p[count].file_extension, "gif" ) == 0 )
			{
				// GIF�ե�����Υ�������GET
				gif_size( work_filename, &image_width, &image_height );
			}
			else if (strcasecmp( skin_rep_data_line_p[count].file_extension, "png" ) == 0 )
			{
				// PNG�ե�����Υ�������GET
				png_size( work_filename, &image_width, &image_height );
			}

			// ������������ʸ����ˡ�
			snprintf(skin_rep_data_line_p[count].image_width, sizeof(skin_rep_data_line_p[count].image_width), "%d", image_width );
			snprintf(skin_rep_data_line_p[count].image_height, sizeof(skin_rep_data_line_p[count].image_height), "%d", image_height );

			// ----------------------------------
			// ��󥯤κǸ��'?'���ɲä��롣
			// ----------------------------------
			strncat(skin_rep_data_line_p[count].file_uri_link, "?", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));

			if (skin_rep_data_line_p[count].menu_file_type == TYPE_JPEG) {
				// JPEG�ʤ� SinglePlay�ˤ��ơ������AllPlay�Ǥ������ǽ�ˤ��롣
				skin_rep_data_global_p->stream_files++;
				strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
				strncat(skin_rep_data_line_p[count].file_uri_link, "action=SinglePlay&", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			} else if ( strlen(http_recv_info_p->sort) > 0 ) {
			// sort=���ؼ�����Ƥ�����硢���������Ѥ���
				snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
				strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			}
		}


		// -------------------------------------
		// AVI�ե����� �������
		// -------------------------------------
		if (skin_rep_data_line_p[count].menu_file_type == TYPE_MOVIE) {

			// set default value.
			snprintf(skin_rep_data_line_p[count].avi_is_interleaved, sizeof(skin_rep_data_line_p[count].avi_is_interleaved), "?");
			snprintf(skin_rep_data_line_p[count].image_width, sizeof(skin_rep_data_line_p[count].image_width), "???");
			snprintf(skin_rep_data_line_p[count].image_height, sizeof(skin_rep_data_line_p[count].image_height), "???");
			snprintf(skin_rep_data_line_p[count].avi_fps, sizeof(skin_rep_data_line_p[count].avi_fps), "???");
			snprintf(skin_rep_data_line_p[count].avi_duration, sizeof(skin_rep_data_line_p[count].avi_duration), "??:??:??");
			snprintf(skin_rep_data_line_p[count].avi_vcodec, sizeof(skin_rep_data_line_p[count].avi_vcodec), "[none]");
			snprintf(skin_rep_data_line_p[count].avi_acodec, sizeof(skin_rep_data_line_p[count].avi_acodec), "[none]");
			snprintf(skin_rep_data_line_p[count].avi_hvcodec, sizeof(skin_rep_data_line_p[count].avi_hvcodec), "[none]");
			snprintf(skin_rep_data_line_p[count].avi_hacodec, sizeof(skin_rep_data_line_p[count].avi_hacodec), "[none]");

			if (strcasecmp(skin_rep_data_line_p[count].file_extension, "avi") == 0) {
				// ----------------------------------
				// AVI�ե�����Υե�ѥ�����
				// ----------------------------------
				strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
				if ( work_filename[strlen(work_filename)-1] != '/' )
				{
					strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
				}
				strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
				debug_log_output("work_filename(avi) = %s", work_filename);

				read_avi_info(work_filename, &skin_rep_data_line_p[count]);
			} else {
				// AVI����ʤ�
				snprintf(skin_rep_data_line_p[count].avi_vcodec, sizeof(skin_rep_data_line_p->avi_vcodec), "[%s]", skin_rep_data_line_p[count].file_extension);
				snprintf(skin_rep_data_line_p[count].avi_hvcodec, sizeof(skin_rep_data_line_p->avi_hvcodec), "[%s]", skin_rep_data_line_p[count].file_extension);
			}
		}

		// -------------------------------------
		// MP3�ե����� �������
		// -------------------------------------
		if ( (skin_rep_data_line_p[count].menu_file_type == TYPE_MUSIC) &&
			 (strcasecmp(skin_rep_data_line_p[count].file_extension, "mp3") == 0) )
		{

			// ----------------------------------
			// MP3�ե�����Υե�ѥ�����
			// ----------------------------------
			strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
			if ( work_filename[strlen(work_filename)-1] != '/' )
			{
				strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
			}
			strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
			debug_log_output("work_filename(mp3) = %s", work_filename);


			// ------------------------
			// MP3��ID3V1�ǡ�����GET
			// ------------------------
			mp3_id3_tag_read(work_filename, &(skin_rep_data_line_p[count]) );

			// for Debug.
			if ( skin_rep_data_line_p[count].mp3_id3v1_flag > 0 )
			{
				debug_log_output("mp3 title:'%s'", 		skin_rep_data_line_p[count].mp3_id3v1_title);
				debug_log_output("mp3 album:'%s'", 		skin_rep_data_line_p[count].mp3_id3v1_album);
				debug_log_output("mp3 artist:'%s'", 	skin_rep_data_line_p[count].mp3_id3v1_artist);
				debug_log_output("mp3 year:'%s'", 		skin_rep_data_line_p[count].mp3_id3v1_year);
				debug_log_output("mp3 comment:'%s'",	skin_rep_data_line_p[count].mp3_id3v1_comment);
				debug_log_output("mp3 title_info:'%s'",	skin_rep_data_line_p[count].mp3_id3v1_title_info);
				debug_log_output("mp3 title_info_limited:'%s'",	skin_rep_data_line_p[count].mp3_id3v1_title_info_limited);
			}

		}
	}

	debug_log_output("-----< end file info generate, count = %d >-----", count);



	// ============================
	// �����ǥ��쥯�ȥ긡��
	// ============================

	memset(skin_rep_data_global_p->secret_dir_link_html, '\0', sizeof(skin_rep_data_global_p->secret_dir_link_html));

	// �����ǥ��쥯�ȥ꤬¸�ߤ��Ƥ��뤫�����å���
	for ( i=0; i<SECRET_DIRECTORY_MAX; i++)
	{
		if ( strlen(secret_directory_list[i].dir_name) > 0 )	// �����ǥ��쥯�ȥ����ͭ�ꡩ
		{
			// ----------------------------------
			// �����ǥ��쥯�ȥ�Υե�ѥ�����
			// ----------------------------------
			strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
			if ( work_filename[strlen(work_filename)-1] != '/' )
			{
				strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
			}
			strncat(work_filename, secret_directory_list[i].dir_name, sizeof(work_filename) - strlen(work_filename));
			debug_log_output("check: work_filename = %s", work_filename);

			// ¸�ߥ����å�
			result = stat(work_filename, &dir_stat);
			if ( result == 0 )
			{
				if ( S_ISDIR(dir_stat.st_mode) != 0 ) // �ǥ��쥯�ȥ�¸�ߡ�
				{
					// ¸�ߤ��Ƥ��顢�����URI����
					strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );
					strncat(work_data, secret_directory_list[i].dir_name, sizeof(work_data) - strlen(work_data) );
					uri_encode(work_data2, sizeof(work_data2), work_data, strlen(work_data) );

					// HTML����
					snprintf(work_data, sizeof(work_data), "<a href=\"%s\" tvid=%d></a> ", work_data2, secret_directory_list[i].tvid);

					debug_log_output("secret_dir_html='%s'", work_data);
					strncat( skin_rep_data_global_p->secret_dir_link_html, work_data, sizeof(skin_rep_data_global_p->secret_dir_link_html) - strlen(skin_rep_data_global_p->secret_dir_link_html) );

				}
			}
		}
		else
		{
			break;
		}
	}
	debug_log_output("secret_dir_html='%s'", skin_rep_data_global_p->secret_dir_link_html);

	send_skin_filemenu(accept_socket, skin_rep_data_global_p, skin_rep_data_line_p, count);

	free( skin_rep_data_global_p );
	free( skin_rep_data_line_p );
}

// ==================================================
//  ��������ɤ߹��ߡ��ִ���������
// ==================================================
static void send_skin_filemenu(int accept_socket, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p, int lines)
{
	SKIN_MAPPING_T *sm_ptr;
	SKIN_T *header_skin;
	SKIN_T *line_skin[MAX_TYPES];
	SKIN_T *tail_skin;
	int i;
	int count;
	unsigned char *menu_work_p;

	// HTTP_OK�إå�����
	http_send_ok_header(accept_socket, 0, NULL);

	// ===============================
	// HEAD ������ե����� �ɤ߹��ߡ��ִ�������
	// ===============================
	if ((header_skin = skin_open(SKIN_MENU_HEAD_HTML)) == NULL) {
		return ;
	}
	// ľ��SKIN��Υǡ������ִ�
	skin_direct_replace_global(header_skin, skin_rep_data_global_p);
	skin_direct_send(accept_socket, header_skin);
	skin_close(header_skin);

	// ===============================
	// LINE�� ������ե����� �ɤ߹��ߡ��ִ�������
	// ===============================
	for (i=0; i<MAX_TYPES; i++) line_skin[i] = NULL;
	for (sm_ptr = skin_mapping; sm_ptr->skin_filename != NULL; sm_ptr++) {
		if (sm_ptr->filetype >= MAX_TYPES) {
			debug_log_output("CRITICAL: check MAX_TYPES or skin_mapping...");
			continue;
		}
		line_skin[sm_ptr->filetype] = skin_open(sm_ptr->skin_filename);
		if (line_skin[sm_ptr->filetype] == NULL) {
			debug_log_output("'%s' is not found?", sm_ptr->skin_filename);
		}
	}
	if (line_skin[TYPE_UNKNOWN] == NULL) {
		debug_log_output("FATAL: cannot find TYPE_UNKNOWN skin definition.");
		return ;
	}

	menu_work_p = malloc(MAX_SKIN_FILESIZE);
	if (menu_work_p == NULL) {
		debug_log_output("malloc failed.");
		return ;
	}

	// LINE �ִ��������ϡ�
	for (count=0; count < lines; count++) {
		int mtype;

		mtype = skin_rep_data_line_p[count].menu_file_type;
		strncpy(menu_work_p, skin_get_string(line_skin[line_skin[mtype] != NULL ? mtype : TYPE_UNKNOWN]), MAX_SKIN_FILESIZE);
		replase_skin_line_data(menu_work_p, MAX_SKIN_FILESIZE, &skin_rep_data_line_p[count] );
		replase_skin_grobal_data(menu_work_p, MAX_SKIN_FILESIZE, skin_rep_data_global_p);

		// �������
		send(accept_socket, menu_work_p, strlen(menu_work_p), 0);
	}

	// ����ΰ����
	free( menu_work_p );

	// LINE�ѤΥ��������
	for (i=0; i<MAX_TYPES; i++) {
		if (line_skin[i] != NULL) skin_close(line_skin[i]);
	}



	// ===============================
	// TAIL ������ե����� �ɤ߹��ߡ��ִ�������
	// ===============================
	if ((tail_skin = skin_open(SKIN_MENU_TAIL_HTML)) == NULL) {
		return ;
	}
	// ľ��SKIN��Υǡ������ִ�
	skin_direct_replace_global(tail_skin, skin_rep_data_global_p);
	skin_direct_send(accept_socket, tail_skin);
	skin_close(tail_skin);

	return;
}



// **************************************************************************
// *path �ǻ��ꤵ�줿�ǥ��쥯�ȥ��¸�ߤ���ե�������򥫥���Ȥ���
//
// return: �ե������
// **************************************************************************
static int count_file_num(unsigned char *path)
{
	int		count;

	DIR	*dir;
	struct dirent	*dent;

	debug_log_output("count_file_num() start. path='%s'", path);

	dir = opendir(path);
	if ( dir == NULL )	// ���顼�����å�
	{
		debug_log_output("opendir() error");
		return ( -1 );
	}

	count = 0;
	while ( 1 )
	{
		dent = readdir(dir);
		if ( dent == NULL  )
			break;

		// ̵��ե���������å���
		if ( file_ignoral_check(dent->d_name, path) != 0 )
			continue;

		count++;
	}

	closedir(dir);

	debug_log_output("count_file_num() end. counter=%d", count);
	return count;
}

// **************************************************************************
// *path �ǻ��ꤵ�줿TSV�ե������¸�ߤ���ե�������򥫥���Ȥ���
//
// return: �ե������
// **************************************************************************
static int count_file_num_in_tsv(unsigned char *path)
{
	int		count;
	int		fd;
	char	buf[1024];

	debug_log_output("count_file_num_in_tsv() start. path='%s'", path);

	fd = open(path, O_RDONLY);
	if ( fd < 0 )
	{
		debug_log_output("'%s' can't open.", path);
		return ( -1 );
	}

	count = 0;
	while ( 1 )
	{
		char *p;
		int ret;

		// �ե����뤫�顢�����ɤ�
		ret = file_read_line( fd, buf, sizeof(buf) );
		if ( ret < 0 )
		{
			debug_log_output("tsv EOF Detect.");
			break;
		}

		p = strchr(buf, '\t');
		if (p == NULL) continue;
		p = strchr(p+1, '\t');
		if (p == NULL) continue;

		count++;
	}

	close(fd);

	debug_log_output("count_file_num_in_tsv() end. counter=%d", count);
	return count;
}



// **************************************************************************
// �ǥ��쥯�ȥ��¸�ߤ������򡢥ե�����ο������ɤ߹��ࡣ
//
// return: �ɤ߹�����ե���������
// **************************************************************************
static int directory_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num)
{
	int	count;
	DIR	*dir;
	struct dirent	*dent;
	struct stat		file_stat;
	int				result;
	unsigned char	fullpath_filename[WIZD_FILENAME_MAX];
	unsigned char	*work_p;
	unsigned char	dir_name[WIZD_FILENAME_MAX];


	debug_log_output("directory_stat() start. path='%s'", path);


	dir = opendir(path);
	if ( dir == NULL )	// ���顼�����å�
	{
		debug_log_output("opendir() error");
		return ( -1 );
	}

	// ���ȤǺ������Ȥ��Τ���ˡ���ʬ�Υǥ��쥯�ȥ�̾��EUC������
	convert_language_code(	path, dir_name, sizeof(dir_name),
							global_param.server_language_code | CODE_HEX, CODE_EUC );
	// �Ǹ��'/'���դ��Ƥ�������
	cut_character_at_linetail(dir_name, '/');
	// '/'���������
	cut_before_last_character(dir_name, '/');

	count = 0;
	while ( 1 )
	{
		if ( count >= file_num )
			break;

		// �ǥ��쥯�ȥ꤫�顢�ե�����̾�򣱸�GET
		dent = readdir(dir);
		if ( dent == NULL  )
			break;

		// ̵��ե���������å���
		if ( file_ignoral_check(dent->d_name, path) != 0 )
			continue;

		//debug_log_output("dent->d_name='%s'", dent->d_name);


		// �ե�ѥ��ե�����̾����
		strncpy(fullpath_filename, path, sizeof(fullpath_filename) );
		strncat(fullpath_filename, dent->d_name, sizeof(fullpath_filename) - strlen(fullpath_filename) );

		//debug_log_output("fullpath_filename='%s'", fullpath_filename );

		// stat() �¹�
		memset(&file_stat, 0, sizeof(file_stat));
		result = stat(fullpath_filename, &file_stat);
		if ( result < 0 )
			continue;


		// �оݤ��ǥ��쥯�ȥ���ä���硢SVI��Ʊ��̾�Υǥ��쥯�ȥ�����å���
		if (( S_ISDIR( file_stat.st_mode ) != 0 ) && ( global_param.flag_hide_same_svi_name_directory == TRUE ))
		{
			// �����å��¹�
			if ( directory_same_check_svi_name(fullpath_filename) != 0 )
				continue;
		}

		// ���Υե�����̾����¸
		if (S_ISDIR( file_stat.st_mode )) {
			snprintf(file_info_p[count].org_name, sizeof(file_info_p[count].org_name), "%s/", dent->d_name);
		} else {
			strncpy(file_info_p[count].org_name, dent->d_name, sizeof(file_info_p[count].org_name) );
		}

		// EUC���Ѵ���
		convert_language_code(	dent->d_name, file_info_p[count].name,
								sizeof(file_info_p[count].name),
								global_param.server_language_code | CODE_HEX, CODE_EUC );

		// �ǥ��쥯�ȥ�Ǥʤ����Ρ��������
		if (S_ISDIR(file_stat.st_mode) == 0) {
			// ()[]�κ���ե饰�����å�
			// �ե饰��TRUE�ǡ��ե����뤬�ǥ��쥯�ȥ�Ǥʤ���С���̤������롣
			if (global_param.flag_filename_cut_parenthesis_area == TRUE) {
				cut_enclose_words(file_info_p[count].name, sizeof(file_info_p[count].name), "(", ")");
				cut_enclose_words(file_info_p[count].name, sizeof(file_info_p[count].name), "[", "]");
				debug_log_output("file_name(enclose_words)='%s'\n", file_info_p[count].name);
			}

			// �ǥ��쥯�ȥ�Ʊ̾ʸ�������ե饰�����å�
			// �ե饰��TRUE�ǡ��ե����뤬�ǥ��쥯�ȥ�Ǥʤ���С�Ʊ��ʸ���������
			if (( global_param.flag_filename_cut_same_directory_name == TRUE )
			 && ( strlen(dir_name) > 0 ))
			{
				// �ǥ��쥯�ȥ�Ʊ̾ʸ�����""���ִ�
				replase_character_first(file_info_p[count].name, sizeof(file_info_p[count].name), dir_name, "");

				// Ƭ��' '���դ��Ƥ���褦�ʤ�к����
				cut_first_character(file_info_p[count].name, ' ');

				debug_log_output("file_name(cut_same_directory_name)='%s'\n", file_info_p[count].name);
			}

			// �ե�����ʤ顢��ĥ�Ҥ�ʬΥ
			if ((work_p = strrchr(file_info_p[count].name, '.')) != NULL) {
				// ���λ����� file_info_p[count].name �� ��ĥ�ҥʥ��ˤʤ롣
				*work_p++ = '\0';
				strncpy(file_info_p[count].ext, work_p, sizeof(file_info_p[count].ext));
				debug_log_output("ext = '%s'", file_info_p[count].ext);
			}
		} else {
			// �ǥ��쥯�ȥ�ʤ��ĥ�ҥʥ�
			file_info_p[count].ext[0] = '\0';
		}

		// ����¾�������¸
		file_info_p[count].type = file_stat.st_mode;
		file_info_p[count].size = file_stat.st_size;
		file_info_p[count].time = file_stat.st_mtime;


		// SVI�ե�������ä��顢file_info_p[count].size �������ؤ��롣
		if (( strcasecmp(file_info_p[count].ext, "svi") ==  0 )
		 || ( strcasecmp(file_info_p[count].ext, "sv3") ==  0 )
		) {
			file_info_p[count].size = svi_file_total_size(fullpath_filename);
		}

		// vob��Ƭ�ե���������å� v0.12f3
		if ( (global_param.flag_show_first_vob_only == TRUE)
		   &&( strcasecmp(file_info_p[count].ext, "vob") == 0 ) ) {
			if (fullpath_filename[strlen(fullpath_filename)-5] == '1') {
				JOINT_FILE_INFO_T joint_file_info;
				if (analyze_vob_file(fullpath_filename, &joint_file_info ) == 0) {
					file_info_p[count].size = joint_file_info.total_size;
}
			} else
				continue;
		}
		count++;
	}

	closedir(dir);

	debug_log_output("directory_stat() end. count=%d", count);

	return count;
}

// **************************************************************************
// TSV�ե������¸�ߤ������򡢥���ȥ�ο������ɤ߹��ࡣ
//
// return: �ɤ߹�����ե���������
// **************************************************************************
static int tsv_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num)
{
	int	count;
	struct stat		file_stat;
	int				result;
	unsigned char	fullpath_filename[WIZD_FILENAME_MAX];
	char			buf[1024];
	int				fd;


	debug_log_output("tsv_stat() start. path='%s'", path);

	fd = open(path, O_RDONLY);
	if ( fd < 0 )
	{
		debug_log_output("'%s' can't open.", path);
		return ( -1 );
	}
	cut_after_last_character(path, '/');
	strcat(path, "/"); // XXX: there's no size check..

	count = 0;
	while ( count < file_num )
	{
		char *p;
		char *fname, *tvid, *title;
		int ret;

		// �ե����뤫�顢�����ɤ�
		ret = file_read_line( fd, buf, sizeof(buf) );
		if ( ret < 0 )
		{
			debug_log_output("tsv EOF Detect.");
			break;
		}

		// �����Υ��ڡ���������
		cut_character_at_linetail(buf, ' ');

		// ���Ԥʤ顢continue
		if ( strlen( buf ) == 0 )
		{
			debug_log_output("continue.");
			continue;
		}

		p = buf;
		fname = p;
		p = strchr(p, '\t');
		if (p == NULL) {
			debug_log_output("\\t notfound");
			continue;
		}
		*p++ = '\0';

		tvid = p;
		p = strchr(p, '\t');
		if (p == NULL) {
			debug_log_output("\\t notfound 2");
			continue;
		}
		*p++ = '\0';

		title = p;

		// �ե�ѥ��ե�����̾����
		if (fname[0] == '/') {
			strncpy(fullpath_filename, global_param.document_root, sizeof(fullpath_filename) );
		} else {
			strncpy(fullpath_filename, path, sizeof(fullpath_filename) );
		}
		strncat(fullpath_filename, fname, sizeof(fullpath_filename) - strlen(fullpath_filename) );

		// �ե�����̾����¸
		strncpy(file_info_p[count].org_name, fname, sizeof(file_info_p[count].org_name) );
		// to euc
		convert_language_code(	title, file_info_p[count].name, sizeof(file_info_p[count].name),
								CODE_AUTO | CODE_HEX, CODE_EUC );


		debug_log_output("fullpath_filename='%s'", fullpath_filename );

		// ��ĥ�Ҥ�����
		// tsv�ξ��ϡ�org_name(�θ���fname) ��������
		filename_to_extension( fname, file_info_p[count].ext, sizeof(file_info_p[count].ext) );

		// stat() �¹�
		memset(&file_stat, 0, sizeof(file_stat));
		result = stat(fullpath_filename, &file_stat);
		if ( result >= 0 ) {
			// ����ȯ��
			file_info_p[count].type = file_stat.st_mode;
			file_info_p[count].size = file_stat.st_size;
			file_info_p[count].time = file_stat.st_mtime;

			// �ǥ��쥯�ȥ�Ȥ狼�ä����ϡ���ĥ�Ҥ���
			if (S_ISDIR(file_stat.st_mode)) {
				file_info_p[count].ext[0] = '\0';
			}
		} else {
			// ���ΤϤʤ�����äơ����դȥ������ȥե�����μ�������
			file_info_p[count].type = 0;
			file_info_p[count].size = 0;
			file_info_p[count].time = 0;
		}

		count++;
	}

	close(fd);

	debug_log_output("tsv_stat() end. count=%d", count);

	return count;
}


// ******************************************************************
// �ե�����̵������å�
// �ǥ��쥯�ȥ���ǡ��ե������̵�뤹�뤫���ʤ�����Ƚ�Ǥ��롣
// return: 0:OK  -1 ̵��
// ******************************************************************
static int file_ignoral_check( unsigned char *name, unsigned char *path )
{
	int				i;
	unsigned char	file_extension[16];
	char			flag;

	unsigned char	work_filename[WIZD_FILENAME_MAX];
	struct stat		file_stat;
	int				result;

	char *ignore_names[] = {
		".",
		"..",
		"lost+found",
		"RECYCLER",
		"System Volume Information",
		"cgi-bin",
	};
	int ignore_count = sizeof(ignore_names) / sizeof(char*);

	// ==================================================================
	// �嵭 ignore_names �˳��������Τ򥹥��å�
	// ==================================================================
	for (i=0; i<ignore_count; i++) {
		if (!strcmp(name, ignore_names[i])) return -1;
	}

	// ==================================================================
	// MacOSX�� "._" �ǻϤޤ�ե�����򥹥��åסʥ꥽�����ե������
	// ==================================================================
	if ( strncmp(name, "._", 2 ) == 0 )
	{
		return ( -1 );
	}


	// ==================================================================
	// wizd���Τ�ʤ��ե����뱣���ե饰�����äƤ����顢��ĥ�ҥ����å�
	// ==================================================================
	if ( global_param.flag_unknown_extention_file_hide == TRUE )
	{
		filename_to_extension( name, file_extension, sizeof(file_extension) );

		flag = 0;
		if ( strlen(file_extension) > 0 ) // ��ĥ��̵�����Τ�ʤ���Ʊ��
		{
			for ( i=0; mime_list[i].file_extension != NULL; i++)
			{
				if ( strcasecmp(mime_list[i].file_extension, file_extension ) == 0 )
				{
					//debug_log_output("%s Known!!!", file_extension );
					flag = 1; // �ΤäƤ�
					break;
				}
			}
		}

		if ( flag == 0 ) // �Τ�ʤ��ä���
		{
			// -----------------------------------------------
			// �ե����뤬���ۥ�Ȥ˥ե����뤫�����å���
			// �⤷��ǥ��쥯�ȥ�ʤ顢return���ʤ���
			// -----------------------------------------------

			// �ե�ѥ�����
			strncpy(work_filename, path, sizeof(work_filename) );
			if ( work_filename[strlen(work_filename)-1] != '/' )
			{
				strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
			}
			strncat(work_filename, name, sizeof(work_filename) - strlen(work_filename) );


			debug_log_output("'%s' Unknown. directory check start.", work_filename );

			// stat() �¹�
			result = stat(work_filename, &file_stat);
			if ( result != 0 )
				return ( -1 );

			if ( S_ISDIR(file_stat.st_mode) == 0 ) // �ǥ��쥯�ȥꤸ��ʤ����Τ�ʤ���ĥ�ҥե�������ȳ��ꡣ
			{
				debug_log_output("'%s' Unknown!!!", name );
				return ( -1 );
			}

			debug_log_output("'%s' is a directory!!!", name );
		}
	}


	// ==================================================================
	// �����ǥ��쥯�ȥ�����å�
	// ==================================================================
	for ( i=0; i<SECRET_DIRECTORY_MAX; i++ )
	{
		if ( strcmp(name, secret_directory_list[i].dir_name ) == 0 )
		{
			debug_log_output("secret_directory_list[%d].'%s' HIT!!", i, secret_directory_list[i].dir_name);
			return ( -1 );
		}
	}


	return ( 0 );
}

// ******************************************************************
// SVI��Ʊ��̾�Υǥ��쥯�ȥ�����å���
// return: 0:OK  -1 ̵��
// ******************************************************************
static int directory_same_check_svi_name( unsigned char *name )
{
	unsigned char	check_svi_filename[WIZD_FILENAME_MAX];
	struct stat		svi_stat;
	int				result;

	debug_log_output("directory_same_check_svi_name() start.'%s'", name);

	// �����å�����SVI�ե�����̾����
	strncpy( check_svi_filename, name, sizeof(check_svi_filename));
	strncat( check_svi_filename, ".svi", sizeof(check_svi_filename) - strlen(check_svi_filename) );

	// --------------------------------------------------
	// sv3 0.12f3
	// --------------------------------------------------
	if (access(check_svi_filename, O_RDONLY) != 0) {
		char *p = check_svi_filename;
		while(*p++);	// ������õ��
		if (*(p-2) == 'i')	// �Ǹ夬svi��'i'
		*(p-2) = '3';
	}

	result = stat(check_svi_filename, &svi_stat);
	if ( result >= 0 ) // ���ä���
	{
		debug_log_output("check_svi_filename '%s' found!!", check_svi_filename);
		return ( -1 );
	}

	return ( 0 );	// OK
}


// **************************************************************************
//
// HTTP_OK �إå�����������
//
// **************************************************************************
void http_send_ok_header(int accept_socket, unsigned long long content_length, char *content_type)
{
	send_printf(accept_socket, HTTP_OK);
	send_printf(accept_socket, HTTP_CONNECTION);
	if (content_type != NULL) {
		send_printf(accept_socket, HTTP_CONTENT_TYPE, content_type);
	} else {
		send_printf(accept_socket, HTTP_CONTENT_TYPE, "text/html");
	}
	send_printf(accept_socket, HTTP_SERVER_NAME, SERVER_NAME);
	if (content_length > 0) {
		send_printf(accept_socket, HTTP_CONTENT_LENGTH, content_length);
	}
	send_printf(accept_socket, HTTP_END);
}

static char* create_1line_playlist(HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p)
{
	unsigned char	file_ext[WIZD_FILENAME_MAX];
	unsigned char	file_name[WIZD_FILENAME_MAX];
	unsigned char	disp_filename[WIZD_FILENAME_MAX];
	unsigned char	file_uri_link[WIZD_FILENAME_MAX];

	static unsigned char	work_data[WIZD_FILENAME_MAX * 2];
	SKIN_REPLASE_LINE_DATA_T	mp3_id3tag_data;	// MP3 ID3�����ɤ߹�����
	MIME_LIST_T *mime;
	int input_code;

	// ---------------------------------------------------------
	// �ե����륿����Ƚ�ꡣ�оݳ��ե�����ʤ�������ʤ�
	// ---------------------------------------------------------

	if (file_info_p == NULL) {
		// single
		strncpy( disp_filename, http_recv_info_p->recv_uri, sizeof(disp_filename) );
		cut_before_last_character(disp_filename, '/');
		filename_to_extension(http_recv_info_p->send_filename, file_ext, sizeof(file_ext));
		input_code = global_param.server_language_code;
	} else {
		// menu
		strncpy(disp_filename, file_info_p->name, sizeof(disp_filename));
		strncpy(file_ext, file_info_p->ext, sizeof(file_ext));
		input_code = CODE_EUC; // file_info_p->name �� ���EUC (wizd 0.12h)
	}
	debug_log_output("file_extension='%s'\n", file_ext);

	mime = lookup_mime_by_ext(file_ext);

	// ------------------------------------------------------
	// �����оݳ��ʤ����
	// ------------------------------------------------------
	if ( mime->menu_file_type != TYPE_MOVIE
	 &&	 mime->menu_file_type != TYPE_MUSIC
	 &&	 mime->menu_file_type != TYPE_JPEG
	 &&	 mime->menu_file_type != TYPE_SVI
	) {
		return NULL;
	}

	// -----------------------------------------
	// ��ĥ�Ҥ�mp3�ʤ顢ID3���������å���
	// -----------------------------------------
	mp3_id3tag_data.mp3_id3v1_flag = 0;

	if ( strcasecmp(file_ext, "mp3" ) == 0 ) {
		// MP3�ե�����Υե�ѥ�����
		strncpy( work_data, http_recv_info_p->send_filename, sizeof(work_data));
		if (file_info_p) {
			// menu
			if ( work_data[strlen(work_data)-1] != '/' )
			{
				strncat(work_data, "/", sizeof(work_data) - strlen(work_data) );
			}
			strncat(work_data, file_info_p->org_name, sizeof(work_data) - strlen(work_data));
		}
		debug_log_output("work_data(mp3) = %s", work_data);

		// ID3���������å�
		memset( &mp3_id3tag_data, 0, sizeof(mp3_id3tag_data));
		mp3_id3_tag_read(work_data , &mp3_id3tag_data );
	}


	// MP3 ID3������¸�ߤ����ʤ�С�playlist ɽ���ե�����̾��ID3�������֤������롣
	if ( mp3_id3tag_data.mp3_id3v1_flag == 1 )
	{
		strncpy(work_data, mp3_id3tag_data.mp3_id3v1_title_info, sizeof(work_data));
		strncat(work_data, ".mp3", sizeof(work_data) - strlen(work_data));	// ���ߡ��γ�ĥ�ҡ�playlist_filename_adjustment()�Ǻ������롣

		// =========================================
		// playlistɽ���� ID3������Ĵ��
		// EUC���Ѵ� �� ��ĥ�Һ�� �� (ɬ�פʤ�)Ⱦ��ʸ�������Ѥ��Ѵ� �� MediaWiz�����ɤ��Ѵ� �� SJIS�ʤ顢����ʸ��������0x7C('|')��ޤ�ʸ��������
		// =========================================
		playlist_filename_adjustment( work_data, file_name, sizeof(file_name), CODE_AUTO );
	}
	else
	// MP3 ID3������¸�ߤ��ʤ��ʤ�С��ե�����̾�򤽤Τޤ޻��Ѥ��롣
	{
		// ---------------------------------
		// ɽ���ե�����̾ Ĵ��
		// EUC���Ѵ� �� ��ĥ�Һ�� �� (ɬ�פʤ�)Ⱦ��ʸ�������Ѥ��Ѵ� �� MediaWiz�����ɤ��Ѵ� �� SJIS�ʤ顢����ʸ��������0x7C('|')��ޤ�ʸ��������
		// ---------------------------------
		playlist_filename_adjustment(disp_filename, file_name, sizeof(file_name), input_code);
	}

	// ------------------------------------
	// Link��URI(���󥳡��ɺѤ�) ������
	// ------------------------------------
	if (file_info_p == NULL) {
		strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );
	} else if (file_info_p->org_name[0] == '/' || !strncmp(file_info_p->org_name, "http://", 7)) {
		// never happen normally. but for proxy. in tsv /-.-http://... or http://...
		strncpy(work_data, file_info_p->org_name, sizeof(work_data));
	} else {
		strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );
		cut_after_last_character( work_data, '/' );
		if ( work_data[strlen(work_data)-1] != '/' )
		{
			strncat( work_data, "/", sizeof(work_data) );
		}
		strncat(work_data, file_info_p->org_name, sizeof(work_data)- strlen(work_data) );
	}

	uri_encode(file_uri_link, sizeof(file_uri_link), work_data, strlen(work_data) );

	if (mime->menu_file_type == TYPE_JPEG) {
		strncat(file_uri_link, "?action=Resize.jpg", sizeof(file_uri_link)- strlen(file_uri_link) );
	}

	debug_log_output("file_uri_link='%s'\n", file_uri_link);

	// URI�γ�ĥ���֤�����������
	extension_add_rename(file_uri_link, sizeof(file_uri_link));

	// ------------------------------------
	// �ץ쥤�ꥹ�Ȥ�����
	// ------------------------------------
	snprintf(work_data, sizeof(work_data), "%s|%d|%d|http://%s%s|\r\n"
		, file_name
		, 0, 0
		, http_recv_info_p->recv_host, file_uri_link
	);

	debug_log_output("work_data='%s'", work_data);

	return work_data;
}


// **************************************************************************
// * allplay �ѤΥץ쥤�ꥹ�Ȥ�����
// **************************************************************************
static void create_all_play_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num)
{
	int i;

	debug_log_output("create_all_play_list() start.");

	// HTTP_OK�إå�����
	http_send_ok_header(accept_socket, 0, NULL);

	// =================================
	// �ե�����ɽ���Ѿ��� ����������
	// =================================
	for ( i=0; i<file_num ; i++ )
	{
		char *ptr;

		// �ǥ��쥯�ȥ��̵��
		if ( S_ISDIR( file_info_p[i].type ) != 0 ) {
			continue;
		}

		ptr = create_1line_playlist(http_recv_info_p, &file_info_p[i]);
		if (ptr != NULL) {
			// 1�ԤŤġ�����������
			send(accept_socket, ptr, strlen(ptr), 0);
		}
	}

	return;
}




// *************************************************************************
//  ɽ���ե����륽����
// *************************************************************************
static void file_info_sort( FILE_INFO_T *p, int num, unsigned long type )
{


	int nDir, nFile, i, row;

	// �ǥ��쥯�ȥ���ȥե��������ʬΥ
	for ( nDir = 0, i = 0; i < num; ++i )
	{
		if ( S_ISDIR( p[ i ].type ) )
		{
			++nDir;
		}
	}
	nFile = num - nDir;

	// �����Ƚ��� ******************************************************************
	// ����
	row = 0;

	// �ǥ��쥯�ȥ꥽���Ȥ�����йԤ�
	if ( SORT_DIR_FLAG( type ) && nDir > 0 )
	{
		// �Ȥꤢ��������������
		qsort( p, num, sizeof( FILE_INFO_T ), dir_sort_api[ SORT_DIR_FLAG( type ) ] );

		// �ǥ��쥯�ȥ�̾�Υ�����
		qsort( &p[ ( SORT_DIR_FLAG( type ) == SORT_DIR_DOWN ) ? num - nDir : 0 ], nDir, sizeof( FILE_INFO_T ), file_sort_api[ SORT_NAME_UP ] );

		// �ե�������ֳ���
		row = ( SORT_DIR_FLAG( type ) == SORT_DIR_DOWN ) ? 0 : nDir;
	}
	else
	{
		// �ե����륽�����оݤ�����ˤ���
		nFile = num;
	}

	// �ե����륽���Ȥ�Ԥ�
	// �ǥ��쥯�ȥ꤬�оݤˤʤäƤ��ʤ���С������оݤˤ���
	if ( ( type & SORT_FILE_MASK ) && nFile > 0 )
	{
		qsort( &p[ row ], nFile, sizeof( FILE_INFO_T ), file_sort_api[ ( type & SORT_FILE_MASK ) ] );
	}

	return;
}



// *************************************************************************
// �ǥ��쥯�ȥ�Υ�����
// *************************************************************************
static int _file_info_dir_sort( const void *in_a, const void *in_b, int order )
{
	FILE_INFO_T *a, *b;
	int n1, n2;

	a = (FILE_INFO_T *) in_a;
	b = (FILE_INFO_T *) in_b;
	n1 = ( S_ISDIR( a->type ) ? 1 : 0 );
	n2 = ( S_ISDIR( b->type ) ? 1 : 0 );
	return ( n1 == n2 ? 0 : ( order ? n1 - n2 : n2 - n1 ) );
}
static int _file_info_dir_sort_order_up( const void *in_a, const void *in_b )
{
	return _file_info_dir_sort( in_a, in_b, 0 );
}
static int _file_info_dir_sort_order_down( const void *in_a, const void *in_b )
{
	return _file_info_dir_sort( in_a, in_b, 1 );
}




// *************************************************************************
// ̾���Υ�����
// *************************************************************************
static int _file_info_name_sort( const void *in_a, const void *in_b, int order )
{
	FILE_INFO_T *a, *b;

	a = (FILE_INFO_T *) in_a;
	b = (FILE_INFO_T *) in_b;
	return ( order ? strcmp( b->name, a->name ) : strcmp( a->name, b->name ) );
}
static int _file_info_name_sort_order_up( const void *in_a, const void *in_b )
{
	return _file_info_name_sort( in_a, in_b, 0 );
}
static int _file_info_name_sort_order_down( const void *in_a, const void *in_b )
{
	return _file_info_name_sort( in_a, in_b, 1 );
}




// *************************************************************************
// �������Υ�����
// *************************************************************************
static int _file_info_size_sort( const void *in_a, const void *in_b, int order )
{
	FILE_INFO_T *a, *b;

	a = (FILE_INFO_T *) in_a;
	b = (FILE_INFO_T *) in_b;
	return (int)( order ? b->size - a->size : a->size - b->size );
}
static int _file_info_size_sort_order_up( const void *in_a, const void *in_b )
{
	return _file_info_size_sort( in_a, in_b, 0 );
}
static int _file_info_size_sort_order_down( const void *in_a, const void *in_b )
{
	return _file_info_size_sort( in_a, in_b, 1 );
}



// *************************************************************************
// ���֤Υ�����
// *************************************************************************
static int _file_info_time_sort( const void *in_a, const void *in_b, int order )
{
	FILE_INFO_T *a, *b;

	a = (FILE_INFO_T *) in_a;
	b = (FILE_INFO_T *) in_b;
	return (int)( order ? b->time - a->time : a->time - b->time );
}
static int _file_info_time_sort_order_up( const void *in_a, const void *in_b )
{
	return _file_info_time_sort( in_a, in_b, 0 );
}
static int _file_info_time_sort_order_down( const void *in_a, const void *in_b )
{
	return _file_info_time_sort( in_a, in_b, 1 );
}

// *************************************************************************
// ����åե�
// *************************************************************************
static int _file_info_shuffle( const void *in_a, const void *in_b )
{
	FILE_INFO_T *a, *b;

	a = (FILE_INFO_T *) in_a;
	b = (FILE_INFO_T *) in_b;
	return (rand() & 0x800) ? 1 : -1;
}








#define		FIT_TERGET_WIDTH	(533)
#define		FIT_TERGET_HEIGHT	(400)


// **************************************************************************
// * ���᡼���ӥ塼���������������ֿ�
// **************************************************************************
void http_image_viewer(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	unsigned int 	image_width, image_height;
	unsigned int 	image_viewer_width, image_viewer_height;

	unsigned char	file_extension[32];

	unsigned char	work_data[WIZD_FILENAME_MAX];

	struct	stat	image_stat;
	int		result;
	int		now_page;

	int		flag_fit_mode = 0;

	SKIN_REPLASE_IMAGE_VIEWER_DATA_T	image_viewer_info;
	SKIN_T *skin;


	// ========================
	// �ִ��ѥǡ�������
	// ========================
	// ��ľ�Υǥ��쥯�ȥ�ѥ�(�ƥѥ�)��������(URI���󥳡���)
	strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) - strlen(work_data) );
	cut_after_last_character(work_data, '/');
	strncat(work_data, "/", sizeof(work_data) - strlen(work_data) ); // �Ǹ��'/'���ɲá�
	debug_log_output("parent_directory='%s'", work_data);


	uri_encode(image_viewer_info.parent_directory_link, sizeof(image_viewer_info.parent_directory_link), work_data, strlen(work_data));
	strncat(image_viewer_info.parent_directory_link, "?", sizeof(image_viewer_info.parent_directory_link) - strlen(image_viewer_info.parent_directory_link));
	// sort=���ؼ�����Ƥ�����硢���������Ѥ���
	if ( strlen(http_recv_info_p->sort) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
		strncat(image_viewer_info.parent_directory_link, work_data, sizeof(image_viewer_info.parent_directory_link) - strlen(image_viewer_info.parent_directory_link));
	}
	debug_log_output("parent_directory_link='%s'", image_viewer_info.parent_directory_link);


	// ���ѥ�̾ ɽ���� ����(ʸ���������Ѵ�)
	convert_language_code(	http_recv_info_p->recv_uri,
							image_viewer_info.current_uri_name,
							sizeof(image_viewer_info.current_uri_name),
							global_param.server_language_code | CODE_HEX,
							global_param.client_language_code);
	debug_log_output("image_viewer: current_uri = '%s'", image_viewer_info.current_uri_name );

	// ���ѥ�̾ Link��������URI���󥳡��ɡ�
	uri_encode(image_viewer_info.current_uri_link, sizeof(image_viewer_info.current_uri_link), http_recv_info_p->recv_uri, strlen(http_recv_info_p->recv_uri));
	strncat(image_viewer_info.current_uri_link, "?", sizeof(image_viewer_info.current_uri_link) - strlen(image_viewer_info.current_uri_link)); // �Ǹ��'?'���ɲ�
	// sort=���ؼ�����Ƥ�����硢���������Ѥ���
	if ( strlen(http_recv_info_p->sort) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
		strncat(image_viewer_info.current_uri_link, work_data, sizeof(image_viewer_info.current_uri_link) - strlen(image_viewer_info.current_uri_link));
	}
	debug_log_output("image_viewer: current_uri_link='%s'", image_viewer_info.current_uri_link);





	if ( http_recv_info_p->page <= 1 )
		now_page = 1;
	else
		now_page = http_recv_info_p->page;


	// 	���ߤΥڡ��� ɽ����
	snprintf(image_viewer_info.now_page_str, sizeof(image_viewer_info.now_page_str), "%d", now_page );


	// �ե����륵����, �����ॹ�����GET
	result = stat(http_recv_info_p->send_filename, &image_stat);
	if ( result != 0 )
	{
		debug_log_output("stat(%s) error.", http_recv_info_p->send_filename);
		return;
	}

	conv_num_to_unit_string(image_viewer_info.file_size_string, image_stat.st_size );
	conv_time_to_string(image_viewer_info.file_timestamp, image_stat.st_mtime );
	conv_time_to_date_string(image_viewer_info.file_timestamp_date, image_stat.st_mtime );
	conv_time_to_time_string(image_viewer_info.file_timestamp_time, image_stat.st_mtime );


	// ����������GET
	image_width = 0;
	image_height = 0;

	// ��ĥ�Ҽ��Ф�
	filename_to_extension(http_recv_info_p->send_filename, file_extension, sizeof(file_extension) );

	// ��ĥ�Ҥ�ʬ��
	if ( (strcasecmp( file_extension, "jpg" ) == 0 ) ||
		 (strcasecmp( file_extension, "jpeg" ) == 0 ))
	{
		// JPEG�ե�����Υ�������GET
		jpeg_size( http_recv_info_p->send_filename, &image_width, &image_height );
	}
	else if (strcasecmp( file_extension, "gif" ) == 0 )
	{
		// GIF�ե�����Υ�������GET
		gif_size( http_recv_info_p->send_filename, &image_width, &image_height );
	}
	else if (strcasecmp( file_extension, "png" ) == 0 )
	{
		// PNG�ե�����Υ�������GET
		png_size( http_recv_info_p->send_filename, &image_width, &image_height );
	}
	debug_log_output("image_width=%d, image_height=%d", image_width, image_height);

	snprintf(image_viewer_info.image_width, sizeof(image_viewer_info.image_width), "%d", image_width);
	snprintf(image_viewer_info.image_height, sizeof(image_viewer_info.image_height), "%d", image_height);



	// ɽ���������ʲ���
	if ( strcasecmp(http_recv_info_p->option, "-2" ) == 0 )  // 0.5x
	{
		image_viewer_width = image_width / 2;
		image_viewer_height = image_height / 2;
		strncpy(image_viewer_info.image_viewer_mode, "0.5x", sizeof(image_viewer_info.image_viewer_mode) );
	}
	else if ( strcasecmp(http_recv_info_p->option, "2x" ) == 0 )  // 2x
	{
		image_viewer_width = image_width * 2;
		image_viewer_height = image_height * 2 ;
		strncpy(image_viewer_info.image_viewer_mode, "2x", sizeof(image_viewer_info.image_viewer_mode) );
	}
	else if ( strcasecmp(http_recv_info_p->option, "4x" ) == 0 )  // 4x
	{
		image_viewer_width = image_width * 4;
		image_viewer_height = image_height * 4 ;
		strncpy(image_viewer_info.image_viewer_mode, "4x", sizeof(image_viewer_info.image_viewer_mode) );
	}
	else if ( strcasecmp(http_recv_info_p->option, "fit" ) == 0 )  // FIT
	{
		// �Ĥ˹�碌�ƥꥵ�������Ƥߤ�
		image_viewer_width = (image_width  * FIT_TERGET_HEIGHT) / image_height;
		image_viewer_height = FIT_TERGET_HEIGHT;

		if ( image_viewer_width > FIT_TERGET_WIDTH ) // ����Ķ���Ƥ�����
		{
			// ���˹�碌�ƥꥵ�������롣
			image_viewer_width = FIT_TERGET_WIDTH;
			image_viewer_height = image_height * FIT_TERGET_WIDTH / image_width;
		}

		debug_log_output("fit:  (%d,%d) -> (%d,%d)", image_width, image_height, image_viewer_width, image_viewer_height);


		strncpy(image_viewer_info.image_viewer_mode, "FIT", sizeof(image_viewer_info.image_viewer_mode) );

		flag_fit_mode = 1;
	}
	else	// 1x
	{
		image_viewer_width = image_width;
		image_viewer_height = image_height;
		strncpy(image_viewer_info.image_viewer_mode, "1x", sizeof(image_viewer_info.image_viewer_mode) );
	}

	snprintf(image_viewer_info.image_viewer_width, sizeof(image_viewer_info.image_viewer_width), "%d", image_viewer_width );
	snprintf(image_viewer_info.image_viewer_height, sizeof(image_viewer_info.image_viewer_height), "%d", image_viewer_height );


	// ==============================
	// ImageViewer �������ɤ߹���
	// ==============================
	if ((skin = skin_open(SKIN_IMAGE_VIEWER_HTML)) == NULL) {
		return ;
	}

	// ==============================
	// �ִ��¹�
	//   ľ��SKIN��Υǡ������ִ�
	// ==============================
	skin_direct_replace_image_viewer(skin, &image_viewer_info);

    // FIT�⡼�ɥ����å�
    if ( flag_fit_mode == 0 ) {
        skin_direct_cut_enclosed_words(skin, SKIN_KEYWORD_DEL_IS_NO_FIT_MODE, SKIN_KEYWORD_DEL_IS_NO_FIT_MODE_E); // FIT�⡼�ɤǤϤʤ�
    } else {
        skin_direct_cut_enclosed_words(skin, SKIN_KEYWORD_DEL_IS_FIT_MODE, SKIN_KEYWORD_DEL_IS_FIT_MODE_E); // FIT�⡼��
    }

	// =================
	// �ֿ��¹�
	// =================
	http_send_ok_header(accept_socket, 0, NULL);
	skin_direct_send(accept_socket, skin);
	skin_close(skin);

	return;
}

// **************************************************************************
// * Single Play List�����������ֿ�
// **************************************************************************
void http_music_single_play(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	char *ptr;

	// HTTP_OK�إå�����
	http_send_ok_header(accept_socket, 0, NULL);

	ptr = create_1line_playlist(http_recv_info_p, NULL);
	if (ptr != NULL) {
		// ���Ԥ�������
		send(accept_socket, ptr, strlen(ptr), 0);
	}

	return;
}



// **************************************************************************
// * wizd play list�ե�����(*.plw)��ꡢPlayList�����������ֿ�
// **************************************************************************
void http_listfile_to_playlist_create(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	int		fd;
	int		ret;

	unsigned char	buf[WIZD_FILENAME_MAX];

	unsigned char	listfile_path[WIZD_FILENAME_MAX];

	unsigned char	file_extension[32];
	unsigned char	file_name[255];
	unsigned char	file_uri_link[WIZD_FILENAME_MAX];

	unsigned char	work_data[WIZD_FILENAME_MAX *2];
	SKIN_REPLASE_LINE_DATA_T	mp3_id3tag_data;	// MP3 ID3�����ɤ߹�����



	// plw�ǡ�����open
	fd = open(http_recv_info_p->send_filename, O_RDONLY);
	if ( fd < 0 ) {
		debug_log_output("'%s' can't open.", http_recv_info_p->send_filename);
		return;
	}

	// listfile������ѥ�������
	strncpy( listfile_path, http_recv_info_p->recv_uri, sizeof(listfile_path));
	cut_after_last_character( listfile_path, '/' );
	if ( listfile_path[strlen(listfile_path)-1] != '/' )
	{
		strncat( listfile_path, "/", sizeof(listfile_path) );
	}

	debug_log_output( "listfile_path: '%s'", listfile_path );


	// =============================
	// �إå�������
	// =============================
	http_send_ok_header(accept_socket, 0, NULL);

	//=====================================
	// �ץ쥤�ꥹ�� ��������
	//=====================================
	while ( 1 ) {
		// �ե����뤫�顢�����ɤ�
		ret = file_read_line( fd, buf, sizeof(buf) );
		if ( ret < 0 ) {
			debug_log_output("listfile EOF Detect.");
			break;
		}

		debug_log_output("-------------");
		debug_log_output("read_buf:'%s'", buf);


		// �����Ⱥ��
		if ( buf[0] == '#' ) {
			buf[0] = '\0';
		}

		// �����Υ��ڡ���������
		cut_character_at_linetail(buf, ' ');

		debug_log_output("read_buf(comment cut):'%s'", buf);

		// ���Ԥʤ顢continue
		if ( strlen( buf ) == 0 )
		{
			debug_log_output("continue.");
			continue;
		}

		// Windows�Ѥ˥ץ쥤�ꥹ����Υե�����̾��Ĵ��
		if (global_param.flag_filename_adjustment_for_windows){
			filename_adjustment_for_windows(buf, http_recv_info_p->send_filename);
		}

		// ��ĥ�� ����
		filename_to_extension(buf, file_extension, sizeof(file_extension) );
		debug_log_output("file_extension:'%s'", file_extension);

		// ɽ���ե�����̾ ����
		strncpy(work_data, buf, sizeof(work_data));
		cut_before_last_character( work_data, '/' );
		strncpy( file_name, work_data, sizeof(file_name));
		debug_log_output("file_name:'%s'", file_name);


		// URI����
		if ( buf[0] == '/' ) // ���Хѥ�
		{
			strncpy( file_uri_link, buf, sizeof(file_uri_link) );
		}
		else // ���Хѥ�
		{
			strncpy( file_uri_link, listfile_path, sizeof(file_uri_link) );
			strncat( file_uri_link, buf, sizeof(file_uri_link) - strlen(file_uri_link) );
		}

		debug_log_output("listfile_path:'%s'", listfile_path);
		debug_log_output("file_uri_link:'%s'", file_uri_link);


		// -----------------------------------------
		// ��ĥ�Ҥ�mp3�ʤ顢ID3���������å���
		// -----------------------------------------
		mp3_id3tag_data.mp3_id3v1_flag = 0;
		if ( strcasecmp(file_extension, "mp3" ) == 0 )
		{
			// MP3�ե�����Υե�ѥ�����
			strncpy(work_data, global_param.document_root, sizeof(work_data) );
			if ( work_data[strlen(work_data)-1] == '/' )
				work_data[strlen(work_data)-1] = '\0';
			strncat( work_data, file_uri_link, sizeof(work_data) );

			debug_log_output("full_path(mp3):'%s'", work_data); // �ե�ѥ�

			// ID3���������å�
			memset( &mp3_id3tag_data, 0, sizeof(mp3_id3tag_data));
			mp3_id3_tag_read(work_data , &mp3_id3tag_data );
		}


		// MP3 ID3������¸�ߤ����ʤ�С�playlist ɽ���ե�����̾��ID3�������֤������롣
		if ( mp3_id3tag_data.mp3_id3v1_flag == 1 )
		{
			strncpy(work_data, mp3_id3tag_data.mp3_id3v1_title_info, sizeof(work_data));
			strncat(work_data, ".mp3", sizeof(work_data) - strlen(work_data));	// ���ߡ��γ�ĥ�ҡ�playlist_filename_adjustment()�Ǻ������롣

			// =========================================
			// playlistɽ���� ID3������Ĵ��
			// EUC���Ѵ� �� ��ĥ�Һ�� �� (ɬ�פʤ�)Ⱦ��ʸ�������Ѥ��Ѵ� �� MediaWiz�����ɤ��Ѵ� �� SJIS�ʤ顢����ʸ��������0x7C('|')��ޤ�ʸ��������
			// =========================================
			playlist_filename_adjustment( work_data, file_name, sizeof(file_name), CODE_AUTO );
		}
		else
		// MP3 ID3������¸�ߤ��ʤ��ʤ�С��ե�����̾�򤽤Τޤ޻��Ѥ��롣
		{
			// ---------------------------------
			// ɽ���ե�����̾ Ĵ��
			// EUC���Ѵ� �� ��ĥ�Һ�� �� (ɬ�פʤ�)Ⱦ��ʸ�������Ѥ��Ѵ� �� MediaWiz�����ɤ��Ѵ� �� SJIS�ʤ顢����ʸ��������0x7C('|')��ޤ�ʸ��������
			// ---------------------------------
			strncpy( work_data, file_name, sizeof(work_data) );
			playlist_filename_adjustment(work_data, file_name, sizeof(file_name), global_param.server_language_code);
		}

		debug_log_output("file_name(adjust):'%s'", file_name);

		// ------------------------------------
		// Link��URI(���󥳡��ɺѤ�) ������
		// ------------------------------------
		strncpy(work_data, file_uri_link, sizeof(work_data) );
		uri_encode(file_uri_link, sizeof(file_uri_link), work_data, strlen(work_data) );

		debug_log_output("file_uri_link(encoded):'%s'", file_uri_link);

		// URI�γ�ĥ���֤�����������
		extension_add_rename(file_uri_link, sizeof(file_uri_link));


		// ------------------------------------
		// �ץ쥤�ꥹ�Ȥ�����
		// ------------------------------------
		send_printf(accept_socket, "%s|%d|%d|http://%s%s|\r\n"
			, file_name
			, 0, 0
			, http_recv_info_p->recv_host, file_uri_link
		);
	}

	close( fd );
	return;
}

// *****************************************************
// fd ���飱���ɤ߹���
// �ɤ߹����ʸ������return����롣
// �Ǹ�ޤ��ɤ���顢-1����롣
// *****************************************************
static int file_read_line( int fd, unsigned char *line_buf, int line_buf_size)
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


#define		SKIN_OPTION_MENU_HTML 	"option_menu.html"	// OptionMenu�Υ�����
#define		SKIN_KEYWORD_CURRENT_PATH_LINK_NO_SORT	"<!--WIZD_INSERT_CURRENT_PATH_LINK_NO_SORT-->"	// ��PATH(Sort��������Ѥ�̵��)��LINK�ѡ�URI���󥳡��ɺѤ�

// **************************************************************************
// * ���ץ�����˥塼�����������ֿ�
// **************************************************************************
void http_option_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	SKIN_T	*skin;

	unsigned char	current_uri_link_no_sort[WIZD_FILENAME_MAX];
	unsigned char	current_uri_link[WIZD_FILENAME_MAX];
	unsigned char	work_data[WIZD_FILENAME_MAX];

	int				now_page;
	unsigned char	now_page_str[16];

	// ========================
	// �ִ��ѥǡ�������
	// ========================

	// --------------------------------------------------------------
	// ���ѥ�̾ Link��(Sort��������Ѥ�̵��)������URI���󥳡��ɡ�
	// --------------------------------------------------------------
	uri_encode(current_uri_link_no_sort, sizeof(current_uri_link_no_sort), http_recv_info_p->recv_uri, strlen(http_recv_info_p->recv_uri));
	strncat(current_uri_link_no_sort, "?", sizeof(current_uri_link_no_sort) - strlen(current_uri_link_no_sort)); // '?'���ɲ�

	debug_log_output("OptionMenu: current_uri_link_no_sort='%s'", current_uri_link_no_sort);


	// -----------------------------------------
	// ���ѥ�̾ Link��(Sort�����դ�) ����
	// -----------------------------------------
	uri_encode(current_uri_link, sizeof(current_uri_link), http_recv_info_p->recv_uri, strlen(http_recv_info_p->recv_uri));
	strncat(current_uri_link, "?", sizeof(current_uri_link) - strlen(current_uri_link)); // '?'���ɲ�

	// sort=���ؼ�����Ƥ�����硢���������Ѥ���
	if ( strlen(http_recv_info_p->sort) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
		strncat(current_uri_link, work_data, sizeof(current_uri_link) - strlen(current_uri_link));
	}
	debug_log_output("OptionMenu: current_uri_link='%s'", current_uri_link);


	// ---------------------
	// ���ڡ����� ����
	// ---------------------

	if ( http_recv_info_p->page <= 1 )
		now_page = 1;
	else
		now_page = http_recv_info_p->page;

	// 	���ߤΥڡ��� ɽ����
	snprintf(now_page_str, sizeof(now_page_str), "%d", now_page );


	// ==============================
	// OptionMenu �������ɤ߹���
	// ==============================
	if ((skin = skin_open(SKIN_OPTION_MENU_HTML)) == NULL) {
		return ;
	}

	// ==============================
	// �ִ��¹�
	// ==============================
#define REPLACE(a, b) skin_direct_replace_string(skin, SKIN_KEYWORD_##a, (b))
	REPLACE(SERVER_NAME, SERVER_NAME);
	REPLACE(CURRENT_PATH_LINK, current_uri_link);
	REPLACE(CURRENT_PATH_LINK_NO_SORT, current_uri_link_no_sort);
	REPLACE(CURRENT_PAGE, now_page_str);
#undef REPLACE

	// =================
	// �ֿ��¹�
	// =================

	http_send_ok_header(accept_socket, 0, NULL);
	skin_direct_send(accept_socket, skin);
	skin_close(skin);

	return;
}



/********************************************************************************/
// ���ܸ�ʸ���������Ѵ���
// (libnkf�Υ�åѡ��ؿ�)
//
//	���ݡ��Ȥ���Ƥ�������ϰʲ����̤ꡣ
//		in_flag:	CODE_AUTO, CODE_SJIS, CODE_EUC, CODE_UTF8, CODE_UTF16
//		out_flag: 	CODE_SJIS, CODE_EUC
/********************************************************************************/
void convert_language_code(const unsigned char *in, unsigned char *out, size_t len, int in_flag, int out_flag)
{
	unsigned char	nkf_option[128];

	memset(nkf_option, '\0', sizeof(nkf_option));


	//=====================================================================
	// in_flag, out_flag��ߤơ�libnkf�ؤΥ��ץ������Ȥ�Ω�Ƥ롣
	//=====================================================================
	switch( in_flag & 0xf )
	{
		case CODE_SJIS:
			strncpy(nkf_option, "S", sizeof(nkf_option));
			break;

		case CODE_EUC:
			strncpy(nkf_option, "E", sizeof(nkf_option));
			break;

		case CODE_UTF8:
			strncpy(nkf_option, "W", sizeof(nkf_option));
			break;

		case CODE_UTF16:
			strncpy(nkf_option, "W16", sizeof(nkf_option));
			break;

		case CODE_AUTO:
		default:
			strncpy(nkf_option, "", sizeof(nkf_option));
			break;
	}


	switch( out_flag )
	{
		case CODE_EUC:
			strncat(nkf_option, "e", sizeof(nkf_option) - strlen(nkf_option) );
			break;

		case CODE_SJIS:
		default:
			strncat(nkf_option, "s", sizeof(nkf_option) - strlen(nkf_option) );
			break;
	}

	// SAMBA��CAP/HEXʸ���Ѵ��⡢nkf��Ǥ���롣
	if (global_param.flag_decode_samba_hex_and_cap == TRUE && (in_flag & CODE_HEX)) {
		strncat(nkf_option, " --cap-input --url-input", sizeof(nkf_option) - strlen(nkf_option) );
	}

	//=================================================
	// libnkf �¹�
	//=================================================
	debug_log_output("nkf %s '%s'", nkf_option, in);
	nkf(in, out, len, nkf_option);
	debug_log_output("nkf result '%s'", out);




	return;
}

static void  mp3_id3_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p )
{
	skin_rep_data_line_p->mp3_id3v1_flag = 0;

	if (!global_param.flag_read_mp3_tag) return;

	if (mp3_id3v2_tag_read(mp3_filename, skin_rep_data_line_p) != 0) {
		mp3_id3v1_tag_read(mp3_filename, skin_rep_data_line_p);
	}
	if ( skin_rep_data_line_p->mp3_id3v1_flag > 0 ) {
		if ( skin_rep_data_line_p->mp3_id3v1_title[0] == '\0' ) {
			skin_rep_data_line_p->mp3_id3v1_flag = 0;
		} else {
			generate_mp3_title_info(skin_rep_data_line_p);
		}
	}
}

// **************************************************************************
// * MP3�ե����뤫�顢ID3v1�����Υ����ǡ���������
// **************************************************************************
static void  mp3_id3v1_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p )
{
	int	fd;
	unsigned char	buf[128];
	off_t		length;

	memset(buf, '\0', sizeof(buf));

	fd = open(mp3_filename,  O_RDONLY);
	if ( fd < 0 )
	{
		return;
	}

	// �Ǹ夫��128byte��SEEK
	length = lseek(fd, -128, SEEK_END);


	// ------------------
	// "TAG"ʸ�����ǧ
	// ------------------

	// 3byte��read.
	read(fd, buf, 3);
	// debug_log_output("buf='%s'", buf);

	// "TAG" ʸ��������å�
	if ( strncmp( buf, "TAG", 3 ) != 0 )
	{
		debug_log_output("NO ID3 Tag.");

		close(fd);
		return;		// MP3 ����̵����
	}


	// ------------------------------------------------------------
	// Tag����read
	//
	//	ʸ����Ǹ��0xFF��' '���դ��Ƥ���������
	//  clientʸ�������ɤ��Ѵ���
	// ------------------------------------------------------------


	// ��̾
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 30);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_title,
							sizeof(skin_rep_data_line_p->mp3_id3v1_title),
							CODE_AUTO, global_param.client_language_code);


	// �����ƥ�����
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 30);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_artist,
							sizeof(skin_rep_data_line_p->mp3_id3v1_artist),
							CODE_AUTO, global_param.client_language_code);

	// ����Х�̾
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 30);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_album,
							sizeof(skin_rep_data_line_p->mp3_id3v1_album),
							CODE_AUTO, global_param.client_language_code);

	// ����ǯ��
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 4);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_year,
							sizeof(skin_rep_data_line_p->mp3_id3v1_year),
							CODE_AUTO, global_param.client_language_code);

	// ������
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 28);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_comment,
							sizeof(skin_rep_data_line_p->mp3_id3v1_comment),
							CODE_AUTO, global_param.client_language_code);

	// ---------------------
	// ¸�ߥե饰
	// ---------------------
	skin_rep_data_line_p->mp3_id3v1_flag = 1;

	close(fd);
}

static unsigned long id3v2_len(unsigned char *buf)
{
	return buf[0] * 0x200000 + buf[1] * 0x4000 + buf[2] * 0x80 + buf[3];
}

// **************************************************************************
// * MP3�ե����뤫�顢ID3v2�����Υ����ǡ���������
// * 0: ����  -1: ����(�����ʤ�)
// **************************************************************************
static int  mp3_id3v2_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p )
{
	int	fd;
	unsigned char	buf[1024];
	unsigned char	*frame;
	off_t		len;
	struct _copy_list {
		unsigned char id[5];
		unsigned char *container;
		size_t maxlen;
	} copy_list[] = {
		{ "TIT2", skin_rep_data_line_p->mp3_id3v1_title
			, sizeof(skin_rep_data_line_p->mp3_id3v1_title) },
		{ "TPE1", skin_rep_data_line_p->mp3_id3v1_artist
			, sizeof(skin_rep_data_line_p->mp3_id3v1_artist) },
		{ "TALB", skin_rep_data_line_p->mp3_id3v1_album
			, sizeof(skin_rep_data_line_p->mp3_id3v1_album) },
		{ "TCOP", skin_rep_data_line_p->mp3_id3v1_year
			, sizeof(skin_rep_data_line_p->mp3_id3v1_year) },
		{ "TYER", skin_rep_data_line_p->mp3_id3v1_year
			, sizeof(skin_rep_data_line_p->mp3_id3v1_year) },
		{ "COMM", skin_rep_data_line_p->mp3_id3v1_comment
			, sizeof(skin_rep_data_line_p->mp3_id3v1_comment) },
	};
	int list_count = sizeof(copy_list) / sizeof(struct _copy_list);
	int i;
	int flag_extension = 0;


	memset(buf, '\0', sizeof(buf));

	fd = open(mp3_filename,  O_RDONLY);
	if ( fd < 0 )
	{
		return -1;
	}

	// ------------------
	// "ID3"ʸ�����ǧ
	// ------------------

	// 10byte��read.
	read(fd, buf, 10);
	// debug_log_output("buf='%s'", buf);

	// "ID3" ʸ��������å�
	if ( strncmp( buf, "ID3", 3 ) != 0 )
	{
		/*
		 *  �ե�����θ��ˤ��äĤ��Ƥ� ID3v2 �����Ȥ�
		 *  �ե����������ˤ���ΤȤ� ���ݤ����� �ɤޤʤ��衣
		 */
		debug_log_output("NO ID3v2 Tag.");

		close(fd);
		return -1;		// v2 ����̵����
	}
	debug_log_output("ID3 v2.%d.%d Tag found", buf[3], buf[4]);
	debug_log_output("ID3 flag: %02X", buf[5]);
	if (buf[5] & 0x40) {
		debug_log_output("ID3 flag: an extended header exist.");
		flag_extension = 1;
	}
	len = id3v2_len(buf + 6);

	if (flag_extension) {
		int exlen;
		if (read(fd, buf, 6) != 6) {
			close(fd);
			return -1;
		}
		exlen = id3v2_len(buf);
		debug_log_output("ID3 ext. flag: len = %d", exlen);
		if (exlen < 6) {
			debug_log_output("invalid ID3 ext. header.");
			close(fd);
			return -1;
		} else if (exlen > 6) {
			debug_log_output("large ID3 ext. header.");
			lseek(fd, exlen - 6, SEEK_CUR);
		}
		len -= exlen;
	}

	// ------------------------------------------------------------
	// Tag����read
	//
	//  clientʸ�������ɤ��Ѵ���
	// ------------------------------------------------------------

	while (len > 0) {
		int frame_len;

		/* �ե졼��إå��� �ɤ߹��� */
		if (read(fd, buf, 10) != 10) {
			close(fd);
			return -1;
		}

		/* �ե졼���Ĺ���򻻽� */
		frame_len = id3v2_len(buf + 4);

		/* �ե졼��Ǹ�ޤ� ���ɤ�Ĥ��� */
		if (frame_len == 0 || *(unsigned long*)buf == 0) break;

		for (i=0; i<list_count; i++) {
			if (!strncmp(buf, copy_list[i].id, 4)) break;
		}
		if (i < list_count) {
			// ��᤹�륿�� ȯ��

			// ¸�ߥե饰
			skin_rep_data_line_p->mp3_id3v1_flag = 1;

			frame = malloc(frame_len + 1);
			memset(frame, '\0', frame_len + 1);
			if (read(fd, frame, frame_len) != frame_len) {
				debug_log_output("ID3v2 Tag[%s] read failed", copy_list[i].id);
				free(frame);
				close(fd);
				return -1;
			}
			debug_log_output("ID3v2 Tag[%s] found. '%s'", copy_list[i].id, frame + 1);
			cut_character_at_linetail(frame + 1, ' ');
			convert_language_code(	frame + 1,
									copy_list[i].container,
									copy_list[i].maxlen,
									CODE_AUTO,
									global_param.client_language_code);
			free(frame);
		} else {
			/* �ޥå����ʤ��ä� */
			buf[4] = '\0';
			debug_log_output("ID3v2 Tag[%s] skip", buf);
			lseek(fd, frame_len, SEEK_CUR);
		}
		len -= (frame_len + 10); /* �ե졼������ + �ե졼��إå� */
	}

	close(fd);
	return skin_rep_data_line_p->mp3_id3v1_flag ? 0 : -1;
}

static void generate_mp3_title_info( SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p )
{
	unsigned char	work_title_info[128];
	memset(work_title_info, '\0', sizeof(work_title_info));

	// ---------------------
	// mp3_title_info����
	// ---------------------
	strncpy(work_title_info, skin_rep_data_line_p->mp3_id3v1_title, sizeof(work_title_info));


	if ( (strlen(skin_rep_data_line_p->mp3_id3v1_album) > 0) && (strlen(skin_rep_data_line_p->mp3_id3v1_artist) > 0) )
	{
		strncat(work_title_info, " [", 									sizeof(work_title_info) - strlen(work_title_info));
		strncat(work_title_info, skin_rep_data_line_p->mp3_id3v1_album, sizeof(work_title_info) - strlen(work_title_info));
		strncat(work_title_info, "/", 									sizeof(work_title_info) - strlen(work_title_info));
		strncat(work_title_info, skin_rep_data_line_p->mp3_id3v1_artist,sizeof(work_title_info) - strlen(work_title_info));
		strncat(work_title_info, "]", 									sizeof(work_title_info) - strlen(work_title_info));
	}
	else if ( (strlen(skin_rep_data_line_p->mp3_id3v1_album) ==0) && (strlen(skin_rep_data_line_p->mp3_id3v1_artist) > 0) )
	{
		strncat(work_title_info, " [", 									sizeof(work_title_info) - strlen(work_title_info));
		strncat(work_title_info, skin_rep_data_line_p->mp3_id3v1_artist,sizeof(work_title_info) - strlen(work_title_info));
		strncat(work_title_info, "]", 									sizeof(work_title_info) - strlen(work_title_info));
	}
	else if ( (strlen(skin_rep_data_line_p->mp3_id3v1_album) > 0) && (strlen(skin_rep_data_line_p->mp3_id3v1_artist) ==0) )
	{
		strncat(work_title_info, " [", 									sizeof(work_title_info) - strlen(work_title_info));
		strncat(work_title_info, skin_rep_data_line_p->mp3_id3v1_album, sizeof(work_title_info) - strlen(work_title_info));
		strncat(work_title_info, "]", 									sizeof(work_title_info) - strlen(work_title_info));
	}

	// mp3_title_info (����̵��)����¸��
	strncpy(skin_rep_data_line_p->mp3_id3v1_title_info, work_title_info, sizeof(skin_rep_data_line_p->mp3_id3v1_title_info) );

	// EUC���Ѵ�
	convert_language_code( 	work_title_info,
							skin_rep_data_line_p->mp3_id3v1_title_info,
							sizeof(skin_rep_data_line_p->mp3_id3v1_title_info),
							CODE_AUTO, CODE_EUC);
	strncpy(work_title_info, skin_rep_data_line_p->mp3_id3v1_title_info, sizeof(work_title_info));


	// CUT�¹�
	euc_string_cut_n_length(work_title_info, global_param.menu_filename_length_max);
	debug_log_output("mp3_title_info(cut)='%s'\n", work_title_info);

	// ���饤�����ʸ�������ɤˡ�
	convert_language_code( 	work_title_info,
							skin_rep_data_line_p->mp3_id3v1_title_info_limited,
							sizeof(skin_rep_data_line_p->mp3_id3v1_title_info_limited),
							CODE_EUC, global_param.client_language_code);

	return;
}


// *************************************************************************************
// playlist���Ϥ�ɽ���ե�����̾������ʤ�������Ĵ������
// *************************************************************************************
static void playlist_filename_adjustment(unsigned char *src_name, unsigned char *dist_name, int dist_size, int input_code )
{
	unsigned char	work_data[WIZD_FILENAME_MAX];


	// ---------------------------------
	// �ե�����̾ ���� (ɽ����)
	// EUC���Ѵ� �� ��ĥ�Һ�� �� (ɬ�פʤ�)Ⱦ��ʸ�������Ѥ��Ѵ� �� MediaWiz�����ɤ��Ѵ� �� SJIS�ʤ顢����ʸ��������0x7C('|')��ޤ�ʸ�����������ˡ�
	// ---------------------------------
	convert_language_code(	src_name,
							dist_name,
							dist_size,
							input_code | CODE_HEX,
							CODE_EUC);

	if ( strchr(dist_name, '.') != NULL )
	{
		cut_after_last_character(dist_name, '.');
	}
	debug_log_output("dist_name(euc)='%s'\n", dist_name);

	if (global_param.flag_allplay_filelist_adjust == TRUE)
	{
		han2euczen(dist_name, work_data, sizeof(work_data) );
		debug_log_output("dist_name(euczen)='%s'\n", work_data);
	}
	else
	{
		strncpy(work_data, dist_name, sizeof(work_data));
	}

	convert_language_code( work_data, dist_name, dist_size, CODE_EUC, global_param.client_language_code);
	debug_log_output("dist_name(MediaWiz Code)='%s'\n", dist_name);

	if ( global_param.client_language_code == CODE_SJIS )
	{
		sjis_code_thrust_replase(dist_name, '|');
		debug_log_output("dist_name(SJIS '|' replace)='%s'\n", dist_name);
	}
	replase_character(dist_name, dist_size, "|", "!");


	return;
}

void http_uri_to_scplaylist_create(int accept_socket, char *uri_string)
{
	// HTTP_OK�إå�����
	http_send_ok_header(accept_socket, 0, "audio/x-scpls");

	// WinAMP���ѥץ쥤�ꥹ������
	send_printf(accept_socket, "[playlist]\n");
	send_printf(accept_socket, "numberofentries=1\n");
	send_printf(accept_socket, "File1=%s\n", uri_string);
	send_printf(accept_socket, "Length1=-1\n");

	debug_log_output("http_uri_to_scplaylist_create: URI %s\n", uri_string);

	return;
}


// Windows�Ѥ˥ץ쥤�ꥹ����Υե�����̾��Ĵ��
//
// ����Ū�ˤϡ��ʲ���Ԥ�
// ���ѥ����ڤ��'\'��'/'���ѹ�
// ��Windows��case insensitive�ʤΤ��б�
//
// �ʤ���playlist_filename_adjustment�Ȥ���ɽ��ʸ�����Ѵؿ������뤬��Ʊ���ʤ�����!!!
static void filename_adjustment_for_windows(unsigned char *filename, const unsigned char *pathname_plw)
{
#define isleadbyte(c)	((0x81 <= (unsigned char)(c) && (unsigned char)(c) <= 0x9f) || (0xe0 <= (unsigned char)(c) && (unsigned char)(c) <= 0xfc))
	unsigned char pathname[WIZD_FILENAME_MAX];
	unsigned char basename[WIZD_FILENAME_MAX];
	unsigned char *curp;
	int sjis_f;

	if (filename == NULL || *filename == '\0')
		return;

	debug_log_output("filename_adjustment_for_windows: filename = [%s], pathname_plw = [%s]\n", filename, pathname_plw );

	// �ѥ����ڤ��'\'��'/'���ѹ�
	// sjis�Ǥ��뤳�Ȥ�����ˤ��������Ǥ�
	curp = filename;
	sjis_f = FALSE;
	while (*curp){
		if (!sjis_f){
			if (isleadbyte(*curp)){
				// sjis��1byte��
				sjis_f = TRUE;
			} else if (*curp == '\\'){
				*curp = '/';			// �֤�����
			}
		} else {
			// sjis��2byte��
			sjis_f = FALSE;
		}
		curp++;
	}


	// ��Ƭ��������ܤΥǥ��쥯�ȥ�̾�����
	if (*filename == '/'){
		// ���Хѥ�
		pathname[0] = '\0';
		curp = filename + 1;
	} else {
		// ���Хѥ�
		strncpy(pathname, pathname_plw, sizeof(pathname));
		cut_after_last_character(pathname, '/');
		curp = filename;
	}
	// ���Υ١���̾�����
	strncpy(basename, curp, sizeof(basename));
	cut_after_character(basename, '/');


	// �ǥ��쥯�ȥ곬����Υ롼��
	while (1){
		int found = FALSE;
		DIR *dir;
		struct dirent *dent;

		// �ǥ��쥯�ȥ꤫��case insensitive��õ��
		debug_log_output("  SEARCH (case-insensitive). pathname = [%s], basename = [%s]\n", pathname, basename );

		dir = opendir(pathname);
		if ( dir == NULL ){
			debug_log_output("Can't Open dir. pathname = [%s]\n", pathname );
			break;
		}

		// �ǥ��쥯�ȥ���Υ롼��
		while (1){
			dent = readdir(dir);
			if (dent == NULL){
				// ���Ĥ���ʤ�(�����餯�ץ쥤�ꥹ�Ȥε��ҥߥ�)
				debug_log_output("  NOT FOUND!!! [%s]\n", basename);
				break;
			}
			debug_log_output("    [%s]\n", dent->d_name);
			if (strcasecmp(dent->d_name, basename) == 0){
				// ���Ĥ���
				debug_log_output("  FOUND!!! [%s]->[%s]\n", basename, dent->d_name);
				strncpy(curp, dent->d_name, strlen(dent->d_name));		// ������ʬ�������̾�����֤�����
				strncpy(basename, dent->d_name, sizeof(basename));
				found = TRUE;
				break;
			}
		}

		closedir(dir);

		if (found){
			// ���γ��ؤ˿ʤ�
			strncat(pathname, "/", sizeof(pathname));
			strncat(pathname, basename, sizeof(pathname));

			curp += strlen(basename);
			if (*curp == '\0'){
				// ��λ
				debug_log_output("Loop end.\n");
				break;
			}
			curp++;
			strncpy(basename, curp, sizeof(basename));
			cut_after_character(basename, '/');
			if (*basename == '\0'){
				// �Ǹ夬'/'�ʤΤ��Ѥ����ɡ���λ
				debug_log_output("Loop end ? (/)\n");
				break;
			}
		} else {
			// ��ɸ��Ĥ���ʤ��ä�
			// �ɤ��������Ǥ��ʤ������Ȥꤢ�������Τޤ�...
			debug_log_output("NOT Found. pathname = %s, basename = [%s]\n", pathname, basename );
			break;
		}
	}

	debug_log_output("filename_adjustment_for_windows: EXIT. filename = [%s]\n", filename );
}

static int read_avi_info(char *fname, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p)
{
	FILE *fp;
	MainAVIHeader avih;
	AVIStreamHeader avish;
	int len, listlen;
	time_t t;
	FOURCC fcc;

	fp = fopen(fname, "rb");
	if (fp == NULL) return -1;

	listlen = read_avi_main_header(fp, &avih);
	if (listlen < 0) return -1;

	snprintf(skin_rep_data_line_p->image_width, sizeof(skin_rep_data_line_p->image_width), "%lu", avih.dwWidth );
	snprintf(skin_rep_data_line_p->image_height, sizeof(skin_rep_data_line_p->image_height), "%lu", avih.dwHeight );

	snprintf(skin_rep_data_line_p->avi_fps, sizeof(skin_rep_data_line_p->avi_fps), "%.3f", 1000000.0/avih.dwMicroSecPerFrame);

	t = (time_t)((double)avih.dwTotalFrames * avih.dwMicroSecPerFrame / 1000000);
	snprintf(skin_rep_data_line_p->avi_duration, sizeof(skin_rep_data_line_p->avi_duration), "%lu:%02lu:%02lu", t/3600, (t/60)%60, t%60);

	while ((len = read_next_chunk(fp, &fcc)) > 0) {
		//debug_log_output("--- %s\n", str_fourcc(fcc));
		if (fcc != SYM_LIST) break;
		if (read_avi_stream_header(fp, &avish, len) < 0) break;
		//debug_log_output("fccType: %s\n", str_fourcc(avish.fccType));
		//debug_log_output("fccHandler: %s (%s)\n", str_fourcc(avish.fccHandler), str_fourcc(avish.dwReserved1));
		//debug_log_output("rate: %d\n", avish.dwRate);

		if (avish.fccType == SYM_VIDS) {
			snprintf(skin_rep_data_line_p->avi_vcodec, sizeof(skin_rep_data_line_p->avi_hvcodec), "%s", str_fourcc(avish.fccHandler));
			snprintf(skin_rep_data_line_p->avi_hvcodec, sizeof(skin_rep_data_line_p->avi_vcodec), "%s", str_fourcc(avish.dwReserved1));
		} else if (avish.fccType == SYM_AUDS) {
			snprintf(skin_rep_data_line_p->avi_acodec, sizeof(skin_rep_data_line_p->avi_hacodec), "%s", str_acodec(avish.fccHandler));
			snprintf(skin_rep_data_line_p->avi_hacodec, sizeof(skin_rep_data_line_p->avi_acodec), "%s", str_acodec(avish.dwReserved1));
		}
	}

	// Interleave check :)
	do {
		DWORD riff_type;
		if (fcc == SYM_LIST) {
			if ((riff_type = read_next_sym(fp)) == -1) {
				return -1;
			}
			len -= sizeof(riff_type);
			if (riff_type == SYM_MOVI) {
				DWORD old = 0;
				int i;
				for (i=0; i<100 && (len = read_next_chunk(fp, &fcc))>=0; i++) {
					len = (len + 1)/ 2 * 2;
					fseek(fp, len, SEEK_CUR);

					/* mask BE value '01wb' into '01__', in LE. */
					if (i != 0 && (fcc & 0x0000ffff) != old) break;
					old = fcc & 0x0000ffff;
				}
				strncpy(skin_rep_data_line_p->avi_is_interleaved, (i<100) ? "I" : "NI", sizeof(skin_rep_data_line_p->avi_is_interleaved));
				break;
			}
		}

		fseek(fp, len, SEEK_CUR);
	} while ((len = read_next_chunk(fp, &fcc)) > 0);


	fclose(fp);

	return 0;
}
