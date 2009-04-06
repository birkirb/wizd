//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_menu.c
//											$Revision: 1.53 $
//											$Date: 2004/12/18 15:24:50 $
//
//	すべて自己責任でおながいしまつ。
//  このソフトについてVertexLinkに問い合わせないでください。
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
	unsigned char	name[WIZD_FILENAME_MAX];		// 表示用ファイル名(EUC)
	unsigned char	org_name[WIZD_FILENAME_MAX];	// オリジナルファイル名
	unsigned char	ext[32];						// オリジナルファイル拡張子
	mode_t			type;			// 種類
	off_t			size;			// サイズ
	// use off_t instead of size_t, since it contains st_size of stat.
	time_t			time;			// 日付
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


// ファイルソート用関数配列
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


// ディレクトリソート用関数配列
static void * dir_sort_api[] = {
	NULL,
	_file_info_dir_sort_order_up,
	_file_info_dir_sort_order_down
};




// **************************************************************************
// ファイルリストを生成して返信
// **************************************************************************
void http_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, int flag_pseudo)
{
	int				file_num;	// DIR内のファイル数
	int				sort_rule;  // temp置き換え用
	unsigned char	*file_info_malloc_p;
	FILE_INFO_T		*file_info_p;

	if (!flag_pseudo) {
		// recv_uri の最後が'/'でなかったら、'/'を追加
		if (( strlen(http_recv_info_p->recv_uri) > 0 ) &&
			( http_recv_info_p->recv_uri[strlen(http_recv_info_p->recv_uri)-1] != '/' ))
		{
			strncat(http_recv_info_p->recv_uri, "/", sizeof(http_recv_info_p->recv_uri) - strlen(http_recv_info_p->recv_uri) );
		}


		//  http_recv_info_p->send_filename の最後が'/'でなかったら、'/'を追加
		if (( strlen(http_recv_info_p->send_filename) > 0 ) &&
			( http_recv_info_p->send_filename[strlen(http_recv_info_p->send_filename)-1] != '/' ))
		{
			strncat(http_recv_info_p->send_filename, "/", sizeof(http_recv_info_p->send_filename) - strlen(http_recv_info_p->send_filename) );
		}
	}


	// ==================================
	// ディレクトリ情報をＧＥＴ
	// ==================================

	if (flag_pseudo) {
		// recv_uri ディレクトリのファイル数を数える。
		file_num = count_file_num_in_tsv( http_recv_info_p->send_filename );
	} else {
		// recv_uri ディレクトリのファイル数を数える。
		file_num = count_file_num( http_recv_info_p->send_filename );
	}

	debug_log_output("file_num = %d", file_num);
	if ( file_num < 0 )
	{
		return;
	}


	// 必要な数だけ、ファイル情報保存エリアをmalloc()
	file_info_malloc_p = malloc( sizeof(FILE_INFO_T)*file_num );
	if ( file_info_malloc_p == NULL )
	{
		debug_log_output("malloc() error");
		return;
	}


	memset(file_info_malloc_p, 0, sizeof(FILE_INFO_T)*file_num);
	file_info_p = (FILE_INFO_T *)file_info_malloc_p;



	// -----------------------------------------------------
	// ファイル情報保存エリアに、ディレクトリ情報を読み込む。
	// -----------------------------------------------------
	if (flag_pseudo) {
		file_num = tsv_stat(http_recv_info_p->send_filename, file_info_p, file_num);
	} else {
		file_num = directory_stat(http_recv_info_p->send_filename, file_info_p, file_num);
	}
	debug_log_output("file_num = %d", file_num);


	// デバッグ。file_info_malloc_p 表示
	//for ( i=0; i<file_num; i++ )
	//{
	//	debug_log_output("file_info[%d] name='%s'", i, file_info_p[i].name );
	//	debug_log_output("file_info[%d] size='%d'", i, file_info_p[i].size );
	//	debug_log_output("file_info[%d] time='%d'", i, file_info_p[i].time );
	//}


	// ---------------------------------------------------
	// sort=でソートが指示されているか確認
	// 指示されていたら、それでglobal_paramを上書き
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
	// 特定ディレクトリでのSORT方法を固定
		unsigned char	*p;
		if ((p=strstr(http_recv_info_p->send_filename,"/video_ts/"))!=NULL) {
			if (*(p+strlen("/video_ts/"))=='\0')	//末尾?
				sort_rule = SORT_NAME_UP;
		}
		if ((p=strstr(http_recv_info_p->send_filename,"/TIMESHIFT/"))!=NULL) {
			if (*(p+strlen("/TIMESHIFT/"))=='\0')	//末尾?
				sort_rule = SORT_TIME_UP;
		}
		//if (strcmp(http_recv_info_p->send_filename,"video")==0)
		//	sort_rule = SORT_TIME_DOWN;
		// アーティスト、年、アルバム名、トラック+Sortクライテリア
		//
	}

	// 必要ならば、ソート実行
	if ( sort_rule != SORT_NONE )
	{
		file_info_sort( file_info_p, file_num, sort_rule | SORT_DIR_UP );
	}

	// -------------------------------------------
	// 自動再生
	// -------------------------------------------
	if ( strcasecmp(http_recv_info_p->action, "allplay") == 0 )
	{
		create_all_play_list(accept_socket, http_recv_info_p, file_info_p, file_num);
		debug_log_output("AllPlay List Create End!!! ");
		return ;
	}


	// -------------------------------------------
	// ファイルメニュー生成＆送信
	// -------------------------------------------
	create_skin_filemenu(accept_socket, http_recv_info_p, file_info_p, file_num);

	free(file_info_malloc_p);

	return;
}





// ****************************************************************************************
// メニュー生成用のdefineいろいろ。
// ****************************************************************************************



// **************************************************************************
// スキンを使用したファイルメニューを生成
// **************************************************************************
static void create_skin_filemenu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num)
{
	int		i;

	unsigned char	work_filename[WIZD_FILENAME_MAX];

	unsigned char	work_data[WIZD_FILENAME_MAX];
	unsigned char	work_data2[WIZD_FILENAME_MAX];

	int		now_page;		// 現在のページ番号
	int		max_page;		// 最大ページ番号
	int		now_page_line;	// 現在のページの表示行数
	int		start_file_num;	// 現在ページの表示開始ファイル番号
	int		end_file_num;	// 現在ページの表示終了ファイル番号

	int		next_page;		// 次のページ(無ければmax_page)
	int		prev_page;		// 一つ前のページ（無ければ 1)


	SKIN_REPLASE_GLOBAL_DATA_T	*skin_rep_data_global_p;
	SKIN_REPLASE_LINE_DATA_T	*skin_rep_data_line_p;

	int skin_rep_line_malloc_size;

	unsigned int	rec_time;
	int	count;

	unsigned int	image_width, image_height;

	struct	stat	dir_stat;
	int				result;

	// ==========================================
	// SKINコンフィグファイル読み込む
	// ==========================================

	skin_read_config(SKIN_MENU_CONF);


	// ==========================================
	// HTML生成準備 各種計算等
	// ==========================================

	// ディレクトリ存在ファイル数
	debug_log_output("file_num = %d", file_num);

	// 最大ページ数計算
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

	// 現在表示ページ番号 計算。
	if ( (http_recv_info_p->page <= 1 ) || (max_page < http_recv_info_p->page ) )
		now_page = 1;
	else
		now_page = http_recv_info_p->page;

	debug_log_output("now_page = %d", now_page);

	// 現在表示ページの表示行数計算。
	if ( max_page == now_page ) // 最後のページ
		now_page_line = file_num - (global_param.page_line_max * (max_page-1));
	else	// 最後以外なら、表示最大数。
		now_page_line = global_param.page_line_max;
	debug_log_output("now_page_line = %d", now_page_line);


	// 表示開始ファイル番号計算
	start_file_num = ((now_page - 1) * global_param.page_line_max);
	debug_log_output("start_file_num = %d", start_file_num);

	if ( max_page == now_page ) // 最後のページ
		end_file_num = file_num;
	else // 最後のページではなかったら。
		end_file_num = (start_file_num + global_param.page_line_max);
	debug_log_output("start_file_num = %d", start_file_num);




	// 前ページ番号 計算
	prev_page =  1 ;
	if ( now_page > 1 )
		prev_page = now_page - 1;

	// 次ページ番号 計算
	next_page = max_page ;
	if ( max_page > now_page )
		next_page = now_page + 1;

	debug_log_output("prev_page=%d  next_page=%d", prev_page ,next_page);



	// ===============================
	// スキン置換用データを準備
	// ===============================

	// 作業エリア確保
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
	// グローバル 表示用情報 生成開始
	// ---------------------------------

	// 現パス名 表示用生成(recv_uri -> current_path_name [clientコード])
	convert_language_code(	http_recv_info_p->recv_uri,
							skin_rep_data_global_p->current_path_name,
							sizeof(skin_rep_data_global_p->current_path_name),
							global_param.server_language_code | CODE_HEX,
							global_param.client_language_code );
	debug_log_output("current_path = '%s'", skin_rep_data_global_p->current_path_name );

	// -----------------------------------------------
	// 親ディレクトリ用を生成
	// -----------------------------------------------
	if (get_parent_path(work_data, http_recv_info_p->recv_uri, sizeof(work_data)) == NULL) {
		debug_log_output("FATAL ERROR! too long recv_uri.");
		return ;
	}
	debug_log_output("parent_directory='%s'", work_data);

	// 一つ上のディレクトリパスをURIエンコード。
	uri_encode(skin_rep_data_global_p->parent_directory_link, sizeof(skin_rep_data_global_p->parent_directory_link), work_data, strlen(work_data));
	// '?'を追加
	strncat(skin_rep_data_global_p->parent_directory_link, "?", sizeof(skin_rep_data_global_p->parent_directory_link) - strlen(skin_rep_data_global_p->parent_directory_link));
	// sort=が指示されていた場合、それを引き継ぐ。
	if ( strlen(http_recv_info_p->sort) > 0 ) {
		snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
		strncat(skin_rep_data_global_p->parent_directory_link, work_data, sizeof(skin_rep_data_global_p->parent_directory_link) - strlen(skin_rep_data_global_p->parent_directory_link));
	}
	debug_log_output("parent_directory_link='%s'", skin_rep_data_global_p->parent_directory_link);

	// 親ディレクトリ名 表示用
	convert_language_code(	work_data,
							skin_rep_data_global_p->parent_directory_name,
							sizeof(skin_rep_data_global_p->parent_directory_name),
							global_param.server_language_code | CODE_HEX,
							global_param.client_language_code );
	debug_log_output("parent_directory_name='%s'", skin_rep_data_global_p->parent_directory_link);


	// -----------------------------------------------
	// 次に現ディレクトリ用を生成
	// -----------------------------------------------

	// 現ディレクトリ名 を生成。まずはEUCに変換。
	convert_language_code(	http_recv_info_p->recv_uri,
							work_data,
							sizeof(work_data),
							global_param.server_language_code | CODE_HEX,
							CODE_EUC );

	// 最後に'/'が付いていたら削除
	cut_character_at_linetail(work_data, '/');
	// '/'より前を削除
	cut_before_last_character(work_data, '/');
	// '/'を追加。
	strncat(work_data, "/", sizeof(work_data) - strlen(work_data) );

	// CUT実行
	euc_string_cut_n_length(work_data, global_param.menu_filename_length_max);

	// 現ディレクトリ名 表示用生成(文字コード変換)
	convert_language_code(	work_data,
							skin_rep_data_global_p->current_directory_name,
							sizeof(skin_rep_data_global_p->current_directory_name),
							CODE_EUC,
							global_param.client_language_code );

	debug_log_output("current_dir = '%s'", skin_rep_data_global_p->current_directory_name );


	// 現パス名 Link用生成（URIエンコード from recv_uri）
	uri_encode(skin_rep_data_global_p->current_directory_link_no_param
		, sizeof(skin_rep_data_global_p->current_directory_link_no_param)
		, http_recv_info_p->recv_uri
		, strlen(http_recv_info_p->recv_uri)
	);
	strncpy(skin_rep_data_global_p->current_directory_link
		, skin_rep_data_global_p->current_directory_link_no_param
		, sizeof(skin_rep_data_global_p->current_directory_link)
	);

	// ?を追加
	strncat(skin_rep_data_global_p->current_directory_link, "?", sizeof(skin_rep_data_global_p->current_directory_link) - strlen(skin_rep_data_global_p->current_directory_link)); // '?'を追加
	// sort=が指示されていた場合、それを引き継ぐ。
	if ( strlen(http_recv_info_p->sort) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
		strncat(skin_rep_data_global_p->current_directory_link, work_data, sizeof(skin_rep_data_global_p->current_directory_link) - strlen(skin_rep_data_global_p->current_directory_link));
	}
	debug_log_output("current_directory_link='%s'", skin_rep_data_global_p->current_directory_link);

	// ------------------------------------ ディレクトリ表示用処理 ここまで

	// ディレクトリ存在ファイル数 表示用
	snprintf(skin_rep_data_global_p->file_num_str, sizeof(skin_rep_data_global_p->file_num_str), "%d", file_num );

	// 	現在のページ 表示用
	snprintf(skin_rep_data_global_p->now_page_str, sizeof(skin_rep_data_global_p->now_page_str), "%d", now_page );

	// 全ページ数 表示用
	snprintf(skin_rep_data_global_p->max_page_str, sizeof(skin_rep_data_global_p->max_page_str), "%d", max_page );

	// 次のページ 表示用
	snprintf(skin_rep_data_global_p->next_page_str, sizeof(skin_rep_data_global_p->next_page_str), "%d", next_page );

	// 前のページ 表示用
	snprintf(skin_rep_data_global_p->prev_page_str, sizeof(skin_rep_data_global_p->prev_page_str), "%d", prev_page );

	// 開始ファイル番号表示用
	if ( file_num == 0 )
		snprintf(skin_rep_data_global_p->start_file_num_str, sizeof(skin_rep_data_global_p->start_file_num_str), "%d", start_file_num );
	else
		snprintf(skin_rep_data_global_p->start_file_num_str, sizeof(skin_rep_data_global_p->start_file_num_str), "%d", start_file_num +1 );

	// 終了ファイル番号表示用
	snprintf(skin_rep_data_global_p->end_file_num_str, sizeof(skin_rep_data_global_p->end_file_num_str), "%d", end_file_num  );


	skin_rep_data_global_p->stream_files = 0;	// 再生可能ファイル数カウント用

	// PCかどうかをスキン置換情報に追加
	skin_rep_data_global_p->flag_pc = http_recv_info_p->flag_pc;

	// BODYタグ用 onloadset="$focus"
	if (http_recv_info_p->focus[0]) {
		// 安全のため まず uri_encode する。
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
	// ファイル表示用情報 生成開始
	// ---------------------------------
	for ( i=start_file_num, count=0; i<(start_file_num + now_page_line) ; i++, count++ )
	{
		debug_log_output("-----< file info generate, count = %d >-----", count);

		// file_info_p[i].name は EUC (変更 wizd 0.12h)

		// 長さ制限を超えていたらCut
		strncpy(work_data, file_info_p[i].name, sizeof(work_data));
		euc_string_cut_n_length(work_data, global_param.menu_filename_length_max);
		debug_log_output("file_name(cut)='%s'\n", work_data);

		// MediaWiz文字コードに
		convert_language_code(	work_data,
								skin_rep_data_line_p[count].file_name_no_ext,
								sizeof(skin_rep_data_line_p[count].file_name_no_ext),
								CODE_EUC,
								global_param.client_language_code);

		debug_log_output("file_name_no_ext='%s'\n", skin_rep_data_line_p[count].file_name_no_ext);

		// --------------------------------------
		// ファイル名(拡張子無し) 生成終了
		// --------------------------------------

		// --------------------------------------------------------------------------------
		// 拡張子だけ生成
		// --------------------------------------------------------------------------------
		strncpy(skin_rep_data_line_p[count].file_extension, file_info_p[i].ext
			, sizeof(skin_rep_data_line_p[count].file_extension));
		debug_log_output("file_extension='%s'\n", skin_rep_data_line_p[count].file_extension);

		// --------------------------------------------------------------------------------
		// ファイル名 生成 (表示用)  (no_extとextをくっつける)
		// --------------------------------------------------------------------------------
		strncpy(skin_rep_data_line_p[count].file_name, skin_rep_data_line_p[count].file_name_no_ext, sizeof(skin_rep_data_line_p[count].file_name));
		if ( strlen(skin_rep_data_line_p[count].file_extension) > 0 ) {
			strncat(skin_rep_data_line_p[count].file_name, ".", sizeof(skin_rep_data_line_p[count].file_name));
			strncat(skin_rep_data_line_p[count].file_name, skin_rep_data_line_p[count].file_extension, sizeof(skin_rep_data_line_p[count].file_name));
		}

		// --------------------------------------------------------------------------------
		// Link用URI(エンコード済み) を生成
		// --------------------------------------------------------------------------------
		//strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );	// xxx
		//strncat(work_data, file_info_p[i].org_name, sizeof(work_data) - strlen(work_data) );
		strncpy(work_data, file_info_p[i].org_name, sizeof(work_data) );
		uri_encode(skin_rep_data_line_p[count].file_uri_link, sizeof(skin_rep_data_line_p[count].file_uri_link), work_data, strlen(work_data) );
		debug_log_output("file_uri_link='%s'\n", skin_rep_data_line_p[count].file_uri_link);


		// --------------------------------------------------------------------------------
		// ファイルスタンプの日時文字列を生成。
		// --------------------------------------------------------------------------------
		conv_time_to_string(skin_rep_data_line_p[count].file_timestamp, file_info_p[i].time );
		conv_time_to_date_string(skin_rep_data_line_p[count].file_timestamp_date, file_info_p[i].time );
		conv_time_to_time_string(skin_rep_data_line_p[count].file_timestamp_time, file_info_p[i].time );


		// --------------------------------------------------------------------------------
		// ファイルサイズ表示用文字列生成
		// --------------------------------------------------------------------------------
		conv_num_to_unit_string(skin_rep_data_line_p[count].file_size_string, file_info_p[i].size );
		debug_log_output("file_size=%llu", file_info_p[i].size );
		debug_log_output("file_size_string='%s'", skin_rep_data_line_p[count].file_size_string );

		// --------------------------------------------------------------------------------
		// tvid 表示用文字列生成
		// --------------------------------------------------------------------------------
		snprintf(skin_rep_data_line_p[count].tvid_string, sizeof(skin_rep_data_line_p[count].tvid_string), "%d", i+1 );

		// --------------------------------------------------------------------------------
		// vod_string 表示用文字列 とりあえず、""を。
		// --------------------------------------------------------------------------------
		strncpy(skin_rep_data_line_p[count].vod_string, "", sizeof(skin_rep_data_line_p[count].vod_string) );

		// --------------------------------------------------------------------------------
		// 行番号 記憶
		// --------------------------------------------------------------------------------
		skin_rep_data_line_p[count].row_num = count+1;

		// =========================================================
		// ファイルタイプ判定処理
		// =========================================================
		if ( S_ISDIR( file_info_p[i].type ) != 0 )
		{
			// ディレクトリ
			skin_rep_data_line_p[count].stream_type = TYPE_NO_STREAM;
			skin_rep_data_line_p[count].menu_file_type = TYPE_DIRECTORY;
		} else {
			// ディレクトリ以外
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
		// ファイルタイプ毎に必要な文字列を追加で生成
		// =========================================================

		// ----------------------------
		// ディレクトリ 特定処理
		// ----------------------------
		if ( skin_rep_data_line_p[count].menu_file_type == TYPE_DIRECTORY ||
			 skin_rep_data_line_p[count].menu_file_type == TYPE_PSEUDO_DIR )
		{
			// '?'を追加する。
			strncat(skin_rep_data_line_p[count].file_uri_link, "?", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));

			// sort=が指示されていた場合、それを引き継ぐ。
			if ( strlen(http_recv_info_p->sort) > 0 )
			{
				snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
				strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			}

		}

		// -------------------------------------
		// ストリームファイル 特定処理
		// -------------------------------------
		if ( skin_rep_data_line_p[count].stream_type == TYPE_STREAM )
		{
			// vod_string に vod="0" をセット
			strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"0\"", sizeof(skin_rep_data_line_p[count].vod_string) );

			// 拡張子置き換え処理。
			extension_add_rename(skin_rep_data_line_p[count].file_uri_link, sizeof(skin_rep_data_line_p[count].file_uri_link));

			switch (skin_rep_data_line_p[count].menu_file_type) {
			case TYPE_SVI:
				// ------------------------------
				// SVIから情報を引き抜く。
				// ------------------------------

				// SVIファイルのフルパス生成
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
				// sviファイルが見当たらない場合はsv3に変更 0.12f3
				// --------------------------------------------------
				if (access(work_filename, O_RDONLY) != 0) {
					char *p = work_filename;
					while(*p++);	// 行末を探す
					if (*(p-2) == 'i')	// 最後がsviの'i'なら'3'にする
						*(p-2) = '3';
				}

				// --------------------------------------------------
				// SVIファイルから情報をゲットして、文字コード変換。
				// 長さ制限に合わせてCut
				// ※ EUCに変換 → Cut → MediaWiz文字コードへ変換
				// --------------------------------------------------
				if (read_svi_info(work_filename, skin_rep_data_line_p[count].svi_info_data, sizeof(skin_rep_data_line_p[count].svi_info_data),  &rec_time ) == 0) {
					if ( strlen(skin_rep_data_line_p[count].svi_info_data) > 0 )
					{
						convert_language_code(	skin_rep_data_line_p[count].svi_info_data,
												work_data,
												sizeof(work_data),
												CODE_AUTO,
												CODE_EUC );

						// ()[]の削除フラグチェック
						// フラグがTRUEで、ファイルがディレクトリでなければ、括弧を削除する。

						if ( global_param.flag_filename_cut_parenthesis_area == TRUE )
						{
							cut_enclose_words(work_data, sizeof(work_data), "(", ")");
							cut_enclose_words(work_data, sizeof(work_data), "[", "]");
							debug_log_output("svi_info_data(enclose_words)='%s'\n", work_data);
						}

						// CUT実行
						euc_string_cut_n_length(work_data, global_param.menu_svi_info_length_max);

						// MediaWiz文字コードに
						convert_language_code(	work_data,
												skin_rep_data_line_p[count].svi_info_data,
												sizeof(skin_rep_data_line_p[count].svi_info_data),
												CODE_EUC,
												global_param.client_language_code);
					}
					debug_log_output("svi_info_data='%s'\n", skin_rep_data_line_p[count].svi_info_data);

					// SVIから読んだ録画時間を、文字列に。
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
				// 再生可能ファイルカウント
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
					// SinglePlay モードにする。 使いたくない場合は この2行をばっさり削除
					strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
					strncat(skin_rep_data_line_p[count].file_uri_link, "?action=SinglePlay&", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
				}

				break;

			case TYPE_PLAYLIST:
			case TYPE_MUSICLIST:
				// ----------------------------
				// playlistファイル 特定処理
				// ----------------------------

				// vod_string に vod="playlist"をセット
				strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
				break;

			default:
				// vod_string を 削除
				skin_rep_data_line_p[count].vod_string[0] = '\0';
				debug_log_output("unknown type");
				break;
			}
		}


		// ----------------------------
		// IMAGEファイル特定処理
		// ----------------------------
		if ( skin_rep_data_line_p[count].menu_file_type == TYPE_IMAGE
		 ||  skin_rep_data_line_p[count].menu_file_type == TYPE_JPEG  )
		{
			// ----------------------------------
			// イメージファイルのフルパス生成
			// ----------------------------------
			strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
			if ( work_filename[strlen(work_filename)-1] != '/' )
			{
				strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
			}
			strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
			debug_log_output("work_filename(image) = %s", work_filename);



			// ------------------------
			// イメージのサイズをGET
			// ------------------------

			image_width = 0;
			image_height = 0;

			// 拡張子で分岐
			if ( (strcasecmp( skin_rep_data_line_p[count].file_extension, "jpg" ) == 0 ) ||
				 (strcasecmp( skin_rep_data_line_p[count].file_extension, "jpeg" ) == 0 ))
			{
				// JPEGファイルのサイズをGET
				jpeg_size( work_filename, &image_width, &image_height );
			}
			else if (strcasecmp( skin_rep_data_line_p[count].file_extension, "gif" ) == 0 )
			{
				// GIFファイルのサイズをGET
				gif_size( work_filename, &image_width, &image_height );
			}
			else if (strcasecmp( skin_rep_data_line_p[count].file_extension, "png" ) == 0 )
			{
				// PNGファイルのサイズをGET
				png_size( work_filename, &image_width, &image_height );
			}

			// 画像サイズを文字列に、
			snprintf(skin_rep_data_line_p[count].image_width, sizeof(skin_rep_data_line_p[count].image_width), "%d", image_width );
			snprintf(skin_rep_data_line_p[count].image_height, sizeof(skin_rep_data_line_p[count].image_height), "%d", image_height );

			// ----------------------------------
			// リンクの最後に'?'を追加する。
			// ----------------------------------
			strncat(skin_rep_data_line_p[count].file_uri_link, "?", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));

			if (skin_rep_data_line_p[count].menu_file_type == TYPE_JPEG) {
				// JPEGなら SinglePlayにして、さらにAllPlayでも再生可能にする。
				skin_rep_data_global_p->stream_files++;
				strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
				strncat(skin_rep_data_line_p[count].file_uri_link, "action=SinglePlay&", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			} else if ( strlen(http_recv_info_p->sort) > 0 ) {
			// sort=が指示されていた場合、それを引き継ぐ。
				snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
				strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			}
		}


		// -------------------------------------
		// AVIファイル 特定処理
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
				// AVIファイルのフルパス生成
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
				// AVIじゃない
				snprintf(skin_rep_data_line_p[count].avi_vcodec, sizeof(skin_rep_data_line_p->avi_vcodec), "[%s]", skin_rep_data_line_p[count].file_extension);
				snprintf(skin_rep_data_line_p[count].avi_hvcodec, sizeof(skin_rep_data_line_p->avi_hvcodec), "[%s]", skin_rep_data_line_p[count].file_extension);
			}
		}

		// -------------------------------------
		// MP3ファイル 特定処理
		// -------------------------------------
		if ( (skin_rep_data_line_p[count].menu_file_type == TYPE_MUSIC) &&
			 (strcasecmp(skin_rep_data_line_p[count].file_extension, "mp3") == 0) )
		{

			// ----------------------------------
			// MP3ファイルのフルパス生成
			// ----------------------------------
			strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
			if ( work_filename[strlen(work_filename)-1] != '/' )
			{
				strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
			}
			strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
			debug_log_output("work_filename(mp3) = %s", work_filename);


			// ------------------------
			// MP3のID3V1データをGET
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
	// 隠しディレクトリ検索
	// ============================

	memset(skin_rep_data_global_p->secret_dir_link_html, '\0', sizeof(skin_rep_data_global_p->secret_dir_link_html));

	// 隠しディレクトリが存在しているかチェック。
	for ( i=0; i<SECRET_DIRECTORY_MAX; i++)
	{
		if ( strlen(secret_directory_list[i].dir_name) > 0 )	// 隠しディレクトリ指定有り？
		{
			// ----------------------------------
			// 隠しディレクトリのフルパス生成
			// ----------------------------------
			strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
			if ( work_filename[strlen(work_filename)-1] != '/' )
			{
				strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
			}
			strncat(work_filename, secret_directory_list[i].dir_name, sizeof(work_filename) - strlen(work_filename));
			debug_log_output("check: work_filename = %s", work_filename);

			// 存在チェック
			result = stat(work_filename, &dir_stat);
			if ( result == 0 )
			{
				if ( S_ISDIR(dir_stat.st_mode) != 0 ) // ディレクトリ存在！
				{
					// 存在してたら、リンク用URI生成
					strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );
					strncat(work_data, secret_directory_list[i].dir_name, sizeof(work_data) - strlen(work_data) );
					uri_encode(work_data2, sizeof(work_data2), work_data, strlen(work_data) );

					// HTML生成
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
//  スキンを読み込み、置換して送信
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

	// HTTP_OKヘッダ送信
	http_send_ok_header(accept_socket, 0, NULL);

	// ===============================
	// HEAD スキンファイル 読み込み＆置換＆送信
	// ===============================
	if ((header_skin = skin_open(SKIN_MENU_HEAD_HTML)) == NULL) {
		return ;
	}
	// 直接SKIN内のデータを置換
	skin_direct_replace_global(header_skin, skin_rep_data_global_p);
	skin_direct_send(accept_socket, header_skin);
	skin_close(header_skin);

	// ===============================
	// LINE用 スキンファイル 読み込み＆置換＆送信
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

	// LINE 置換処理開始。
	for (count=0; count < lines; count++) {
		int mtype;

		mtype = skin_rep_data_line_p[count].menu_file_type;
		strncpy(menu_work_p, skin_get_string(line_skin[line_skin[mtype] != NULL ? mtype : TYPE_UNKNOWN]), MAX_SKIN_FILESIZE);
		replase_skin_line_data(menu_work_p, MAX_SKIN_FILESIZE, &skin_rep_data_line_p[count] );
		replase_skin_grobal_data(menu_work_p, MAX_SKIN_FILESIZE, skin_rep_data_global_p);

		// 毎回送信
		send(accept_socket, menu_work_p, strlen(menu_work_p), 0);
	}

	// 作業領域解放
	free( menu_work_p );

	// LINE用のスキン解放
	for (i=0; i<MAX_TYPES; i++) {
		if (line_skin[i] != NULL) skin_close(line_skin[i]);
	}



	// ===============================
	// TAIL スキンファイル 読み込み＆置換＆送信
	// ===============================
	if ((tail_skin = skin_open(SKIN_MENU_TAIL_HTML)) == NULL) {
		return ;
	}
	// 直接SKIN内のデータを置換
	skin_direct_replace_global(tail_skin, skin_rep_data_global_p);
	skin_direct_send(accept_socket, tail_skin);
	skin_close(tail_skin);

	return;
}



// **************************************************************************
// *path で指定されたディレクトリに存在するファイル数をカウントする
//
// return: ファイル数
// **************************************************************************
static int count_file_num(unsigned char *path)
{
	int		count;

	DIR	*dir;
	struct dirent	*dent;

	debug_log_output("count_file_num() start. path='%s'", path);

	dir = opendir(path);
	if ( dir == NULL )	// エラーチェック
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

		// 無視ファイルチェック。
		if ( file_ignoral_check(dent->d_name, path) != 0 )
			continue;

		count++;
	}

	closedir(dir);

	debug_log_output("count_file_num() end. counter=%d", count);
	return count;
}

// **************************************************************************
// *path で指定されたTSVファイルに存在するファイル数をカウントする
//
// return: ファイル数
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

		// ファイルから、１行読む
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
// ディレクトリに存在する情報を、ファイルの数だけ読み込む。
//
// return: 読み込んだファイル情報数
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
	if ( dir == NULL )	// エラーチェック
	{
		debug_log_output("opendir() error");
		return ( -1 );
	}

	// あとで削除するときのために、自分のディレクトリ名をEUCで生成
	convert_language_code(	path, dir_name, sizeof(dir_name),
							global_param.server_language_code | CODE_HEX, CODE_EUC );
	// 最後に'/'が付いていたら削除
	cut_character_at_linetail(dir_name, '/');
	// '/'より前を削除
	cut_before_last_character(dir_name, '/');

	count = 0;
	while ( 1 )
	{
		if ( count >= file_num )
			break;

		// ディレクトリから、ファイル名を１個GET
		dent = readdir(dir);
		if ( dent == NULL  )
			break;

		// 無視ファイルチェック。
		if ( file_ignoral_check(dent->d_name, path) != 0 )
			continue;

		//debug_log_output("dent->d_name='%s'", dent->d_name);


		// フルパスファイル名生成
		strncpy(fullpath_filename, path, sizeof(fullpath_filename) );
		strncat(fullpath_filename, dent->d_name, sizeof(fullpath_filename) - strlen(fullpath_filename) );

		//debug_log_output("fullpath_filename='%s'", fullpath_filename );

		// stat() 実行
		memset(&file_stat, 0, sizeof(file_stat));
		result = stat(fullpath_filename, &file_stat);
		if ( result < 0 )
			continue;


		// 対象がディレクトリだった場合、SVIと同一名称ディレクトリチェック。
		if (( S_ISDIR( file_stat.st_mode ) != 0 ) && ( global_param.flag_hide_same_svi_name_directory == TRUE ))
		{
			// チェック実行
			if ( directory_same_check_svi_name(fullpath_filename) != 0 )
				continue;
		}

		// 元のファイル名を保存
		if (S_ISDIR( file_stat.st_mode )) {
			snprintf(file_info_p[count].org_name, sizeof(file_info_p[count].org_name), "%s/", dent->d_name);
		} else {
			strncpy(file_info_p[count].org_name, dent->d_name, sizeof(file_info_p[count].org_name) );
		}

		// EUCに変換！
		convert_language_code(	dent->d_name, file_info_p[count].name,
								sizeof(file_info_p[count].name),
								global_param.server_language_code | CODE_HEX, CODE_EUC );

		// ディレクトリでない場合の、特定処理
		if (S_ISDIR(file_stat.st_mode) == 0) {
			// ()[]の削除フラグチェック
			// フラグがTRUEで、ファイルがディレクトリでなければ、括弧を削除する。
			if (global_param.flag_filename_cut_parenthesis_area == TRUE) {
				cut_enclose_words(file_info_p[count].name, sizeof(file_info_p[count].name), "(", ")");
				cut_enclose_words(file_info_p[count].name, sizeof(file_info_p[count].name), "[", "]");
				debug_log_output("file_name(enclose_words)='%s'\n", file_info_p[count].name);
			}

			// ディレクトリ同名文字列削除フラグチェック
			// フラグがTRUEで、ファイルがディレクトリでなければ、同一文字列を削除。
			if (( global_param.flag_filename_cut_same_directory_name == TRUE )
			 && ( strlen(dir_name) > 0 ))
			{
				// ディレクトリ同名文字列を""で置換
				replase_character_first(file_info_p[count].name, sizeof(file_info_p[count].name), dir_name, "");

				// 頭に' 'が付いているようならば削除。
				cut_first_character(file_info_p[count].name, ' ');

				debug_log_output("file_name(cut_same_directory_name)='%s'\n", file_info_p[count].name);
			}

			// ファイルなら、拡張子の分離
			if ((work_p = strrchr(file_info_p[count].name, '.')) != NULL) {
				// この時点で file_info_p[count].name は 拡張子ナシになる。
				*work_p++ = '\0';
				strncpy(file_info_p[count].ext, work_p, sizeof(file_info_p[count].ext));
				debug_log_output("ext = '%s'", file_info_p[count].ext);
			}
		} else {
			// ディレクトリなら拡張子ナシ
			file_info_p[count].ext[0] = '\0';
		}

		// その他情報を保存
		file_info_p[count].type = file_stat.st_mode;
		file_info_p[count].size = file_stat.st_size;
		file_info_p[count].time = file_stat.st_mtime;


		// SVIファイルだったら、file_info_p[count].size を入れ替える。
		if (( strcasecmp(file_info_p[count].ext, "svi") ==  0 )
		 || ( strcasecmp(file_info_p[count].ext, "sv3") ==  0 )
		) {
			file_info_p[count].size = svi_file_total_size(fullpath_filename);
		}

		// vob先頭ファイルチェック v0.12f3
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
// TSVファイルに存在する情報を、エントリの数だけ読み込む。
//
// return: 読み込んだファイル情報数
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

		// ファイルから、１行読む
		ret = file_read_line( fd, buf, sizeof(buf) );
		if ( ret < 0 )
		{
			debug_log_output("tsv EOF Detect.");
			break;
		}

		// 行末のスペースを削除。
		cut_character_at_linetail(buf, ' ');

		// 空行なら、continue
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

		// フルパスファイル名生成
		if (fname[0] == '/') {
			strncpy(fullpath_filename, global_param.document_root, sizeof(fullpath_filename) );
		} else {
			strncpy(fullpath_filename, path, sizeof(fullpath_filename) );
		}
		strncat(fullpath_filename, fname, sizeof(fullpath_filename) - strlen(fullpath_filename) );

		// ファイル名を保存
		strncpy(file_info_p[count].org_name, fname, sizeof(file_info_p[count].org_name) );
		// to euc
		convert_language_code(	title, file_info_p[count].name, sizeof(file_info_p[count].name),
								CODE_AUTO | CODE_HEX, CODE_EUC );


		debug_log_output("fullpath_filename='%s'", fullpath_filename );

		// 拡張子を生成
		// tsvの場合は、org_name(の元のfname) から生成
		filename_to_extension( fname, file_info_p[count].ext, sizeof(file_info_p[count].ext) );

		// stat() 実行
		memset(&file_stat, 0, sizeof(file_stat));
		result = stat(fullpath_filename, &file_stat);
		if ( result >= 0 ) {
			// 実体発見
			file_info_p[count].type = file_stat.st_mode;
			file_info_p[count].size = file_stat.st_size;
			file_info_p[count].time = file_stat.st_mtime;

			// ディレクトリとわかった時は、拡張子を削除
			if (S_ISDIR(file_stat.st_mode)) {
				file_info_p[count].ext[0] = '\0';
			}
		} else {
			// 実体はない。よって、日付とサイズとファイルの種類不明
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
// ファイル無視チェック
// ディレクトリ内で、ファイルを、無視するかしないかを判断する。
// return: 0:OK  -1 無視
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
	// 上記 ignore_names に該当するものをスキップ
	// ==================================================================
	for (i=0; i<ignore_count; i++) {
		if (!strcmp(name, ignore_names[i])) return -1;
	}

	// ==================================================================
	// MacOSX用 "._" で始まるファイルをスキップ（リソースファイル）
	// ==================================================================
	if ( strncmp(name, "._", 2 ) == 0 )
	{
		return ( -1 );
	}


	// ==================================================================
	// wizdが知らないファイル隠しフラグがたっていたら、拡張子チェック
	// ==================================================================
	if ( global_param.flag_unknown_extention_file_hide == TRUE )
	{
		filename_to_extension( name, file_extension, sizeof(file_extension) );

		flag = 0;
		if ( strlen(file_extension) > 0 ) // 拡張子無しは知らないと同義
		{
			for ( i=0; mime_list[i].file_extension != NULL; i++)
			{
				if ( strcasecmp(mime_list[i].file_extension, file_extension ) == 0 )
				{
					//debug_log_output("%s Known!!!", file_extension );
					flag = 1; // 知ってた
					break;
				}
			}
		}

		if ( flag == 0 ) // 知らなかった。
		{
			// -----------------------------------------------
			// ファイルが、ホントにファイルかチェック。
			// もしもディレクトリなら、returnしない。
			// -----------------------------------------------

			// フルパス生成
			strncpy(work_filename, path, sizeof(work_filename) );
			if ( work_filename[strlen(work_filename)-1] != '/' )
			{
				strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
			}
			strncat(work_filename, name, sizeof(work_filename) - strlen(work_filename) );


			debug_log_output("'%s' Unknown. directory check start.", work_filename );

			// stat() 実行
			result = stat(work_filename, &file_stat);
			if ( result != 0 )
				return ( -1 );

			if ( S_ISDIR(file_stat.st_mode) == 0 ) // ディレクトリじゃない。知らない拡張子ファイルだと確定。
			{
				debug_log_output("'%s' Unknown!!!", name );
				return ( -1 );
			}

			debug_log_output("'%s' is a directory!!!", name );
		}
	}


	// ==================================================================
	// 隠しディレクトリチェック
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
// SVIと同一名称ディレクトリチェック。
// return: 0:OK  -1 無視
// ******************************************************************
static int directory_same_check_svi_name( unsigned char *name )
{
	unsigned char	check_svi_filename[WIZD_FILENAME_MAX];
	struct stat		svi_stat;
	int				result;

	debug_log_output("directory_same_check_svi_name() start.'%s'", name);

	// チェックするSVIファイル名生成
	strncpy( check_svi_filename, name, sizeof(check_svi_filename));
	strncat( check_svi_filename, ".svi", sizeof(check_svi_filename) - strlen(check_svi_filename) );

	// --------------------------------------------------
	// sv3 0.12f3
	// --------------------------------------------------
	if (access(check_svi_filename, O_RDONLY) != 0) {
		char *p = check_svi_filename;
		while(*p++);	// 行末を探す
		if (*(p-2) == 'i')	// 最後がsviの'i'
		*(p-2) = '3';
	}

	result = stat(check_svi_filename, &svi_stat);
	if ( result >= 0 ) // あった？
	{
		debug_log_output("check_svi_filename '%s' found!!", check_svi_filename);
		return ( -1 );
	}

	return ( 0 );	// OK
}


// **************************************************************************
//
// HTTP_OK ヘッダ生成＆送信
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
	SKIN_REPLASE_LINE_DATA_T	mp3_id3tag_data;	// MP3 ID3タグ読み込み用
	MIME_LIST_T *mime;
	int input_code;

	// ---------------------------------------------------------
	// ファイルタイプ判定。対象外ファイルなら作成しない
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
		input_code = CODE_EUC; // file_info_p->name は 常にEUC (wizd 0.12h)
	}
	debug_log_output("file_extension='%s'\n", file_ext);

	mime = lookup_mime_by_ext(file_ext);

	// ------------------------------------------------------
	// 再生対象外なら戻る
	// ------------------------------------------------------
	if ( mime->menu_file_type != TYPE_MOVIE
	 &&	 mime->menu_file_type != TYPE_MUSIC
	 &&	 mime->menu_file_type != TYPE_JPEG
	 &&	 mime->menu_file_type != TYPE_SVI
	) {
		return NULL;
	}

	// -----------------------------------------
	// 拡張子がmp3なら、ID3タグチェック。
	// -----------------------------------------
	mp3_id3tag_data.mp3_id3v1_flag = 0;

	if ( strcasecmp(file_ext, "mp3" ) == 0 ) {
		// MP3ファイルのフルパス生成
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

		// ID3タグチェック
		memset( &mp3_id3tag_data, 0, sizeof(mp3_id3tag_data));
		mp3_id3_tag_read(work_data , &mp3_id3tag_data );
	}


	// MP3 ID3タグが存在したならば、playlist 表示ファイル名をID3タグで置き換える。
	if ( mp3_id3tag_data.mp3_id3v1_flag == 1 )
	{
		strncpy(work_data, mp3_id3tag_data.mp3_id3v1_title_info, sizeof(work_data));
		strncat(work_data, ".mp3", sizeof(work_data) - strlen(work_data));	// ダミーの拡張子。playlist_filename_adjustment()で削除される。

		// =========================================
		// playlist表示用 ID3タグを調整
		// EUCに変換 → 拡張子削除 → (必要なら)半角文字を全角に変換 → MediaWizコードに変換 → SJISなら、不正文字コード0x7C('|')を含む文字を削除。
		// =========================================
		playlist_filename_adjustment( work_data, file_name, sizeof(file_name), CODE_AUTO );
	}
	else
	// MP3 ID3タグが存在しないならば、ファイル名をそのまま使用する。
	{
		// ---------------------------------
		// 表示ファイル名 調整
		// EUCに変換 → 拡張子削除 → (必要なら)半角文字を全角に変換 → MediaWizコードに変換 → SJISなら、不正文字コード0x7C('|')を含む文字を削除。
		// ---------------------------------
		playlist_filename_adjustment(disp_filename, file_name, sizeof(file_name), input_code);
	}

	// ------------------------------------
	// Link用URI(エンコード済み) を生成
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

	// URIの拡張子置き換え処理。
	extension_add_rename(file_uri_link, sizeof(file_uri_link));

	// ------------------------------------
	// プレイリストを生成
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
// * allplay 用のプレイリストを生成
// **************************************************************************
static void create_all_play_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num)
{
	int i;

	debug_log_output("create_all_play_list() start.");

	// HTTP_OKヘッダ送信
	http_send_ok_header(accept_socket, 0, NULL);

	// =================================
	// ファイル表示用情報 生成＆送信
	// =================================
	for ( i=0; i<file_num ; i++ )
	{
		char *ptr;

		// ディレクトリは無視
		if ( S_ISDIR( file_info_p[i].type ) != 0 ) {
			continue;
		}

		ptr = create_1line_playlist(http_recv_info_p, &file_info_p[i]);
		if (ptr != NULL) {
			// 1行づつ、すぐに送信
			send(accept_socket, ptr, strlen(ptr), 0);
		}
	}

	return;
}




// *************************************************************************
//  表示ファイルソート
// *************************************************************************
static void file_info_sort( FILE_INFO_T *p, int num, unsigned long type )
{


	int nDir, nFile, i, row;

	// ディレクトリ数とファイル数の分離
	for ( nDir = 0, i = 0; i < num; ++i )
	{
		if ( S_ISDIR( p[ i ].type ) )
		{
			++nDir;
		}
	}
	nFile = num - nDir;

	// ソート処理 ******************************************************************
	// 準備
	row = 0;

	// ディレクトリソートがあれば行う
	if ( SORT_DIR_FLAG( type ) && nDir > 0 )
	{
		// とりあえず全部ソート
		qsort( p, num, sizeof( FILE_INFO_T ), dir_sort_api[ SORT_DIR_FLAG( type ) ] );

		// ディレクトリ名のソート
		qsort( &p[ ( SORT_DIR_FLAG( type ) == SORT_DIR_DOWN ) ? num - nDir : 0 ], nDir, sizeof( FILE_INFO_T ), file_sort_api[ SORT_NAME_UP ] );

		// ファイル位置確定
		row = ( SORT_DIR_FLAG( type ) == SORT_DIR_DOWN ) ? 0 : nDir;
	}
	else
	{
		// ファイルソート対象を全件にする
		nFile = num;
	}

	// ファイルソートを行う
	// ディレクトリが対象になっていなければ、全件対象にする
	if ( ( type & SORT_FILE_MASK ) && nFile > 0 )
	{
		qsort( &p[ row ], nFile, sizeof( FILE_INFO_T ), file_sort_api[ ( type & SORT_FILE_MASK ) ] );
	}

	return;
}



// *************************************************************************
// ディレクトリのソート
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
// 名前のソート
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
// サイズのソート
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
// 時間のソート
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
// シャッフル
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
// * イメージビューアーを生成して返信
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
	// 置換用データ生成
	// ========================
	// 一つ上のディレクトリパス(親パス)を生成。(URIエンコード)
	strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) - strlen(work_data) );
	cut_after_last_character(work_data, '/');
	strncat(work_data, "/", sizeof(work_data) - strlen(work_data) ); // 最後に'/'を追加。
	debug_log_output("parent_directory='%s'", work_data);


	uri_encode(image_viewer_info.parent_directory_link, sizeof(image_viewer_info.parent_directory_link), work_data, strlen(work_data));
	strncat(image_viewer_info.parent_directory_link, "?", sizeof(image_viewer_info.parent_directory_link) - strlen(image_viewer_info.parent_directory_link));
	// sort=が指示されていた場合、それを引き継ぐ。
	if ( strlen(http_recv_info_p->sort) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
		strncat(image_viewer_info.parent_directory_link, work_data, sizeof(image_viewer_info.parent_directory_link) - strlen(image_viewer_info.parent_directory_link));
	}
	debug_log_output("parent_directory_link='%s'", image_viewer_info.parent_directory_link);


	// 現パス名 表示用 生成(文字コード変換)
	convert_language_code(	http_recv_info_p->recv_uri,
							image_viewer_info.current_uri_name,
							sizeof(image_viewer_info.current_uri_name),
							global_param.server_language_code | CODE_HEX,
							global_param.client_language_code);
	debug_log_output("image_viewer: current_uri = '%s'", image_viewer_info.current_uri_name );

	// 現パス名 Link用生成（URIエンコード）
	uri_encode(image_viewer_info.current_uri_link, sizeof(image_viewer_info.current_uri_link), http_recv_info_p->recv_uri, strlen(http_recv_info_p->recv_uri));
	strncat(image_viewer_info.current_uri_link, "?", sizeof(image_viewer_info.current_uri_link) - strlen(image_viewer_info.current_uri_link)); // 最後に'?'を追加
	// sort=が指示されていた場合、それを引き継ぐ。
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


	// 	現在のページ 表示用
	snprintf(image_viewer_info.now_page_str, sizeof(image_viewer_info.now_page_str), "%d", now_page );


	// ファイルサイズ, タイムスタンプGET
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


	// 画像サイズGET
	image_width = 0;
	image_height = 0;

	// 拡張子取り出し
	filename_to_extension(http_recv_info_p->send_filename, file_extension, sizeof(file_extension) );

	// 拡張子で分岐
	if ( (strcasecmp( file_extension, "jpg" ) == 0 ) ||
		 (strcasecmp( file_extension, "jpeg" ) == 0 ))
	{
		// JPEGファイルのサイズをGET
		jpeg_size( http_recv_info_p->send_filename, &image_width, &image_height );
	}
	else if (strcasecmp( file_extension, "gif" ) == 0 )
	{
		// GIFファイルのサイズをGET
		gif_size( http_recv_info_p->send_filename, &image_width, &image_height );
	}
	else if (strcasecmp( file_extension, "png" ) == 0 )
	{
		// PNGファイルのサイズをGET
		png_size( http_recv_info_p->send_filename, &image_width, &image_height );
	}
	debug_log_output("image_width=%d, image_height=%d", image_width, image_height);

	snprintf(image_viewer_info.image_width, sizeof(image_viewer_info.image_width), "%d", image_width);
	snprintf(image_viewer_info.image_height, sizeof(image_viewer_info.image_height), "%d", image_height);



	// 表示サイズ（仮）
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
		// 縦に合わせてリサイズしてみる
		image_viewer_width = (image_width  * FIT_TERGET_HEIGHT) / image_height;
		image_viewer_height = FIT_TERGET_HEIGHT;

		if ( image_viewer_width > FIT_TERGET_WIDTH ) // 横幅超えていたら
		{
			// 横に合わせてリサイズする。
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
	// ImageViewer スキン読み込み
	// ==============================
	if ((skin = skin_open(SKIN_IMAGE_VIEWER_HTML)) == NULL) {
		return ;
	}

	// ==============================
	// 置換実行
	//   直接SKIN内のデータを置換
	// ==============================
	skin_direct_replace_image_viewer(skin, &image_viewer_info);

    // FITモードチェック
    if ( flag_fit_mode == 0 ) {
        skin_direct_cut_enclosed_words(skin, SKIN_KEYWORD_DEL_IS_NO_FIT_MODE, SKIN_KEYWORD_DEL_IS_NO_FIT_MODE_E); // FITモードではない
    } else {
        skin_direct_cut_enclosed_words(skin, SKIN_KEYWORD_DEL_IS_FIT_MODE, SKIN_KEYWORD_DEL_IS_FIT_MODE_E); // FITモード
    }

	// =================
	// 返信実行
	// =================
	http_send_ok_header(accept_socket, 0, NULL);
	skin_direct_send(accept_socket, skin);
	skin_close(skin);

	return;
}

// **************************************************************************
// * Single Play Listを生成して返信
// **************************************************************************
void http_music_single_play(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	char *ptr;

	// HTTP_OKヘッダ送信
	http_send_ok_header(accept_socket, 0, NULL);

	ptr = create_1line_playlist(http_recv_info_p, NULL);
	if (ptr != NULL) {
		// １行だけ送信
		send(accept_socket, ptr, strlen(ptr), 0);
	}

	return;
}



// **************************************************************************
// * wizd play listファイル(*.plw)より、PlayListを生成して返信
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
	SKIN_REPLASE_LINE_DATA_T	mp3_id3tag_data;	// MP3 ID3タグ読み込み用



	// plwデータ、open
	fd = open(http_recv_info_p->send_filename, O_RDONLY);
	if ( fd < 0 ) {
		debug_log_output("'%s' can't open.", http_recv_info_p->send_filename);
		return;
	}

	// listfileがあるパスを生成
	strncpy( listfile_path, http_recv_info_p->recv_uri, sizeof(listfile_path));
	cut_after_last_character( listfile_path, '/' );
	if ( listfile_path[strlen(listfile_path)-1] != '/' )
	{
		strncat( listfile_path, "/", sizeof(listfile_path) );
	}

	debug_log_output( "listfile_path: '%s'", listfile_path );


	// =============================
	// ヘッダの送信
	// =============================
	http_send_ok_header(accept_socket, 0, NULL);

	//=====================================
	// プレイリスト 生成開始
	//=====================================
	while ( 1 ) {
		// ファイルから、１行読む
		ret = file_read_line( fd, buf, sizeof(buf) );
		if ( ret < 0 ) {
			debug_log_output("listfile EOF Detect.");
			break;
		}

		debug_log_output("-------------");
		debug_log_output("read_buf:'%s'", buf);


		// コメント削除
		if ( buf[0] == '#' ) {
			buf[0] = '\0';
		}

		// 行末のスペースを削除。
		cut_character_at_linetail(buf, ' ');

		debug_log_output("read_buf(comment cut):'%s'", buf);

		// 空行なら、continue
		if ( strlen( buf ) == 0 )
		{
			debug_log_output("continue.");
			continue;
		}

		// Windows用にプレイリスト内のファイル名を調整
		if (global_param.flag_filename_adjustment_for_windows){
			filename_adjustment_for_windows(buf, http_recv_info_p->send_filename);
		}

		// 拡張子 生成
		filename_to_extension(buf, file_extension, sizeof(file_extension) );
		debug_log_output("file_extension:'%s'", file_extension);

		// 表示ファイル名 生成
		strncpy(work_data, buf, sizeof(work_data));
		cut_before_last_character( work_data, '/' );
		strncpy( file_name, work_data, sizeof(file_name));
		debug_log_output("file_name:'%s'", file_name);


		// URI生成
		if ( buf[0] == '/' ) // 絶対パス
		{
			strncpy( file_uri_link, buf, sizeof(file_uri_link) );
		}
		else // 相対パス
		{
			strncpy( file_uri_link, listfile_path, sizeof(file_uri_link) );
			strncat( file_uri_link, buf, sizeof(file_uri_link) - strlen(file_uri_link) );
		}

		debug_log_output("listfile_path:'%s'", listfile_path);
		debug_log_output("file_uri_link:'%s'", file_uri_link);


		// -----------------------------------------
		// 拡張子がmp3なら、ID3タグチェック。
		// -----------------------------------------
		mp3_id3tag_data.mp3_id3v1_flag = 0;
		if ( strcasecmp(file_extension, "mp3" ) == 0 )
		{
			// MP3ファイルのフルパス生成
			strncpy(work_data, global_param.document_root, sizeof(work_data) );
			if ( work_data[strlen(work_data)-1] == '/' )
				work_data[strlen(work_data)-1] = '\0';
			strncat( work_data, file_uri_link, sizeof(work_data) );

			debug_log_output("full_path(mp3):'%s'", work_data); // フルパス

			// ID3タグチェック
			memset( &mp3_id3tag_data, 0, sizeof(mp3_id3tag_data));
			mp3_id3_tag_read(work_data , &mp3_id3tag_data );
		}


		// MP3 ID3タグが存在したならば、playlist 表示ファイル名をID3タグで置き換える。
		if ( mp3_id3tag_data.mp3_id3v1_flag == 1 )
		{
			strncpy(work_data, mp3_id3tag_data.mp3_id3v1_title_info, sizeof(work_data));
			strncat(work_data, ".mp3", sizeof(work_data) - strlen(work_data));	// ダミーの拡張子。playlist_filename_adjustment()で削除される。

			// =========================================
			// playlist表示用 ID3タグを調整
			// EUCに変換 → 拡張子削除 → (必要なら)半角文字を全角に変換 → MediaWizコードに変換 → SJISなら、不正文字コード0x7C('|')を含む文字を削除。
			// =========================================
			playlist_filename_adjustment( work_data, file_name, sizeof(file_name), CODE_AUTO );
		}
		else
		// MP3 ID3タグが存在しないならば、ファイル名をそのまま使用する。
		{
			// ---------------------------------
			// 表示ファイル名 調整
			// EUCに変換 → 拡張子削除 → (必要なら)半角文字を全角に変換 → MediaWizコードに変換 → SJISなら、不正文字コード0x7C('|')を含む文字を削除。
			// ---------------------------------
			strncpy( work_data, file_name, sizeof(work_data) );
			playlist_filename_adjustment(work_data, file_name, sizeof(file_name), global_param.server_language_code);
		}

		debug_log_output("file_name(adjust):'%s'", file_name);

		// ------------------------------------
		// Link用URI(エンコード済み) を生成
		// ------------------------------------
		strncpy(work_data, file_uri_link, sizeof(work_data) );
		uri_encode(file_uri_link, sizeof(file_uri_link), work_data, strlen(work_data) );

		debug_log_output("file_uri_link(encoded):'%s'", file_uri_link);

		// URIの拡張子置き換え処理。
		extension_add_rename(file_uri_link, sizeof(file_uri_link));


		// ------------------------------------
		// プレイリストを生成
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
// fd から１行読み込む
// 読み込んだ文字数がreturnされる。
// 最後まで読んだら、-1が戻る。
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
		// 一文字read.
		read_len  = read(fd, &read_char, 1);
		if ( read_len <= 0 ) // EOF検知
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


#define		SKIN_OPTION_MENU_HTML 	"option_menu.html"	// OptionMenuのスキン
#define		SKIN_KEYWORD_CURRENT_PATH_LINK_NO_SORT	"<!--WIZD_INSERT_CURRENT_PATH_LINK_NO_SORT-->"	// 現PATH(Sort情報引き継ぎ無し)。LINK用。URIエンコード済み

// **************************************************************************
// * オプションメニューを生成して返信
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
	// 置換用データ生成
	// ========================

	// --------------------------------------------------------------
	// 現パス名 Link用(Sort情報引き継ぎ無し)生成（URIエンコード）
	// --------------------------------------------------------------
	uri_encode(current_uri_link_no_sort, sizeof(current_uri_link_no_sort), http_recv_info_p->recv_uri, strlen(http_recv_info_p->recv_uri));
	strncat(current_uri_link_no_sort, "?", sizeof(current_uri_link_no_sort) - strlen(current_uri_link_no_sort)); // '?'を追加

	debug_log_output("OptionMenu: current_uri_link_no_sort='%s'", current_uri_link_no_sort);


	// -----------------------------------------
	// 現パス名 Link用(Sort情報付き) 生成
	// -----------------------------------------
	uri_encode(current_uri_link, sizeof(current_uri_link), http_recv_info_p->recv_uri, strlen(http_recv_info_p->recv_uri));
	strncat(current_uri_link, "?", sizeof(current_uri_link) - strlen(current_uri_link)); // '?'を追加

	// sort=が指示されていた場合、それを引き継ぐ。
	if ( strlen(http_recv_info_p->sort) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
		strncat(current_uri_link, work_data, sizeof(current_uri_link) - strlen(current_uri_link));
	}
	debug_log_output("OptionMenu: current_uri_link='%s'", current_uri_link);


	// ---------------------
	// 現ページ数 生成
	// ---------------------

	if ( http_recv_info_p->page <= 1 )
		now_page = 1;
	else
		now_page = http_recv_info_p->page;

	// 	現在のページ 表示用
	snprintf(now_page_str, sizeof(now_page_str), "%d", now_page );


	// ==============================
	// OptionMenu スキン読み込み
	// ==============================
	if ((skin = skin_open(SKIN_OPTION_MENU_HTML)) == NULL) {
		return ;
	}

	// ==============================
	// 置換実行
	// ==============================
#define REPLACE(a, b) skin_direct_replace_string(skin, SKIN_KEYWORD_##a, (b))
	REPLACE(SERVER_NAME, SERVER_NAME);
	REPLACE(CURRENT_PATH_LINK, current_uri_link);
	REPLACE(CURRENT_PATH_LINK_NO_SORT, current_uri_link_no_sort);
	REPLACE(CURRENT_PAGE, now_page_str);
#undef REPLACE

	// =================
	// 返信実行
	// =================

	http_send_ok_header(accept_socket, 0, NULL);
	skin_direct_send(accept_socket, skin);
	skin_close(skin);

	return;
}



/********************************************************************************/
// 日本語文字コード変換。
// (libnkfのラッパー関数)
//
//	サポートされている形式は以下の通り。
//		in_flag:	CODE_AUTO, CODE_SJIS, CODE_EUC, CODE_UTF8, CODE_UTF16
//		out_flag: 	CODE_SJIS, CODE_EUC
/********************************************************************************/
void convert_language_code(const unsigned char *in, unsigned char *out, size_t len, int in_flag, int out_flag)
{
	unsigned char	nkf_option[128];

	memset(nkf_option, '\0', sizeof(nkf_option));


	//=====================================================================
	// in_flag, out_flagをみて、libnkfへのオプションを組み立てる。
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

	// SAMBAのCAP/HEX文字変換も、nkfに任せる。
	if (global_param.flag_decode_samba_hex_and_cap == TRUE && (in_flag & CODE_HEX)) {
		strncat(nkf_option, " --cap-input --url-input", sizeof(nkf_option) - strlen(nkf_option) );
	}

	//=================================================
	// libnkf 実行
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
// * MP3ファイルから、ID3v1形式のタグデータを得る
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

	// 最後から128byteへSEEK
	length = lseek(fd, -128, SEEK_END);


	// ------------------
	// "TAG"文字列確認
	// ------------------

	// 3byteをread.
	read(fd, buf, 3);
	// debug_log_output("buf='%s'", buf);

	// "TAG" 文字列チェック
	if ( strncmp( buf, "TAG", 3 ) != 0 )
	{
		debug_log_output("NO ID3 Tag.");

		close(fd);
		return;		// MP3 タグ無し。
	}


	// ------------------------------------------------------------
	// Tag情報read
	//
	//	文字列最後に0xFFと' 'が付いていたら削除。
	//  client文字コードに変換。
	// ------------------------------------------------------------


	// 曲名
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 30);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_title,
							sizeof(skin_rep_data_line_p->mp3_id3v1_title),
							CODE_AUTO, global_param.client_language_code);


	// アーティスト
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 30);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_artist,
							sizeof(skin_rep_data_line_p->mp3_id3v1_artist),
							CODE_AUTO, global_param.client_language_code);

	// アルバム名
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 30);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_album,
							sizeof(skin_rep_data_line_p->mp3_id3v1_album),
							CODE_AUTO, global_param.client_language_code);

	// 制作年度
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 4);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_year,
							sizeof(skin_rep_data_line_p->mp3_id3v1_year),
							CODE_AUTO, global_param.client_language_code);

	// コメント
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 28);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_comment,
							sizeof(skin_rep_data_line_p->mp3_id3v1_comment),
							CODE_AUTO, global_param.client_language_code);

	// ---------------------
	// 存在フラグ
	// ---------------------
	skin_rep_data_line_p->mp3_id3v1_flag = 1;

	close(fd);
}

static unsigned long id3v2_len(unsigned char *buf)
{
	return buf[0] * 0x200000 + buf[1] * 0x4000 + buf[2] * 0x80 + buf[3];
}

// **************************************************************************
// * MP3ファイルから、ID3v2形式のタグデータを得る
// * 0: 成功  -1: 失敗(タグなし)
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
	// "ID3"文字列確認
	// ------------------

	// 10byteをread.
	read(fd, buf, 10);
	// debug_log_output("buf='%s'", buf);

	// "ID3" 文字列チェック
	if ( strncmp( buf, "ID3", 3 ) != 0 )
	{
		/*
		 *  ファイルの後ろにくっついてる ID3v2 タグとか
		 *  ファイルの途中にあるのとか 面倒だから 読まないよ。
		 */
		debug_log_output("NO ID3v2 Tag.");

		close(fd);
		return -1;		// v2 タグ無し。
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
	// Tag情報read
	//
	//  client文字コードに変換。
	// ------------------------------------------------------------

	while (len > 0) {
		int frame_len;

		/* フレームヘッダを 読み込む */
		if (read(fd, buf, 10) != 10) {
			close(fd);
			return -1;
		}

		/* フレームの長さを算出 */
		frame_len = id3v2_len(buf + 4);

		/* フレーム最後まで たどりついた */
		if (frame_len == 0 || *(unsigned long*)buf == 0) break;

		for (i=0; i<list_count; i++) {
			if (!strncmp(buf, copy_list[i].id, 4)) break;
		}
		if (i < list_count) {
			// 解釈するタグ 発見

			// 存在フラグ
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
			/* マッチしなかった */
			buf[4] = '\0';
			debug_log_output("ID3v2 Tag[%s] skip", buf);
			lseek(fd, frame_len, SEEK_CUR);
		}
		len -= (frame_len + 10); /* フレーム本体 + フレームヘッダ */
	}

	close(fd);
	return skin_rep_data_line_p->mp3_id3v1_flag ? 0 : -1;
}

static void generate_mp3_title_info( SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p )
{
	unsigned char	work_title_info[128];
	memset(work_title_info, '\0', sizeof(work_title_info));

	// ---------------------
	// mp3_title_info生成
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

	// mp3_title_info (制限無し)を保存。
	strncpy(skin_rep_data_line_p->mp3_id3v1_title_info, work_title_info, sizeof(skin_rep_data_line_p->mp3_id3v1_title_info) );

	// EUCに変換
	convert_language_code( 	work_title_info,
							skin_rep_data_line_p->mp3_id3v1_title_info,
							sizeof(skin_rep_data_line_p->mp3_id3v1_title_info),
							CODE_AUTO, CODE_EUC);
	strncpy(work_title_info, skin_rep_data_line_p->mp3_id3v1_title_info, sizeof(work_title_info));


	// CUT実行
	euc_string_cut_n_length(work_title_info, global_param.menu_filename_length_max);
	debug_log_output("mp3_title_info(cut)='%s'\n", work_title_info);

	// クライアント文字コードに。
	convert_language_code( 	work_title_info,
							skin_rep_data_line_p->mp3_id3v1_title_info_limited,
							sizeof(skin_rep_data_line_p->mp3_id3v1_title_info_limited),
							CODE_EUC, global_param.client_language_code);

	return;
}


// *************************************************************************************
// playlistに渡す表示ファイル名を、問題ない形式に調整する
// *************************************************************************************
static void playlist_filename_adjustment(unsigned char *src_name, unsigned char *dist_name, int dist_size, int input_code )
{
	unsigned char	work_data[WIZD_FILENAME_MAX];


	// ---------------------------------
	// ファイル名 生成 (表示用)
	// EUCに変換 → 拡張子削除 → (必要なら)半角文字を全角に変換 → MediaWizコードに変換 → SJISなら、不正文字コード0x7C('|')を含む文字を伏せ字に。
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
	// HTTP_OKヘッダ送信
	http_send_ok_header(accept_socket, 0, "audio/x-scpls");

	// WinAMP系用プレイリスト送信
	send_printf(accept_socket, "[playlist]\n");
	send_printf(accept_socket, "numberofentries=1\n");
	send_printf(accept_socket, "File1=%s\n", uri_string);
	send_printf(accept_socket, "Length1=-1\n");

	debug_log_output("http_uri_to_scplaylist_create: URI %s\n", uri_string);

	return;
}


// Windows用にプレイリスト内のファイル名を調整
//
// 具体的には、以下を行う
// ・パス区切りの'\'を'/'に変更
// ・Windowsはcase insensitiveなので対応
//
// なお、playlist_filename_adjustmentという表示文字列用関数があるが混同しないこと!!!
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

	// パス区切りの'\'を'/'に変更
	// sjisであることを前提にした処理です
	curp = filename;
	sjis_f = FALSE;
	while (*curp){
		if (!sjis_f){
			if (isleadbyte(*curp)){
				// sjisの1byte目
				sjis_f = TRUE;
			} else if (*curp == '\\'){
				*curp = '/';			// 置き換え
			}
		} else {
			// sjisの2byte目
			sjis_f = FALSE;
		}
		curp++;
	}


	// 先頭から一段目のディレクトリ名を取得
	if (*filename == '/'){
		// 絶対パス
		pathname[0] = '\0';
		curp = filename + 1;
	} else {
		// 相対パス
		strncpy(pathname, pathname_plw, sizeof(pathname));
		cut_after_last_character(pathname, '/');
		curp = filename;
	}
	// 次のベース名を取得
	strncpy(basename, curp, sizeof(basename));
	cut_after_character(basename, '/');


	// ディレクトリ階層毎のループ
	while (1){
		int found = FALSE;
		DIR *dir;
		struct dirent *dent;

		// ディレクトリからcase insensitiveで探す
		debug_log_output("  SEARCH (case-insensitive). pathname = [%s], basename = [%s]\n", pathname, basename );

		dir = opendir(pathname);
		if ( dir == NULL ){
			debug_log_output("Can't Open dir. pathname = [%s]\n", pathname );
			break;
		}

		// ディレクトリ内のループ
		while (1){
			dent = readdir(dir);
			if (dent == NULL){
				// 見つからない(おそらくプレイリストの記述ミス)
				debug_log_output("  NOT FOUND!!! [%s]\n", basename);
				break;
			}
			debug_log_output("    [%s]\n", dent->d_name);
			if (strcasecmp(dent->d_name, basename) == 0){
				// 見つけた
				debug_log_output("  FOUND!!! [%s]->[%s]\n", basename, dent->d_name);
				strncpy(curp, dent->d_name, strlen(dent->d_name));		// 該当部分を本来の名前に置き換え
				strncpy(basename, dent->d_name, sizeof(basename));
				found = TRUE;
				break;
			}
		}

		closedir(dir);

		if (found){
			// 次の階層に進む
			strncat(pathname, "/", sizeof(pathname));
			strncat(pathname, basename, sizeof(pathname));

			curp += strlen(basename);
			if (*curp == '\0'){
				// 終了
				debug_log_output("Loop end.\n");
				break;
			}
			curp++;
			strncpy(basename, curp, sizeof(basename));
			cut_after_character(basename, '/');
			if (*basename == '\0'){
				// 最後が'/'なのは変だけど、終了
				debug_log_output("Loop end ? (/)\n");
				break;
			}
		} else {
			// 結局見つからなかった
			// どうせ再生できないが、とりあえずそのまま...
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
