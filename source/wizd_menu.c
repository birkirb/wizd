//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_menu.c
//											$Revision: 1.53 $
//											$Date: 2004/12/18 15:24:50 $
//
//	The insulator it waits entirely on self responsibility.
//  Please do not inquire to VertexLink concerning this software.
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
#include <netinet/in.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>


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
	unsigned char	chapter_str[WIZD_FILENAME_MAX];
	unsigned char	org_name[WIZD_FILENAME_MAX];	// オリジナルファイル名
	unsigned char	ext[32];						// オリジナルファイル拡張子
	mode_t			type;			// 種類
	off_t			size;			// サイズ
	// use off_t instead of size_t, since it contains st_size of stat.
	time_t			time;			// 日付
	dvd_duration	duration;
	int			    title;
	int				is_longest;
} FILE_INFO_T;





#define		FILEMENU_BUF_SIZE	(1024*16)


static int count_file_num(unsigned char *path);
static int directory_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num);
static int count_file_num_in_tsv(unsigned char *path);
static int tsv_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num);

static int file_ignoral_check(unsigned char *name, unsigned char *path);
static int directory_same_check_svi_name(unsigned char *name);

static void send_skin_filemenu(int accept_socket, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p, int lines, int delete_mode);

//static void create_system_filemenu(HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, unsigned char *send_filemenu_buf, int buf_size);
static void create_skin_filemenu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num);
static void create_all_play_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num);
static char* create_1line_playlist(HTTP_RECV_INFO *http_recv_info_p, char *path, FILE_INFO_T *file_info_p, off_t offset);
static int recurse_all_play_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, char *directory, int count, int depth);

static void create_shuffle_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num);
static int recurse_shuffle_list(HTTP_RECV_INFO *http_recv_info_p, char *directory, char **list, int count, int depth);

#define MAX_PLAY_LIST_DEPTH 10

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
static int		_file_info_duration_sort( const void *in_a, const void *in_b);

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
	_file_info_duration_sort,
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
	int				file_num=0;	// DIR内のファイル数
	int				sort_rule;  // temp置き換え用
	int				pageno;
	unsigned char	*file_info_malloc_p;
	FILE_INFO_T		*file_info_p;
	dvd_reader_t *dvd=NULL;
	ifo_handle_t *ifo_file=NULL;
	ifo_handle_t *ifo_title=NULL;
	int got_title_ifo;
	tt_srpt_t *tt_srpt=NULL;
	int i, j;
	char dvdoptbuf[200];
	char work_data[2048];
	char work_data2[2048];
	struct	stat	dir_stat;
	int print_ifo = 0;
	int longest_duration = -1;
	int longest_title = -1;
	int time_a;
	int first_time;
	off_t	offset=0;

	debug_log_output("http_menu start, recv_uri='%s', send_filename='%s'\n", http_recv_info_p->recv_uri, http_recv_info_p->send_filename);

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

		// Check to see if we can open as a DVD
		snprintf(work_data, sizeof(work_data), "%sVIDEO_TS/VIDEO_TS.IFO", http_recv_info_p->send_filename);
		if(0 == stat(work_data, &dir_stat)) {
			dvd = DVDOpen( http_recv_info_p->send_filename );
			if( dvd ) {
				ifo_file = ifoOpen( dvd, 0 );
				if( ifo_file ) {
					if (print_ifo)
						ifoPrint_VTS_ATRT(ifo_file->vts_atrt);
					tt_srpt = ifo_file->tt_srpt;

					debug_log_output( "Found DVD with %d titles.\n", tt_srpt->nr_of_srpts );

					if (strncmp(http_recv_info_p->action, "showchapters", 12) == 0 ){
						pageno = atoi(&http_recv_info_p->action[12]);
						file_num = tt_srpt->title[ pageno].nr_of_ptts;
					} else
					file_num = tt_srpt->nr_of_srpts;
					if(file_num == 0) {
						// Fall back to normal directory listings
						ifoClose( ifo_file );
						DVDClose( dvd );
						dvd = 0;
					}
				} else {
					debug_log_output("The DVD '%s' does not contain an IFO\n", http_recv_info_p->send_filename );
					DVDClose( dvd );
					dvd = 0;
				}
			}
		}
	}

	// ==================================
	// Directory information GET
	// ==================================

	if (dvd == 0) {
		if (flag_pseudo) {
			// It counts the number of files of the recv_uri directory.			
			file_num = count_file_num_in_tsv( http_recv_info_p->send_filename );
		} else {
			// counts the number of files of the recv_uri directory.				
			file_num = count_file_num( http_recv_info_p->send_filename );
		}
	} else
		/* one more for the chapters entry */
		file_num++;

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


	dvdoptbuf[0] = 0;
	if (strlen(http_recv_info_p->dvdopt) > 0)
	{
		sprintf(dvdoptbuf, "dvdopt=%s&", http_recv_info_p->dvdopt);

		if (strcasecmp(http_recv_info_p->dvdopt ,"showaudio") == 0 ) {
			global_param.flag_show_audio_info = 1;
		} else if (strcasecmp(http_recv_info_p->dvdopt ,"noshowaudio") == 0 ) {
			global_param.flag_show_audio_info = 0;
		} else if (strcasecmp(http_recv_info_p->dvdopt ,"hide_short") == 0 ) {
			global_param.flag_hide_short_titles = 1;
		} else if (strcasecmp(http_recv_info_p->dvdopt ,"nohide_short") == 0 ) {
			global_param.flag_hide_short_titles = 0;
		} else if (strcasecmp(http_recv_info_p->dvdopt ,"splitchapters") == 0 ){
			global_param.flag_split_vob_chapters = 1;
		} else if (strcasecmp(http_recv_info_p->dvdopt ,"nosplitchapters") == 0 ){
			global_param.flag_split_vob_chapters = 0;
		} else if (strcasecmp(http_recv_info_p->dvdopt ,"notitlesplit") == 0 ){
			global_param.flag_split_vob_chapters = 2;
		}
	}

	// -----------------------------------------------------
	// Directory information is read to the file information retention area.
	// -----------------------------------------------------
	if (dvd) {
		if (global_param.sort_rule != SORT_DURATION)
			global_param.sort_rule = SORT_NONE;

		if ( strcasecmp(http_recv_info_p->action, "dvdplay") == 0 )
		{
			int titleid = (http_recv_info_p->page / 1000) - 1;

			if( (titleid < 0) || (titleid >= file_num) )
				titleid = 0;

			offset = 0;
			i = http_recv_info_p->page % 1000;

			// Unless starting at a chapter point, check for any bookmarks
			if(global_param.bookmark_threshold && (i==0)) {
				// Check for a bookmark
				FILE *fp;
				snprintf(work_data2, sizeof(work_data2), "%sVIDEO_TS/VTS_%02d_1.VOB.wizd.bookmark", 
					http_recv_info_p->send_filename, tt_srpt->title[titleid].title_set_nr);
				debug_log_output("Checking for bookmark: '%s'", work_data2);
				fp = fopen(work_data2, "r");
				if(fp != NULL) {
					fgets(work_data2, sizeof(work_data2), fp);
					fclose(fp);
					offset = atoll(work_data2);
					debug_log_output("Bookmark offset: %lld", offset);
				}
			}

			debug_log_output("DVD Title %d Playlist Create Start!!! ", titleid+1);
			http_send_ok_header(accept_socket, 0, NULL);

			// Change any chapter selection to base-0
			if(i>0) i--;

			for(; i<tt_srpt->title[ titleid ].nr_of_ptts; i++) {
				snprintf(work_data, sizeof(work_data), "%sVIDEO_TS/VTS_%02d_1.VOB", http_recv_info_p->recv_uri, tt_srpt->title[titleid].title_set_nr);
				uri_encode(work_data2, sizeof(work_data2), work_data, strlen(work_data) );
				snprintf(work_data, sizeof(work_data), "%s %d|%lld|0|http://%s%s?page=%d&%sfile=dummy.mpg|\n",
					(offset == 0) ? "Chapter" : "Resume>", i+1, offset, http_recv_info_p->recv_host, work_data2, ((titleid+1) * 1000) + i, dvdoptbuf);
				offset = 0;
				debug_log_output("%d: %s", i+1, work_data);
				send(accept_socket, work_data, strlen(work_data), 0);
			}
			debug_log_output("DVD Title Playlist Create End!!! ");
			ifoClose( ifo_file );
			DVDClose( dvd );
			return ;
		} else if ( strncmp(http_recv_info_p->action, "showchapters", 12) == 0) {
			int titleid;
			pgc_t *cur_pgc;
			vts_ptt_srpt_t *vts_ptt_srpt;
			int pgc_id, ttn, pgn, diff;
			int hour, minute, second, tmp;
			int accum_hour, accum_minute, accum_second;
			int cur_cell = 0;

			titleid = atoi(&http_recv_info_p->action[12]);
			ifo_title = ifoOpen(dvd, tt_srpt->title[titleid].title_set_nr);
			vts_ptt_srpt = ifo_title->vts_ptt_srpt;
			ttn = tt_srpt->title[ titleid ].vts_ttn;
			accum_hour = accum_minute = accum_second = 0;
			for(i = 0; i<tt_srpt->title[ titleid ].nr_of_ptts; i++) {
				pgc_id = vts_ptt_srpt->title[ttn - 1].ptt[i].pgcn;
				pgn = vts_ptt_srpt->title[ ttn - 1 ].ptt[ i ].pgn;
				cur_pgc = ifo_title->vts_pgcit->pgci_srp[pgc_id - 1 ].pgc;
				if (cur_pgc->program_map[pgn - 1] == cur_cell + 1)
					diff = 1;
				else
					diff = cur_pgc->program_map[ pgn ] - cur_pgc->program_map[ pgn - 1 ];
				debug_log_output("chapter %d has %d cells\n", i + 1, diff);
				hour = minute = second = 0;
				for (j = 0; j < diff; j++) {
					tmp = cur_pgc->cell_playback[cur_cell].playback_time.second;
					tmp = (tmp & 0xf) + ((tmp & 0xf0) >> 4)*10;
					second += tmp;
					if (second >= 60) {
						minute++;
						second -= 60;
					}
					tmp = cur_pgc->cell_playback[cur_cell].playback_time.minute;
					tmp = (tmp & 0xf) + ((tmp & 0xf0) >> 4)*10;
					minute += tmp;
					if (minute >= 60) {
						hour++;
						minute -= 60;
					}
					tmp = cur_pgc->cell_playback[cur_cell].playback_time.hour;
					tmp = (tmp & 0xf) + ((tmp & 0xf0) >> 4)*10;
					hour += tmp;
					cur_cell++;
				}
				accum_second += second;
				if (accum_second >= 60) {
					accum_minute++;
					accum_second -= 60;
				}
				accum_minute += minute;
				if (accum_minute >= 60) {
					accum_hour++;
					accum_minute -= 60;
				}
				accum_hour += hour;
				file_info_p[i].type = 0;
				file_info_p[i].size = -1;
				file_info_p[i].time = 0;
				file_info_p[i].is_longest = 1;
				file_info_p[i].duration.hour = accum_hour;
				file_info_p[i].duration.minute = accum_minute;
				file_info_p[i].duration.second = accum_second;
				// The 1000's digit is the title id, base-1
				// The 1's digit is chapter number, base-1
				file_info_p[i].title = ((titleid + 1) * 1000) + i + 1;
				strncpy(file_info_p[i].ext, "plw", sizeof(file_info_p[i].ext));
				if(hour < 1) 
					snprintf(file_info_p[i].name, sizeof(file_info_p[i].name), "Chapter %d  ( %d:%02d )", i + 1, minute, second);
				else
					snprintf(file_info_p[i].name, sizeof(file_info_p[i].name), "Chapter %d  ( %d:%02d:%02d )", i + 1, hour, minute, second);
				file_info_p[i].org_name[0] = 0;
			}
			ifoClose(ifo_title);
			file_num--; // get rid of the bump we did above
			http_recv_info_p->sort[0] = '\0';
			global_param.sort_rule = SORT_NONE;
		} else {
# if 0
			file_info_p[0].type = 0;
			file_info_p[0].size = -1;
			file_info_p[0].time = 0;
			file_info_p[0].is_longest = 0;
			file_info_p[0].duration.hour = 99; /* put at top of menu */
			file_info_p[0].title = -1;
			strncpy(file_info_p[0].ext, "chapter", sizeof(file_info_p[0].ext));
			snprintf(file_info_p[0].name, sizeof(file_info_p[0].name), "VIDEO_TS");
			file_info_p[0].org_name[0] = 0;
# else
			file_info_p[0].type = S_IFDIR;
			file_info_p[0].size = 0;
			file_info_p[0].time = 0;
			file_info_p[0].is_longest = 0;
			file_info_p[0].duration.hour = 99; /* put at top of menu */
			file_info_p[0].title = -1;
			file_info_p[0].ext[0] = 0;
			strncpy(file_info_p[0].name, "VIDEO_TS", sizeof(file_info_p[0].name));
			strncpy(file_info_p[0].org_name, "VIDEO_TS/", sizeof(file_info_p[0].org_name));
# endif

			got_title_ifo = -1;
			for( i = 1; i < file_num; ++i ) {
				if (got_title_ifo != tt_srpt->title[i - 1].title_set_nr) {
					if (got_title_ifo != -1) {
							ifoClose(ifo_title);
					}
					ifo_title = ifoOpen(dvd, tt_srpt->title[i - 1].title_set_nr);
					if (ifo_title && print_ifo)
						ifoPrint_VTS_ATRT(ifo_title->vts_atrt);
					got_title_ifo = tt_srpt->title[i - 1].title_set_nr;
				}
				j = ifo_title->vts_pgcit->pgci_srp[tt_srpt->title[i - 1].vts_ttn - 1].pgc->playback_time.hour;
				file_info_p[i].duration.hour = (j&0x0f) + (((j&0xf0)>>4)*10);
				j = ifo_title->vts_pgcit->pgci_srp[tt_srpt->title[i - 1].vts_ttn - 1].pgc->playback_time.minute;
				file_info_p[i].duration.minute = (j&0x0f) + (((j&0xf0)>>4)*10);
				j = ifo_title->vts_pgcit->pgci_srp[tt_srpt->title[i - 1].vts_ttn - 1].pgc->playback_time.second;
				file_info_p[i].duration.second = (j&0x0f) + (((j&0xf0)>>4)*10);
				time_a = (file_info_p[i].duration.hour * 3600) + (file_info_p[i].duration.minute * 60) + file_info_p[i].duration.second;
				if (time_a > longest_duration) {
					longest_title = i;
					longest_duration = time_a;
				}

				file_info_p[i].type = 0;
				file_info_p[i].size = -1;
				file_info_p[i].time = 0;
				file_info_p[i].is_longest = 0;

				strncpy(file_info_p[i].ext, "plw", sizeof(file_info_p[i].ext));

				strcpy(work_data, "Chapters ");
				if (global_param.flag_show_audio_info) {
					if (ifo_title->vtsi_mat->nr_of_vts_audio_streams > 0) {
						first_time = 1;
						for (j = 0; j < ifo_title->vtsi_mat->nr_of_vts_audio_streams; j++) {
							if (ifo_title->vts_pgcit->pgci_srp[0].pgc->audio_control[j] == 0)
								continue;
							if(first_time) {
								strcat(work_data, "(");
								first_time = 0;
							} else {
								strcat(work_data, ", ");
							}
							switch (ifo_title->vtsi_mat->vts_audio_attr[j].audio_format) {
							  case 2:
								strcat(work_data, "mpeg1 ");
								break;
							  case 3:
								strcat(work_data, "mpeg2ext ");
								break;
							  case 4:
								strcat(work_data, "lpcm ");
								break;
							  case 6:
								strcat(work_data, "DTS ");
								break;
							  case 0:
								/* ac3 - no indication - just pass through */
								//strcat(work_data, "AC3 ");
								break;
							  default:
								strcat(work_data, "???");
								break;
							}
							if (ifo_title->vtsi_mat->vts_audio_attr[j].channels == 5)
								strcat(work_data, "5.1");
							else
								strcat(work_data, "2.0");
	
							if (ifo_title->vtsi_mat->vts_audio_attr[j].lang_code != 0)
								sprintf(work_data2, " %c%c",
									ifo_title->vtsi_mat->vts_audio_attr[j].lang_code >> 8,
									ifo_title->vtsi_mat->vts_audio_attr[j].lang_code % 256);
							strcat(work_data, work_data2);
						}
						if(!first_time)
							strcat(work_data, ")");
					}
				}
#if 0
				if (global_param.flag_show_audio_info) {
					if (ifo_title->vtsi_mat->nr_of_vts_subp_streams > 0) {
						first_time = 1;
						for (j = 0; j < ifo_title->vtsi_mat->nr_of_vts_subp_streams; j++) {
							if(first_time) {
								strcat(work_data, "(");
								first_time = 0;
							} else {
								strcat(work_data, ", ");
							}
							if (ifo_title->vtsi_mat->vts_subp_attr[j].lang_code == 0)
								strcpy(work_data2, "??");
							else
								sprintf(work_data2, "%c%c",
									ifo_title->vtsi_mat->vts_subp_attr[j].lang_code >> 8,
									ifo_title->vtsi_mat->vts_subp_attr[j].lang_code % 256);
							strcat(work_data, work_data2);
						}
						if(!first_time)
							strcat(work_data, ")");
					}
				}
#endif
# if 0
				else {
					sprintf(work_data2, "Chapters");
					strcat(work_data, work_data2);
				}
# endif

				file_info_p[i].title = i * 1000;
				snprintf(file_info_p[i].name, sizeof(file_info_p[i].name), "Title %d", i);

				snprintf(file_info_p[i].chapter_str, sizeof(file_info_p[i].chapter_str), "%d %s", tt_srpt->title[i - 1].nr_of_ptts, work_data);
				debug_log_output("chapter str is %s\n", file_info_p[i].chapter_str);

				memset(file_info_p[i].org_name, 0, sizeof(file_info_p[i].org_name));

				debug_log_output( "Title %d\n", i);
				debug_log_output( "\tIn VTS: %d [TTN %d]\n",
					tt_srpt->title[ i - 1 ].title_set_nr,
					tt_srpt->title[ i - 1 ].vts_ttn );
				debug_log_output( "\tTitle has %d chapters and %d angles\n",
					tt_srpt->title[ i - 1 ].nr_of_ptts,
					tt_srpt->title[ i - 1 ].nr_of_angles );
			}
			ifoClose( ifo_title );
			ifoClose( ifo_file );
			DVDClose( dvd );
		}
	} else if (flag_pseudo) {
		file_num = tsv_stat(http_recv_info_p->send_filename, file_info_p, file_num);
	} else {
		file_num = directory_stat(http_recv_info_p->send_filename, file_info_p, file_num);
	}

	debug_log_output("file_num = %d", file_num);

	if (longest_title != -1) {
		file_info_p[longest_title].is_longest = 1;
		file_info_p[longest_title].title = longest_title * 1000;
		debug_log_output("title %d is the longest\n", longest_title);
	}


	// Debugging. File_info_malloc_p indication
	//for ( i=0; i<file_num; i++ )
	//{
	//	debug_log_output("file_info[%d] name='%s'", i, file_info_p[i].name );
	//	debug_log_output("file_info[%d] size='%d'", i, file_info_p[i].size );
	//	debug_log_output("file_info[%d] time='%d'", i, file_info_p[i].time );
	//}


	// ---------------------------------------------------
	// Verification whether sort is indicated with sort=,
	// When it is indicated, superscribing global_param with that
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
		else if (strcasecmp(http_recv_info_p->sort ,"duration") == 0 )
			global_param.sort_rule = SORT_DURATION;
	} else if (flag_pseudo) global_param.sort_rule = SORT_NONE;

	sort_rule = global_param.sort_rule;

	// 0.12f4
	if (global_param.flag_specific_dir_sort_type_fix == TRUE) {
	// Locking the SORT method with the specific directory
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

	// If it is necessary, execution of sort
	if ( sort_rule != SORT_NONE )
	{
		file_info_sort( file_info_p, file_num, sort_rule | SORT_DIR_UP );
	}

	// -------------------------------------------
	// Auto Play Back
	// -------------------------------------------
	if ( strcasecmp(http_recv_info_p->action, "allplay") == 0 )
	{
		if(http_recv_info_p->default_file_type == TYPE_MUSICLIST) {
			// Create the default music playlist		
			int fd;
			snprintf(work_data, sizeof(work_data), "%s/%s/%s",
				global_param.skin_root, global_param.skin_name, DEFAULT_MUSICLIST);
			duplex_character_to_unique(work_data, '/');
			fd=open(work_data,O_CREAT|O_TRUNC|O_WRONLY, S_IREAD | S_IWRITE);
			if(fd >= 0) {
				create_all_play_list(fd, http_recv_info_p, file_info_p, file_num);
				if(lseek(fd,0,SEEK_CUR)==0) {
					close(fd);
					unlink(work_data);
					debug_log_output("Removing empty playlist file '%s'", work_data);
				} else {
					close(fd);
				}
			} else {
				debug_log_output("Failed to create file '%s'", work_data);
			}
			// Redirect back to the original directory
			snprintf(work_data, sizeof(work_data),
				"HTTP/1.1 301 Found\r\n"
				"Location: %s\r\n"
				"\r\n", http_recv_info_p->recv_uri);
			write(accept_socket, work_data, strlen(work_data)+1);
		} else if(http_recv_info_p->default_file_type == TYPE_PLAYLIST) {
			// Create the default photo playlist
			int fd;
			snprintf(work_data, sizeof(work_data), "%s/%s/" DEFAULT_PHOTOLIST,
				global_param.skin_root, global_param.skin_name);
			duplex_character_to_unique(work_data, '/');
			fd=open(work_data,O_CREAT|O_TRUNC|O_WRONLY, S_IREAD | S_IWRITE);
			if(fd >= 0) {
				create_all_play_list(fd, http_recv_info_p, file_info_p, file_num);
				if(lseek(fd,0,SEEK_CUR)==0) {
					close(fd);
					unlink(work_data);
					debug_log_output("Removing empty playlist file '%s'", work_data);
				} else {
					close(fd);
				}
			} else {
				debug_log_output("Failed to create file '%s'", work_data);
			}
			// Redirect back to the original directory
			snprintf(work_data, sizeof(work_data),
				"HTTP/1.1 301 Found\r\n"
				"Location: %s\r\n"
				"\r\n", http_recv_info_p->recv_uri);
			write(accept_socket, work_data, strlen(work_data)+1);
		} else {
			// HTTP_OKヘッダ送信
			http_send_ok_header(accept_socket, 0, NULL);
			create_all_play_list(accept_socket, http_recv_info_p, file_info_p, file_num);
		}
		debug_log_output("AllPlay List Create End!!! ");
		return ;
	}

	// -------------------------------------------
	// Create a random allplay list
	// -------------------------------------------
	if ( strcasecmp(http_recv_info_p->action, "shuffle") == 0 )
	{
		if(http_recv_info_p->default_file_type == TYPE_MUSICLIST) {
			// Create the default music playlist		
			int fd;
			snprintf(work_data, sizeof(work_data), "%s/%s/%s",
				global_param.skin_root, global_param.skin_name, DEFAULT_MUSICLIST);
			duplex_character_to_unique(work_data, '/');
			fd=open(work_data,O_CREAT|O_TRUNC|O_WRONLY, S_IREAD | S_IWRITE);
			if(fd >= 0) {
				create_shuffle_list(fd, http_recv_info_p, file_info_p, file_num);
				if(lseek(fd,0,SEEK_CUR)==0) {
					close(fd);
					unlink(work_data);
					debug_log_output("Removing empty playlist file '%s'", work_data);
				} else {
					close(fd);
				}
			} else {
				debug_log_output("Failed to create file '%s'", work_data);
			}
			// Redirect back to the original directory
			snprintf(work_data, sizeof(work_data),
				"HTTP/1.1 301 Found\r\n"
				"Location: %s\r\n"
				"\r\n", http_recv_info_p->recv_uri);
			write(accept_socket, work_data, strlen(work_data)+1);
		} else if(http_recv_info_p->default_file_type == TYPE_PLAYLIST) {
			// Create the default photo playlist
			int fd;
			snprintf(work_data, sizeof(work_data), "%s/%s/" DEFAULT_PHOTOLIST,
				global_param.skin_root, global_param.skin_name);
			duplex_character_to_unique(work_data, '/');
			fd=open(work_data,O_CREAT|O_TRUNC|O_WRONLY, S_IREAD | S_IWRITE);
			if(fd >= 0) {
				create_shuffle_list(fd, http_recv_info_p, file_info_p, file_num);
				if(lseek(fd,0,SEEK_CUR)==0) {
					close(fd);
					unlink(work_data);
					debug_log_output("Removing empty playlist file '%s'", work_data);
				} else {
					close(fd);
				}
			} else {
				debug_log_output("Failed to create file '%s'", work_data);
			}
			// Redirect back to the original directory
			snprintf(work_data, sizeof(work_data),
				"HTTP/1.1 301 Found\r\n"
				"Location: %s\r\n"
				"\r\n", http_recv_info_p->recv_uri);
			write(accept_socket, work_data, strlen(work_data)+1);
		} else {
			// HTTP_OKヘッダ送信
			http_send_ok_header(accept_socket, 0, NULL);
			create_shuffle_list(accept_socket, http_recv_info_p, file_info_p, file_num);
		}
		debug_log_output("Shuffle List Create End!!! ");
		return ;
	}


	// -------------------------------------------
	// File menu creation and tranmission
	// -------------------------------------------
	create_skin_filemenu(accept_socket, http_recv_info_p, file_info_p, file_num);

	free(file_info_malloc_p);

	return;
}





// ****************************************************************************************
// Define variety for menu formation.
// ****************************************************************************************



// **************************************************************************
// Forming the file menu which uses the skin
// **************************************************************************

static void create_skin_filemenu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num)
{
	int		i,j;

	unsigned char	work_filename[WIZD_FILENAME_MAX];
	char 			dvdoptbuf[200];

	unsigned char	work_data[WIZD_FILENAME_MAX];
	unsigned char	work_data2[WIZD_FILENAME_MAX];

	int		now_page;		// Present page
	int		max_page;		// 最大ページ番号
	int		now_page_line;	// number of lines
	int		start_file_num;	// start file number of the page
	int		end_file_num;	// 現在ページの表示終了ファイル番号
	int		stream_type;
	int		menu_file_type;

	int		next_page;		// next page if not max page
	int		prev_page;		// 一つ前のページ（無ければ 1)


	SKIN_REPLASE_GLOBAL_DATA_T	*skin_rep_data_global_p;
	SKIN_REPLASE_LINE_DATA_T	*skin_rep_data_line_p;

	int skin_rep_line_malloc_size;

	unsigned int	rec_time;
	int	count;

	unsigned int	image_width, image_height;

	struct	stat	dir_stat;
	int				result;

	int delete_mode=0;
	if ( global_param.flag_allow_delete && (strcasecmp(http_recv_info_p->action, "delete") == 0) ) {
		debug_log_output("action=delete: Enabling delete mode output");
		delete_mode = 1;
	}

	// ==========================================
	// Read Skin Config
	// ==========================================

	skin_read_config(SKIN_MENU_CONF);

	// ==========================================
	// HTML Formation
	// ==========================================

	// Number of files
	debug_log_output("file_num = %d", file_num);
	debug_log_output("page_line_max = %d", global_param.page_line_max);

	// 最大ページ数計算
	if ( file_num == 0 )
	{
		max_page = 1;
	}
	else
	{
		max_page = ((file_num-1) / global_param.page_line_max) + 1;
	}
	debug_log_output("max_page = %d", max_page);

	// Current page calculation
	if ( (http_recv_info_p->page <= 1 ) || (max_page < http_recv_info_p->page ) )
		now_page = 1;
	else
		now_page = http_recv_info_p->page;

	debug_log_output("now_page = %d", now_page);

	// calculation of line to be shown on current page
	if ( max_page == now_page ) // 最後のページ
		now_page_line = file_num - (global_param.page_line_max * (max_page-1));
	else	// 最後以外なら、表示最大数。
		now_page_line = global_param.page_line_max;
	debug_log_output("now_page_line = %d", now_page_line);


	// Start file number calculation
	start_file_num = ((now_page - 1) * global_param.page_line_max);
	debug_log_output("start_file_num = %d", start_file_num);

	if ( max_page == now_page ) // 最後のページ
		end_file_num = file_num;
	else // not last page
		end_file_num = (start_file_num + global_param.page_line_max);
	debug_log_output("start_file_num = %d", start_file_num);




	// Prev folder calculation
	prev_page =  1 ;
	if ( now_page > 1 )
		prev_page = now_page - 1;

	// Next page/folder calculation
	next_page = max_page ;
	if ( max_page > now_page )
		next_page = now_page + 1;

	debug_log_output("prev_page=%d  next_page=%d", prev_page ,next_page);



	// ===============================
	// Prepare data for display in skin
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
	// Start of information formation for global indication
	// ---------------------------------

	// Formation for present path name indication (recv_uri - current_path_name [ client code ])
	convert_language_code(	http_recv_info_p->recv_uri,
							skin_rep_data_global_p->current_path_name,
							sizeof(skin_rep_data_global_p->current_path_name),
							global_param.server_language_code | CODE_HEX,
							global_param.client_language_code );
	debug_log_output("current_path = '%s'", skin_rep_data_global_p->current_path_name );

	// -----------------------------------------------
	// Forming the one for upper directory
	// -----------------------------------------------
	work_data[0] = 0;
	work_data2[0] = 0;
	if (get_parent_path(work_data, http_recv_info_p->recv_uri, sizeof(work_data)) == NULL) {
		debug_log_output("FATAL ERROR! too long recv_uri.");
		return ;
	}
	debug_log_output("parent_directory='%s'", work_data);

	// Directory path with respect to one URI encoding.
	uri_encode(skin_rep_data_global_p->parent_directory_link, sizeof(skin_rep_data_global_p->parent_directory_link), work_data, strlen(work_data));
	// '?'add
	strncat(skin_rep_data_global_p->parent_directory_link, "?", sizeof(skin_rep_data_global_p->parent_directory_link) - strlen(skin_rep_data_global_p->parent_directory_link));
	// if sort
	if ( strlen(http_recv_info_p->sort) > 0 ) {
		snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
		strncat(skin_rep_data_global_p->parent_directory_link, work_data, sizeof(skin_rep_data_global_p->parent_directory_link) - strlen(skin_rep_data_global_p->parent_directory_link));
	}
	debug_log_output("parent_directory_link='%s'", skin_rep_data_global_p->parent_directory_link);

	// For upper directory name indication
	convert_language_code(	work_data,
							skin_rep_data_global_p->parent_directory_name,
							sizeof(skin_rep_data_global_p->parent_directory_name),
							global_param.server_language_code | CODE_HEX,
							global_param.client_language_code );
	debug_log_output("parent_directory_name='%s'", skin_rep_data_global_p->parent_directory_link);


	// -----------------------------------------------
	// Forming for present directory next
	// -----------------------------------------------

	// Forming present directory name. First it converts to EUC.
	convert_language_code(	http_recv_info_p->recv_uri,
							work_data,
							sizeof(work_data),
							global_param.server_language_code | CODE_HEX,
							CODE_EUC );

	// delete last dir char end with “/”
	cut_character_at_linetail(work_data, '/');
	// delete before  start with /
	cut_before_last_character(work_data, '/');
	// Adding ‘/’
	strncat(work_data, "/", sizeof(work_data) - strlen(work_data) );

	// CUT Exec
	euc_string_cut_n_length(work_data, global_param.menu_filename_length_max);

	// Formation for present directory name indication (character code conversion)
	convert_language_code(	work_data,
							skin_rep_data_global_p->current_directory_name,
							sizeof(skin_rep_data_global_p->current_directory_name),
							CODE_EUC,
							global_param.client_language_code );

	debug_log_output("current_dir = '%s'", skin_rep_data_global_p->current_directory_name );


	// The formation for present path name Link (URI encoding from recv_uri)
	uri_encode(skin_rep_data_global_p->current_directory_link_no_param
		, sizeof(skin_rep_data_global_p->current_directory_link_no_param)
		, http_recv_info_p->recv_uri
		, strlen(http_recv_info_p->recv_uri)
	);
	strncpy(skin_rep_data_global_p->current_directory_link
		, skin_rep_data_global_p->current_directory_link_no_param
		, sizeof(skin_rep_data_global_p->current_directory_link)
	);

	strncpy(skin_rep_data_global_p->current_directory_absolute, "http://", sizeof(skin_rep_data_global_p->current_directory_absolute));
	strncat(skin_rep_data_global_p->current_directory_absolute, http_recv_info_p->recv_host, sizeof(skin_rep_data_global_p->current_directory_absolute) - strlen(skin_rep_data_global_p->current_directory_absolute));
	strncpy(skin_rep_data_global_p->recv_host, http_recv_info_p->recv_host, sizeof(skin_rep_data_global_p->recv_host));


	// ?を追加
	strncat(skin_rep_data_global_p->current_directory_link, "?", sizeof(skin_rep_data_global_p->current_directory_link) - strlen(skin_rep_data_global_p->current_directory_link)); // '?'を追加
	// sort=が指示されていた場合、それを引き継ぐ。
	if ( strlen(http_recv_info_p->sort) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
		strncat(skin_rep_data_global_p->current_directory_link, work_data, sizeof(skin_rep_data_global_p->current_directory_link) - strlen(skin_rep_data_global_p->current_directory_link));
	}
	if (strncmp(http_recv_info_p->action, "showchapters", 12) == 0 ){
		snprintf(work_data, sizeof(work_data), "action=%s&", http_recv_info_p->action);
		strncat(skin_rep_data_global_p->current_directory_link, work_data, sizeof(skin_rep_data_global_p->current_directory_link) - strlen(skin_rep_data_global_p->current_directory_link));
	}
	debug_log_output("current_directory_link='%s'", skin_rep_data_global_p->current_directory_link);
	strncat(skin_rep_data_global_p->current_directory_absolute, skin_rep_data_global_p->current_directory_link, sizeof(skin_rep_data_global_p->current_directory_absolute) - strlen(skin_rep_data_global_p->current_directory_absolute));

	debug_log_output("current_directory_absolute='%s'", skin_rep_data_global_p->current_directory_absolute);

	// ------------------------------------ processing here for directory indication

	// For directory existence file several indications
	snprintf(skin_rep_data_global_p->file_num_str, sizeof(skin_rep_data_global_p->file_num_str), "%d", file_num );

	// 	For present page indication
	snprintf(skin_rep_data_global_p->now_page_str, sizeof(skin_rep_data_global_p->now_page_str), "%d", now_page );

	// For full page several indications
	snprintf(skin_rep_data_global_p->max_page_str, sizeof(skin_rep_data_global_p->max_page_str), "%d", max_page );

	//for next page indication
	snprintf(skin_rep_data_global_p->next_page_str, sizeof(skin_rep_data_global_p->next_page_str), "%d", next_page );

	// 前のページ 表示用
	snprintf(skin_rep_data_global_p->prev_page_str, sizeof(skin_rep_data_global_p->prev_page_str), "%d", prev_page );

	// 開始ファイル番号表示用
	if ( file_num == 0 )
		snprintf(skin_rep_data_global_p->start_file_num_str, sizeof(skin_rep_data_global_p->start_file_num_str), "%d", start_file_num );
	else
		snprintf(skin_rep_data_global_p->start_file_num_str, sizeof(skin_rep_data_global_p->start_file_num_str), "%d", start_file_num +1 );

	// For end file number indication
	snprintf(skin_rep_data_global_p->end_file_num_str, sizeof(skin_rep_data_global_p->end_file_num_str), "%d", end_file_num  );

	skin_rep_data_global_p->stream_dirs = 0;	// Subdirectories
	skin_rep_data_global_p->stream_files = 0;	// Playable video files
	skin_rep_data_global_p->music_files = 0;	// Playable music files
	skin_rep_data_global_p->photo_files = 0;	// Playable photo files

	// Whether or not PC it adds to the information of skin substitution
	skin_rep_data_global_p->flag_pc = http_recv_info_p->flag_pc;

	// Set up for the default playlists
	strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename));
	strncat(work_filename, DEFAULT_PHOTOLIST, sizeof(work_filename)-strlen(work_filename));
	if( (0 == stat(work_filename, &dir_stat)) && S_ISREG(dir_stat.st_mode) ) {
		snprintf(skin_rep_data_global_p->default_photolist, sizeof(skin_rep_data_global_p->default_photolist), "pod=\"2,1,%s%s?type=slideshow\"",
			skin_rep_data_global_p->current_directory_absolute, DEFAULT_PHOTOLIST);
	} else {
		snprintf(work_filename, sizeof(work_filename), "%s/%s/%s", global_param.skin_root, global_param.skin_name, DEFAULT_PHOTOLIST);
		duplex_character_to_unique(work_filename, '/');
		if( (0 == stat(work_filename, &dir_stat)) && S_ISREG(dir_stat.st_mode) ) {
			snprintf(skin_rep_data_global_p->default_photolist, sizeof(skin_rep_data_global_p->default_photolist), 
				"pod=\"2,1,http://%s/%s?type=slideshow\"", http_recv_info_p->recv_host, DEFAULT_PHOTOLIST);
		} else {
			// No default photo list
			strncpy(skin_rep_data_global_p->default_photolist, "vod=\"playlist\"", sizeof(skin_rep_data_global_p->default_photolist));
		}
	}

	debug_log_output("Default photolist = '%s'", skin_rep_data_global_p->default_photolist);

	strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename));
	strncat(work_filename, DEFAULT_MUSICLIST, sizeof(work_filename)-strlen(work_filename));
	if( (0 == stat(work_filename, &dir_stat)) && S_ISREG(dir_stat.st_mode) ) {
		strncpy(skin_rep_data_global_p->default_musiclist, DEFAULT_MUSICLIST, sizeof(skin_rep_data_global_p->default_musiclist));
	} else {
		snprintf(work_filename, sizeof(work_filename), "%s/%s/%s", global_param.skin_root, global_param.skin_name, DEFAULT_MUSICLIST);
		duplex_character_to_unique(work_filename, '/');
		if( (0 == stat(work_filename, &dir_stat)) && S_ISREG(dir_stat.st_mode) ) {
			strncpy(skin_rep_data_global_p->default_musiclist, DEFAULT_MUSICLIST, sizeof(skin_rep_data_global_p->default_musiclist));
		} else {
			// No default musiclist
			strncpy(skin_rep_data_global_p->default_musiclist, "mute", sizeof(skin_rep_data_global_p->default_musiclist));
		}
	}

	debug_log_output("Default musiclist = '%s'", skin_rep_data_global_p->default_musiclist);

	// BODY tag onloadset= "$focus"
	if (http_recv_info_p->focus[0]) {
		// For safety uri_encode it does first.
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
	// Start of information formation for file indication
	// ---------------------------------
	for ( i=0, count=0 ; i < file_num ; i++) {
		// =========================================================
		// File type decision processing
		// =========================================================
		if ( S_ISDIR( file_info_p[i].type ) != 0 )
		{
			// ディレクトリ
			stream_type = TYPE_NO_STREAM;
			menu_file_type = TYPE_DIRECTORY;
		} else if( delete_mode ) {
			// In delete mode, display everything else as a deletable file
			stream_type = TYPE_NO_STREAM;
			menu_file_type = TYPE_DELETE;
		} else {
			// Other than directory
			MIME_LIST_T *mime;

			if ((mime = lookup_mime_by_ext(file_info_p[i].ext)) == NULL) {
				stream_type = TYPE_NO_STREAM;
				menu_file_type = TYPE_UNKNOWN;
			} else {
				stream_type = mime->stream_type;
				menu_file_type = mime->menu_file_type;
			}
		}
		debug_log_output("menu_file_type=%d\n", menu_file_type);
		// Count the total number of files of each type in the directory
		switch(menu_file_type) {
		case TYPE_MOVIE:
		case TYPE_SVI:
			skin_rep_data_global_p->stream_files++;
			debug_log_output("Is video file %d", skin_rep_data_global_p->stream_files);
			break;
		case TYPE_MUSIC:
			skin_rep_data_global_p->music_files++;
			debug_log_output("Is music file %d", skin_rep_data_global_p->music_files);
			break;
		case TYPE_IMAGE:
		case TYPE_JPEG:
			skin_rep_data_global_p->photo_files++;
			debug_log_output("Is photo file %d", skin_rep_data_global_p->photo_files);
			break;
		case TYPE_DIRECTORY:
			skin_rep_data_global_p->stream_dirs++;
			break;
		}
	if ( (i>=start_file_num) && (i<(start_file_num + now_page_line)) )
	{
		debug_log_output("-----< file info generate, count = %d >-----", count);

		// File_info_p [ i ] name EUC (modification wizd 0.12h)
		// When it exceeds length restriction, Cut
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
		// File name (the extension it is not) formation end
		// --------------------------------------

		// --------------------------------------------------------------------------------
		// Just extension formation
		// --------------------------------------------------------------------------------
		strncpy(skin_rep_data_line_p[count].file_extension, file_info_p[i].ext
			, sizeof(skin_rep_data_line_p[count].file_extension));
		debug_log_output("file_extension='%s'\n", skin_rep_data_line_p[count].file_extension);

		// --------------------------------------------------------------------------------
		// File name formation (for indication) (no_ext and ext are attached)
		// --------------------------------------------------------------------------------
		strncpy(skin_rep_data_line_p[count].file_name, skin_rep_data_line_p[count].file_name_no_ext, sizeof(skin_rep_data_line_p[count].file_name));
		if ( strlen(skin_rep_data_line_p[count].file_extension) > 0 ) {
			strncat(skin_rep_data_line_p[count].file_name, ".", sizeof(skin_rep_data_line_p[count].file_name));
			strncat(skin_rep_data_line_p[count].file_name, skin_rep_data_line_p[count].file_extension, sizeof(skin_rep_data_line_p[count].file_name));
		}

		// --------------------------------------------------------------------------------
		// For Link URI (encoding to be completed)
		// --------------------------------------------------------------------------------
		//strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );	// xxx
		//strncat(work_data, file_info_p[i].org_name, sizeof(work_data) - strlen(work_data) );
		strncpy(work_data, file_info_p[i].org_name, sizeof(work_data) );
		uri_encode(skin_rep_data_line_p[count].file_uri_link, sizeof(skin_rep_data_line_p[count].file_uri_link), work_data, strlen(work_data) );
		debug_log_output("file_uri_link='%s'\n", skin_rep_data_line_p[count].file_uri_link);


		// --------------------------------------------------------------------------------
		// Forming the day and time character string of the file stamp.
		// --------------------------------------------------------------------------------
		conv_time_to_string(skin_rep_data_line_p[count].file_timestamp, file_info_p[i].time );
		conv_time_to_date_string(skin_rep_data_line_p[count].file_timestamp_date, file_info_p[i].time );
		conv_time_to_time_string(skin_rep_data_line_p[count].file_timestamp_time, file_info_p[i].time );
		conv_duration_to_string(skin_rep_data_line_p[count].file_duration, &file_info_p[i].duration );



		// --------------------------------------------------------------------------------
		// Character string formation for file size indication
		// --------------------------------------------------------------------------------
		conv_num_to_unit_string(skin_rep_data_line_p[count].file_size_string, file_info_p[i].size );
		debug_log_output("file_size=%llu", file_info_p[i].size );
		debug_log_output("file_size_string='%s'", skin_rep_data_line_p[count].file_size_string );

		// --------------------------------------------------------------------------------
		// Character string formation for tvid indication
		// --------------------------------------------------------------------------------
		snprintf(skin_rep_data_line_p[count].tvid_string, sizeof(skin_rep_data_line_p[count].tvid_string), "%d", i+1 );

		// --------------------------------------------------------------------------------
		// The character string for vod_string indication temporarily, "".
		// --------------------------------------------------------------------------------
		strncpy(skin_rep_data_line_p[count].vod_string, "", sizeof(skin_rep_data_line_p[count].vod_string) );

		// --------------------------------------------------------------------------------
		// Line number memory
		// --------------------------------------------------------------------------------
		skin_rep_data_line_p[count].row_num = count+1;

		skin_rep_data_line_p[count].stream_type = stream_type;
		skin_rep_data_line_p[count].menu_file_type = menu_file_type;

		// =========================================================
		// Forming the character string which is necessary for every file type with addition
		// =========================================================

		// ----------------------------
		// Directory specification processing
		// ----------------------------
		if ( skin_rep_data_line_p[count].menu_file_type == TYPE_DIRECTORY ||
			 skin_rep_data_line_p[count].menu_file_type == TYPE_PSEUDO_DIR )
		{
			// it adds “?”
			strncat(skin_rep_data_line_p[count].file_uri_link, "?", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));

			// When sort= is indicated, it takes over that.
			if ( strlen(http_recv_info_p->sort) > 0 )
			{
				snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
				strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			}
		}

		// -------------------------------------
		// Stream file specification processing
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
				// It pulls information from SVI out.
				// ------------------------------

				// Full pass formation of SVI file
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
				// When the svi file is not found, in sv3 modification 0.12f3
				// --------------------------------------------------
				if (access(work_filename, O_RDONLY) != 0) {
					char *p = work_filename;
					while(*p++);	// 行末を探す
					if (*(p-2) == 'i')	// 最後がsviの'i'なら'3'にする
						*(p-2) = '3';
				}

				// --------------------------------------------------
				// Getting information from the SVI file, character code conversion
				// Adjusting to length restriction, Cut
				// * Conversion -> it converts to the Cut -> MediaWiz character code to EUC						// --------------------------------------------------
				if (read_svi_info(work_filename, skin_rep_data_line_p[count].svi_info_data, sizeof(skin_rep_data_line_p[count].svi_info_data),  &rec_time ) == 0) {
					if ( strlen(skin_rep_data_line_p[count].svi_info_data) > 0 )
					{
						convert_language_code(	skin_rep_data_line_p[count].svi_info_data,
												work_data,
												sizeof(work_data),
												CODE_AUTO,
												CODE_EUC );

						// () [ ] Deletion flag check
						// If the flag with TRUE, the file is not the directory, the parenthesis is deleted.
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

					// The video recording time which was read from SVI, in the character string.					
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
				// playlist file processing
				// ----------------------------

				if(file_info_p[i].org_name[0] == 0) {
					// Special case for DVD directories

					if (global_param.flag_hide_short_titles && file_info_p[i].duration.hour == 0 && file_info_p[i].duration.minute == 0 && file_info_p[i].duration.second <= 1) {
						count--;
						continue;
					}

					dvdoptbuf[0] = 0;
					if (global_param.flag_split_vob_chapters == 2) {
						if (file_info_p[i].is_longest)
							sprintf(dvdoptbuf, "dvdopt=notitlesplit&");
						else
							sprintf(dvdoptbuf, "dvdopt=splitchapters&");
						debug_log_output("PLAYLIST: dvdoptbuf %s, longest %d\n", dvdoptbuf, i);
					} else if (strlen(http_recv_info_p->dvdopt) > 0)
						sprintf(dvdoptbuf, "dvdopt=%s&", http_recv_info_p->dvdopt);

					snprintf(skin_rep_data_line_p[count].chapter_link, sizeof(skin_rep_data_line_p[count].chapter_link), "%s?action=showchapters%d", skin_rep_data_line_p[count].file_uri_link, (file_info_p[i].title / 1000) - 1);
					snprintf(work_data, sizeof(work_data), "?action=dvdplay&page=%d&%s", file_info_p[i].title, dvdoptbuf);
					strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
					strcpy(skin_rep_data_line_p[count].chapter_str, file_info_p[i].chapter_str);
					debug_log_output("copied %s to chapter link\n", skin_rep_data_line_p[count].chapter_link);
					debug_log_output("copied %s to chapter str\n", skin_rep_data_line_p[count].chapter_str);
				}
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
		// IMAGE file specification processing
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
			// Size of image GET
			// ------------------------

			image_width = 0;
			image_height = 0;

			// It diverges with the extension
			if ( (strcasecmp( skin_rep_data_line_p[count].file_extension, "jpg" ) == 0 ) ||
				 (strcasecmp( skin_rep_data_line_p[count].file_extension, "jpeg" ) == 0 ))
			{
				// Size of JPEG file GET
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

			// Picture size in character string,			
			snprintf(skin_rep_data_line_p[count].image_width, sizeof(skin_rep_data_line_p[count].image_width), "%d", image_width );
			snprintf(skin_rep_data_line_p[count].image_height, sizeof(skin_rep_data_line_p[count].image_height), "%d", image_height );

			// ----------------------------------
			// Link lastly '? ' It adds.
			// ----------------------------------
			strncat(skin_rep_data_line_p[count].file_uri_link, "?", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));

			if (skin_rep_data_line_p[count].menu_file_type == TYPE_JPEG) {
				// JPEGなら SinglePlayにして、さらにAllPlayでも再生可能にする。
				strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
				strncat(skin_rep_data_line_p[count].file_uri_link, "action=SinglePlay&", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			} else if ( strlen(http_recv_info_p->sort) > 0 ) {
			// sort=が指示されていた場合、それを引き継ぐ。
				snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
				strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			}
		}


		// -------------------------------------
		// AVI file specification processing
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
		// MP3 file specification processing
		// -------------------------------------
		if ( (skin_rep_data_line_p[count].menu_file_type == TYPE_MUSIC) &&
			 (strcasecmp(skin_rep_data_line_p[count].file_extension, "mp3") == 0) )
		{

			// ----------------------------------
			// Full pass formation of MP3 file
			// ----------------------------------
			strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
			if ( work_filename[strlen(work_filename)-1] != '/' )
			{
				strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
			}
			strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
			debug_log_output("work_filename(mp3) = %s", work_filename);


			// ------------------------
			// The ID3V1 data of MP3 GET
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

		// ---------------------------------
		// Internet explorer URL shortcut
		// ---------------------------------
		if ( skin_rep_data_line_p[count].menu_file_type == TYPE_URL )
		{
			// Open the file and extract the URL
			FILE *fp;
					
			strncpy(work_data, http_recv_info_p->send_filename, sizeof(work_data) );
			strncat(work_data, file_info_p[i].name, sizeof(work_data) - strlen(work_data) );
			strncat(work_data, ".url", sizeof(work_data) - strlen(work_data) );

			fp = fopen(work_data, "r");
			if(fp != NULL) {
				debug_log_output("Scanning file for URL '%s'\n", work_data);

				while(!feof(fp) && (NULL != fgets(work_data, sizeof(work_data), fp)) ) {
					if(strncasecmp(work_data, "URL=", 4) == 0) {
						// Strip trailing whitespace
						for(j=strlen(work_data)-1; (j>4) && isspace(work_data[j]); j--)
							work_data[j]=0;
						strncpy(skin_rep_data_line_p[count].file_uri_link, work_data+4, sizeof(skin_rep_data_line_p[count].file_uri_link));
						debug_log_output("Found URL '%s'\n", skin_rep_data_line_p[count].file_uri_link);
						break;
					}
				}
				fclose(fp);
			} else {
				debug_log_output("Unable to open file '%s'\n", work_data);
			}
		}

		if ( skin_rep_data_line_p[count].menu_file_type == TYPE_CHAPTER) {
			snprintf(skin_rep_data_line_p[count].file_uri_link,
				         sizeof(skin_rep_data_line_p[count].file_uri_link),
						 "?action=showchapters%d",
						 file_info_p[i].title);
			strncpy(work_data, file_info_p[i].name, sizeof(work_data));
		}
		count++;
	}
	}

	debug_log_output("-----< end file info generate, count = %d >-----", count);

	if(    (global_param.flag_allplay_includes_subdir)
		&& (skin_rep_data_global_p->stream_dirs > 0)
/*		&& (skin_rep_data_global_p->stream_files == 0)
		&& (skin_rep_data_global_p->music_files == 0)
		&& (skin_rep_data_global_p->photo_files == 0) */ )
	{
		// When subdirectories exist, and recursive allplay is enabled,
		// then enable the default type for this alias, if defined
		switch(http_recv_info_p->default_file_type) {
		case TYPE_MOVIE:
			skin_rep_data_global_p->stream_files++;
			break;
		case TYPE_MUSIC:
			skin_rep_data_global_p->music_files++;
			break;
		case TYPE_JPEG:
			skin_rep_data_global_p->photo_files++;
			break;
		}
	}

	// ============================
	// Hidden directory search
	// ============================

	memset(skin_rep_data_global_p->secret_dir_link_html, '\0', sizeof(skin_rep_data_global_p->secret_dir_link_html));

	// Check whether the hiding directory exists.
	for ( i=0; i<SECRET_DIRECTORY_MAX; i++)
	{
		if ( strlen(secret_directory_list[i].dir_name) > 0 )	// 隠しディレクトリ指定有り？

		{
			// ----------------------------------
			// Full pass formation of hiding directory
			// ----------------------------------
			strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
			if ( work_filename[strlen(work_filename)-1] != '/' )
			{
				strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
			}
			strncat(work_filename, secret_directory_list[i].dir_name, sizeof(work_filename) - strlen(work_filename));
			debug_log_output("check: work_filename = %s", work_filename);

			// Existence check
			result = stat(work_filename, &dir_stat);
			if ( result == 0 )
			{
				if ( S_ISDIR(dir_stat.st_mode) != 0 ) // ディレクトリ存在！
				{
					// Existing, the cod, URI formation for link
					strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );
					strncat(work_data, secret_directory_list[i].dir_name, sizeof(work_data) - strlen(work_data) );
					uri_encode(work_data2, sizeof(work_data2), work_data, strlen(work_data) );

					// HTML formation
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

	send_skin_filemenu(accept_socket, skin_rep_data_global_p, skin_rep_data_line_p, count, delete_mode);

	free( skin_rep_data_global_p );
	free( skin_rep_data_line_p );
}

// ==================================================
//  It reads the skin, substitutes and transmits
// ==================================================
static void send_skin_filemenu(int accept_socket, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p, int lines, int delete_mode)
{
	SKIN_MAPPING_T *sm_ptr;
	SKIN_T *header_skin;
	SKIN_T *line_skin[MAX_TYPES];
	SKIN_T *tail_skin;
	int i;
	int count;
	unsigned char *menu_work_p;

	// HTTP_OK header transmission
	http_send_ok_header(accept_socket, 0, NULL);

	// ===============================
	// HEAD skin file reading & substitution & transmission
	// ===============================
	if ((header_skin = skin_open(delete_mode ? SKIN_DELETE_HEAD_HTML : SKIN_MENU_HEAD_HTML)) == NULL) {
		return ;
	}
	// Substituting the data inside SKIN directly
	skin_direct_replace_global(header_skin, skin_rep_data_global_p);
	skin_direct_send(accept_socket, header_skin);
	skin_close(header_skin);

	// ===============================
	// The skin file reading & substitution & transmission for LINE
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

	// Start of LINE substitution processing.
	for (count=0; count < lines; count++) {
		int mtype;

		mtype = skin_rep_data_line_p[count].menu_file_type;
		strncpy(menu_work_p, skin_get_string(line_skin[line_skin[mtype] != NULL ? mtype : TYPE_UNKNOWN]), MAX_SKIN_FILESIZE);
		replase_skin_line_data(menu_work_p, MAX_SKIN_FILESIZE, &skin_rep_data_line_p[count] );
		replase_skin_grobal_data(menu_work_p, MAX_SKIN_FILESIZE, skin_rep_data_global_p);

		// Each time transmission
		send(accept_socket, menu_work_p, strlen(menu_work_p), 0);
	}

	// Working space release
	free( menu_work_p );

	// The skin release for LINE
	for (i=0; i<MAX_TYPES; i++) {
		if (line_skin[i] != NULL) skin_close(line_skin[i]);
	}



	// ===============================
	// TAIL skin file reading & substitution & transmission

	// ===============================
	if ((tail_skin = skin_open(delete_mode ? SKIN_DELETE_TAIL_HTML : SKIN_MENU_TAIL_HTML)) == NULL) {
		return ;
	}
	// Substituting the data inside SKIN directly
	skin_direct_replace_global(tail_skin, skin_rep_data_global_p);
	skin_direct_send(accept_socket, tail_skin);
	skin_close(tail_skin);

	return;
}



// **************************************************************************
// The number of files which exist in the directory which is appointed with *path is counted
//
// Return: The number of files
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

	if(global_param.num_aliases && (strcasecmp(path, global_param.document_root) == 0)) {
		debug_log_output("Adding %d aliases to root directory\n", global_param.num_aliases);
		count += global_param.num_aliases;
	}

	debug_log_output("count_file_num() end. counter=%d", count);
	return count;
}

// **************************************************************************
// The number of files which exist in the TSV file which is appointed with *path is counted
//
// Return: The number of files
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
// Equal to the number of files reads the information which exists in the directory.
//
// Return: The quantity of file information which it reads
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
	// When the '/' has been attached lastly, deletion
	cut_character_at_linetail(dir_name, '/');
	// 'Deleting before from the /'
	cut_before_last_character(dir_name, '/');

	count = 0;

	// If this is the root directory, then add in any aliases defined in wizd.conf
	// Insert these first, so they appear at the top in an unsorted list
	if(global_param.num_aliases && (strcasecmp(path, global_param.document_root) == 0)) {
		int i;
		for(i=0; (i < global_param.num_aliases) && (count < file_num); i++) {
			debug_log_output("Inserting alias '%s'\n", global_param.alias_name[i]);
			strncpy(file_info_p[count].name, global_param.alias_name[i], sizeof(file_info_p[count].name));
			snprintf(file_info_p[count].org_name, sizeof(file_info_p[count].org_name), "%s/", global_param.alias_name[i]);
			file_info_p[count].ext[0] = '\0';
			file_info_p[count].type = S_IFDIR;
			file_info_p[count].size = 0;
			file_info_p[count].time = 0;
			count++;
		}
	}

	while ( 1 )
	{
		if ( count >= file_num )
			break;

		// From directory, file name 1 GET
		dent = readdir(dir);
		if ( dent == NULL  )
			break;

		// Disregard file check.
		if ( file_ignoral_check(dent->d_name, path) != 0 ) {
			debug_log_output("dent->d_name='%s' file_ignoral_check failed ", dent->d_name);
			continue;
		}

		//debug_log_output("dent->d_name='%s'", dent->d_name);


		// Full pass file name formation
		strncpy(fullpath_filename, path, sizeof(fullpath_filename) );
		strncat(fullpath_filename, dent->d_name, sizeof(fullpath_filename) - strlen(fullpath_filename) );

		//debug_log_output("fullpath_filename='%s'", fullpath_filename );

		// stat() 実行
		memset(&file_stat, 0, sizeof(file_stat));
		result = stat(fullpath_filename, &file_stat);
		if ( result < 0 ) {
			debug_log_output("fullpath_filename='%s' stat failed ", fullpath_filename);
			continue;
		}


		// When the object is the directory, identical name directory check with SVI.
		if (( S_ISDIR( file_stat.st_mode ) != 0 ) && ( global_param.flag_hide_same_svi_name_directory == TRUE ))
		{
			// Execution of check 
			if ( directory_same_check_svi_name(fullpath_filename) != 0 ) {
				debug_log_output("fullpath_filename='%s' directory_same_check_svi_name failed ", fullpath_filename);
				continue;
			}
		}

		// Retaining original file name
		if (S_ISDIR( file_stat.st_mode )) {
			snprintf(file_info_p[count].org_name, sizeof(file_info_p[count].org_name), "%s/", dent->d_name);
		} else {
			strncpy(file_info_p[count].org_name, dent->d_name, sizeof(file_info_p[count].org_name) );
		}

		// EUCに変換！
		convert_language_code(	dent->d_name, file_info_p[count].name,
								sizeof(file_info_p[count].name),
								global_param.server_language_code | CODE_HEX, CODE_EUC );

		// Specification processing when it is not the directory
		if (S_ISDIR(file_stat.st_mode) == 0) {
			// () [ ] Deletion flag check
			// If the flag with TRUE, the file is not the directory, the parenthesis is deleted.
			if (global_param.flag_filename_cut_parenthesis_area == TRUE) {
				cut_enclose_words(file_info_p[count].name, sizeof(file_info_p[count].name), "(", ")");
				cut_enclose_words(file_info_p[count].name, sizeof(file_info_p[count].name), "[", "]");
				debug_log_output("file_name(enclose_words)='%s'\n", file_info_p[count].name);
			}

			// Directory same name character string deletion flag check
			// If the flag with TRUE, the file is not the directory, deleting the identical character string.			if (( global_param.flag_filename_cut_same_directory_name == TRUE )
			 if (( global_param.flag_filename_cut_same_directory_name == TRUE )&& ( strlen(dir_name) > 0 ))
			{
				// "" With substituting the directory same name character string
				replase_character_first(file_info_p[count].name, sizeof(file_info_p[count].name), dir_name, "");

				// If it seems that is attached in the head ' ' deletion.
				cut_first_character(file_info_p[count].name, ' ');

				debug_log_output("file_name(cut_same_directory_name)='%s'\n", file_info_p[count].name);
			}

			// If file, separation of extension
			if ((work_p = strrchr(file_info_p[count].name, '.')) != NULL) {
				// この時点で file_info_p[count].name は 拡張子ナシになる。
				*work_p++ = '\0';
				strncpy(file_info_p[count].ext, work_p, sizeof(file_info_p[count].ext));
				debug_log_output("ext = '%s'", file_info_p[count].ext);
			}
		} else {
			// If the directory there is no extension
			file_info_p[count].ext[0] = '\0';
		}

		// In addition retaining information
		file_info_p[count].type = file_stat.st_mode;
		file_info_p[count].size = file_stat.st_size;
		file_info_p[count].time = file_stat.st_mtime;
		file_info_p[count].duration.hour = -1;


		// When it is the SVI file, file_info_p [ count ] size it replaces.
		if (( strcasecmp(file_info_p[count].ext, "svi") ==  0 )
		 || ( strcasecmp(file_info_p[count].ext, "sv3") ==  0 )
		) {
			file_info_p[count].size = svi_file_total_size(fullpath_filename);
		}

		// Vob first file check v0.12f3
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
// TSVEqual to the number of entrys reads the information which exists in the file.
//
// Return: The quantity of file information which it reads
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

		// Full pass file name formation
		if (fname[0] == '/') {
			strncpy(fullpath_filename, global_param.document_root, sizeof(fullpath_filename) );
		} else {
			strncpy(fullpath_filename, path, sizeof(fullpath_filename) );
		}
		strncat(fullpath_filename, fname, sizeof(fullpath_filename) - strlen(fullpath_filename) );

		// Retaining file name
		strncpy(file_info_p[count].org_name, fname, sizeof(file_info_p[count].org_name) );
		// to euc
		convert_language_code(	title, file_info_p[count].name, sizeof(file_info_p[count].name),
								CODE_AUTO | CODE_HEX, CODE_EUC );


		debug_log_output("fullpath_filename='%s'", fullpath_filename );

		// Forming the extension 
		// Case of tsv, org_name (original fname) from it forms 
		filename_to_extension( fname, file_info_p[count].ext, sizeof(file_info_p[count].ext) );

		// stat() 実行
		memset(&file_stat, 0, sizeof(file_stat));
		result = stat(fullpath_filename, &file_stat);
		if ( result >= 0 ) {
			// Substantial discovery
			file_info_p[count].type = file_stat.st_mode;
			file_info_p[count].size = file_stat.st_size;
			file_info_p[count].time = file_stat.st_mtime;

			// When the directory being understood, deleting the extension 
			if (S_ISDIR(file_stat.st_mode)) {
				file_info_p[count].ext[0] = '\0';
			}
		} else {
		// There is no substance. Depending, type obscurity of date and size and the file 


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
// File disregard check
// Inside the directory, it judges whether there is no oak which ignores the file.
// Return: 0:OK -1 disregard 
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
	// Skipping those which correspond to description above ignore_names
	// ==================================================================
	for (i=0; i<ignore_count; i++) {
		if (!strcmp(name, ignore_names[i])) return -1;
	}

	// ==================================================================
	// For MacOSX with ". the file which starts the _" skip (the resource file)
	// ==================================================================
	if ( strncmp(name, "._", 2 ) == 0 )
	{
		return ( -1 );
	}


	// ==================================================================
	// When the file hiding flag where wizd does not know is passed, extension check
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
			// Check whether file, truly file.
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
	// Hidden Dir check
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
// HTTP_OK header formation & transmission
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

static char* create_1line_playlist(HTTP_RECV_INFO *http_recv_info_p, char *path, FILE_INFO_T *file_info_p, off_t offset)
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
	if(mime == NULL)
		return NULL;

	// ------------------------------------------------------
	// If outside the playback object it returns
	// ------------------------------------------------------
	if ( mime->menu_file_type != TYPE_MOVIE
	 &&	 mime->menu_file_type != TYPE_MUSIC
	 &&	 mime->menu_file_type != TYPE_JPEG
	 &&	 mime->menu_file_type != TYPE_SVI) {
		return NULL;
	}

	// Restrict the file types included in the playlist
	switch( http_recv_info_p->default_file_type ) {
	default:
		if ( mime->menu_file_type != TYPE_MOVIE
		 &&	 mime->menu_file_type != TYPE_MUSIC
		 &&	 mime->menu_file_type != TYPE_JPEG
		 &&	 mime->menu_file_type != TYPE_SVI) {
			return NULL;
		}
		break;
	case TYPE_MOVIE:
		if ( mime->menu_file_type != TYPE_MOVIE
		 &&	 mime->menu_file_type != TYPE_SVI) {
			return NULL;
		}
		break;
	case TYPE_MUSIC:
	case TYPE_MUSICLIST:
		if ( mime->menu_file_type != TYPE_MUSIC) {
			return NULL;
		}
		break;
	case TYPE_JPEG:
	case TYPE_PLAYLIST:
		if ( mime->menu_file_type != TYPE_JPEG) {
			return NULL;
		}
		break;
	}

	// -----------------------------------------
	// If extension mp3, ID3 tag check.
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
			strncat(work_data, path, sizeof(work_data) - strlen(work_data) );
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
		// If in EUC conversion -> extension deletion -> (necessity if) half angle letter //in full size conversion -> in the MediaWiz cord/code conversion -> SJIS, illegitimate //character code 0x7C (the '|') deleting the letter which is included.
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
	// For Link URI (encoding to be completed) it forms 
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
		strncat(work_data, path, sizeof(work_data) - strlen(work_data) );
		strncat(work_data, file_info_p->org_name, sizeof(work_data)- strlen(work_data) );
	}

	// When generating default music playlists, just return the filename
	if (http_recv_info_p->default_file_type == TYPE_MUSICLIST) {
		strncat(work_data, "\r\n", sizeof(work_data)-strlen(work_data));
		return work_data;
	}

	uri_encode(file_uri_link, sizeof(file_uri_link), work_data, strlen(work_data) );

	if (mime->menu_file_type == TYPE_JPEG) {
		strncat(file_uri_link, "?action=Resize.jpg", sizeof(file_uri_link)- strlen(file_uri_link) );
	}

	debug_log_output("file_uri_link='%s'\n", file_uri_link);

	// The extension replacement processing of URI.
	extension_add_rename(file_uri_link, sizeof(file_uri_link));

	// ------------------------------------
	// Forming the play list 
	// ------------------------------------
	if(    ( http_recv_info_p->default_file_type == TYPE_JPEG )
		|| ( http_recv_info_p->default_file_type == TYPE_PLAYLIST ) ) {
		// Different playlist type for images
		// First number is slide show duration, in seconds
		// Second number is (???) transition type
		// Third entry is the slide label
		// Fourth entry is the slide URI
		if(!global_param.flag_slide_show_labels)
			file_name[0]=0;

		snprintf(work_data, sizeof(work_data), "%d|%d|%s|http://%s%s|\r\n"
			, global_param.slide_show_seconds
			, global_param.slide_show_transition
			, file_name
			, http_recv_info_p->recv_host, file_uri_link
		);
	} else {
		snprintf(work_data, sizeof(work_data), "%s%s|%lld|%d|http://%s%s|\r\n"
			, (offset==0) ? "" : "Resume> "
			, file_name
			, offset, 0
			, http_recv_info_p->recv_host, file_uri_link
		);
	}

	debug_log_output("work_data='%s'", work_data);

	return work_data;
}


// **************************************************************************
// * Forming the play list for allplay
// **************************************************************************
static void create_all_play_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num)
{
	int i,count=0;

	debug_log_output("create_all_play_list() start.");

	// =================================
	// ファイル表示用情報 生成＆送信
	// =================================
	for ( i=0; (i<file_num) && (count < global_param.max_play_list_items) ; i++ )
	{
		char *ptr;

		// ディレクトリは無視
		if ( S_ISDIR( file_info_p[i].type ) != 0 ) {
			if( global_param.flag_allplay_includes_subdir ) {
				// Do a depth-first scan of the directories
				char work_buf[1024];
				snprintf(work_buf, sizeof(work_buf), "%s/", file_info_p[i].name);
				count = recurse_all_play_list(accept_socket, http_recv_info_p, work_buf, count, 0);
				continue;
			}
		}

		ptr = create_1line_playlist(http_recv_info_p, "", &file_info_p[i], 0);
		if (ptr != NULL) {
			// 1行づつ、すぐに送信
			write(accept_socket, ptr, strlen(ptr));
			count++;
		}
	}

	debug_log_output("create_all_play_list() found %d items", count);

	return;
}

static int recurse_all_play_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, char *directory, int count, int depth)
{
	FILE_INFO_T *file_info_p;
	char work_buf[1024];
	int i, file_num;
	char *ptr;

	if(depth >= MAX_PLAY_LIST_DEPTH) {
		debug_log_output("recurse_all_play_list: directory '%s', depth %d - Max depth reached - returning\n", directory, depth);
		return count;
	}
	if(count >= global_param.max_play_list_items) {
		debug_log_output("recurse_all_play_list: directory '%s', count %d - Max count reached - returning\n", directory, count);
		return count;
	}

	// recv_uri ディレクトリのファイル数を数える。
	strncpy(work_buf, http_recv_info_p->send_filename, sizeof(work_buf));
	strncat(work_buf, directory, sizeof(work_buf)-strlen(work_buf));
	file_num = count_file_num( work_buf );


	debug_log_output("directory '%s', depth %d, file_num = %d\n", directory, depth, file_num);
	if ( file_num <= 0 )
		return count;

	// 必要な数だけ、ファイル情報保存エリアをmalloc()
	file_info_p = (FILE_INFO_T *)malloc( sizeof(FILE_INFO_T)*file_num );
	if ( file_info_p == NULL )
	{
		debug_log_output("malloc() error");
		return count;
	}

	memset(file_info_p, 0, sizeof(FILE_INFO_T)*file_num);

	file_num = directory_stat(work_buf, file_info_p, file_num);
	debug_log_output("directory_stat(%s) returned %d items\n", directory, file_num);
	for ( i=0; (i<file_num) && (count < global_param.max_play_list_items) ; i++ )
	{

		// ディレクトリは無視
		if ( S_ISDIR( file_info_p[i].type ) != 0 ) {
			// Do a depth-first scan of the directories
			snprintf(work_buf, sizeof(work_buf), "%s%s/", directory, file_info_p[i].name);
			count = recurse_all_play_list(accept_socket, http_recv_info_p, work_buf, count, depth+1);
		} else {
			ptr = create_1line_playlist(http_recv_info_p, directory, &file_info_p[i], 0);
			if (ptr != NULL) {
				// 1行づつ、すぐに送信
				write(accept_socket, ptr, strlen(ptr));
				count++;
			}
		}
	}
	free(file_info_p);
	return count;
}


static void create_shuffle_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num)
{
	int i,count=0;
	char **list = (char **)malloc(global_param.max_play_list_items*sizeof(char *));

	debug_log_output("create_shuffle_list() start.");

	// =================================
	// Information formation & transmission for file indication 
	// =================================
	for ( i=0; (i<file_num) && (count < global_param.max_play_list_items) ; i++ )
	{
		char *ptr;

		// ディレクトリは無視
		if ( S_ISDIR( file_info_p[i].type ) != 0 ) {
			if( global_param.flag_allplay_includes_subdir ) {
				// Do a depth-first scan of the directories
				char work_buf[WIZD_FILENAME_MAX];
				snprintf(work_buf, sizeof(work_buf), "%s/", file_info_p[i].name);
				count = recurse_shuffle_list(http_recv_info_p, work_buf, list, count, 0);
			}
		} else {
			ptr = create_1line_playlist(http_recv_info_p, "", &file_info_p[i], 0);
			if (ptr != NULL) {
				// 1行づつ、すぐに送信
				list[count++] = strdup(ptr);
			}
		}
	}

	debug_log_output("create_shuffle_list() found %d items", count);

	// Random shuffle
	srand(time(NULL));
	while(count > 0) {
		i = (int)floor((double)rand() / (double)RAND_MAX * (double)count);
		if(i<count) {
			write(accept_socket, list[i], strlen(list[i]));
			free(list[i]);
			list[i] = list[--count];
		}
	}

	free(list);

	return;
}

static int recurse_shuffle_list(HTTP_RECV_INFO *http_recv_info_p, char *directory, char **list, int count, int depth)
{
	FILE_INFO_T *file_info_p;
	char work_buf[WIZD_FILENAME_MAX];
	int i, file_num;
	char *ptr;

	if(depth >= MAX_PLAY_LIST_DEPTH) {
		debug_log_output("recurse_shuffle_list: directory '%s', depth %d - Max depth reached - returning\n", directory, depth);
		return count;
	}
	if(count >= global_param.max_play_list_items) {
		debug_log_output("recurse_shuffle_list: directory '%s', count %d - Max count reached - returning\n", directory, count);
		return count;
	}

	// recv_uri ディレクトリのファイル数を数える。
	strncpy(work_buf, http_recv_info_p->send_filename, sizeof(work_buf));
	strncat(work_buf, directory, sizeof(work_buf)-strlen(work_buf));
	file_num = count_file_num( work_buf );

	debug_log_output("directory '%s', depth %d, file_num = %d\n", directory, depth, file_num);
	if ( file_num <= 0 )
		return count;

	// 必要な数だけ、ファイル情報保存エリアをmalloc()
	file_info_p = (FILE_INFO_T *)malloc( sizeof(FILE_INFO_T)*file_num );
	if ( file_info_p == NULL )
	{
		debug_log_output("malloc() error");
		return count;
	}

	memset(file_info_p, 0, sizeof(FILE_INFO_T)*file_num);

	file_num = directory_stat(work_buf, file_info_p, file_num);
	debug_log_output("directory_stat(%s) returned %d items\n", directory, file_num);
	for ( i=0; (i<file_num) && (count < global_param.max_play_list_items) ; i++ )
	{

		// As for directory disregard 
		if ( S_ISDIR( file_info_p[i].type ) != 0 ) {
			// Do a depth-first scan of the directories
			snprintf(work_buf, sizeof(work_buf), "%s%s/", directory, file_info_p[i].name);
			count = recurse_shuffle_list(http_recv_info_p, work_buf, list, count, depth+1);
		} else {
			ptr = create_1line_playlist(http_recv_info_p, directory, &file_info_p[i], 0);
			if (ptr != NULL) {
				// 1行づつ、すぐに送信
				list[count++] = strdup(ptr);
				debug_log_output("shuffle item %d: '%s'\n", count, ptr);
			}
		}
	}
	free(file_info_p);
	return count;
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

		// File position decision
		row = ( SORT_DIR_FLAG( type ) == SORT_DIR_DOWN ) ? 0 : nDir;
	}
	else
	{
		// ファイルソート対象を全件にする
		nFile = num;
	}

	// File sort is done
	// If the directory has not become the object, it makes all the case objects
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
static int _file_info_duration_sort( const void *in_a, const void *in_b)
{
	FILE_INFO_T *a, *b;
	int			 time_a, time_b;

	a = (FILE_INFO_T *) in_a;
	b = (FILE_INFO_T *) in_b;
	time_a = (a->duration.hour * 3600) + (a->duration.minute * 60) + a->duration.second;
	time_b = (b->duration.hour * 3600) + (b->duration.minute * 60) + b->duration.second;
	return (int)(time_b - time_a );
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
// * Forming the image viewer, it replies
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
	// Data generation for substitution 
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

	// It diverges with the extension
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
	// Execution of substitution 
	//   Substituting the data inside SKIN directly 
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
	off_t	offset=0;

	if(global_param.bookmark_threshold) {
		// Check for a bookmark
		char	work_buf[WIZD_FILENAME_MAX];
		FILE	*fp;
		snprintf(work_buf, sizeof(work_buf), "%s.wizd.bookmark", http_recv_info_p->send_filename);
		debug_log_output("Checking for bookmark: '%s'", work_buf);
		fp = fopen(work_buf, "r");
		if(fp != NULL) {
			fgets(work_buf, sizeof(work_buf), fp);
			fclose(fp);
			offset = atoll(work_buf);
			debug_log_output("Bookmark offset: %lld", offset);
		}
	}

	// HTTP_OKヘッダ送信
	http_send_ok_header(accept_socket, 0, NULL);

	// Clear out any playlist type overrides
	http_recv_info_p->default_file_type = TYPE_UNKNOWN;

	ptr = create_1line_playlist(http_recv_info_p, "", NULL, offset);
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

	unsigned char	*ptr;
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

		// Adjusting the file name inside the play list one for Windows 
		if (global_param.flag_filename_adjustment_for_windows){
			filename_adjustment_for_windows(buf, http_recv_info_p->send_filename);
		}

		// Extension formation
		filename_to_extension(buf, file_extension, sizeof(file_extension) );
		debug_log_output("file_extension:'%s'", file_extension);

		// 表示ファイル名 生成
		strncpy(work_data, buf, sizeof(work_data));
		cut_before_last_character( work_data, '/' );
		strncpy( file_name, work_data, sizeof(file_name));
		debug_log_output("file_name:'%s'", file_name);


		// If already in URI form, send it unmodified, but with the labels
		ptr = strstr(buf,"http:");
		if(ptr != NULL) {
			cut_character_at_linetail(ptr, '|');
			cut_character_at_linetail(file_name, '|');
			cut_after_character(file_name, '?');
			if(http_recv_info_p->default_file_type == TYPE_PLAYLIST) {
				if(!global_param.flag_slide_show_labels)
					file_name[0]=0;
				send_printf(accept_socket, "%d|%d|%s|%s|\r\n", 
					global_param.slide_show_seconds, global_param.slide_show_transition, file_name, ptr );
			} else {
				send_printf(accept_socket, "%s|0|0|%s|\r\n", file_name, ptr );
			}
		} else {
		// URI生成
		if ( buf[0] == '/') // 絶対パス
		{
			strncpy( file_uri_link, buf, sizeof(file_uri_link) );
		} else if (global_param.num_aliases > 0) {
			int i,len;

			for(i=0; i<global_param.num_aliases; i++) {
				len = strlen(global_param.alias_path[i]);
				debug_log_output("checking if %s is an alias for %s\n", global_param.alias_path[i], buf);
				if(strncasecmp(buf, global_param.alias_path[i], len)==0) {
					debug_log_output("FOUND IT! substituting alias path '%s' for buf '%s', path is %s, len is %d\n",
						global_param.alias_name[i], buf, global_param.alias_path[i], len);
					// The name matches an alias - substitute the alias path
					strcpy(file_uri_link, "/");
					strcat(file_uri_link, global_param.alias_name[i]);
					strcat(file_uri_link, "/");
					strcat(file_uri_link, buf + len);
					break;
				}
			}
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


// If the MP3 ID3 tag is existed, playlist indicatory file name is replaced with the ID3 tag.
		if ( mp3_id3tag_data.mp3_id3v1_flag == 1 )
		{
			debug_log_output("got flag of 1, change title_info of %s\n", mp3_id3tag_data.mp3_id3v1_title_info);
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
			// If in EUC conversion -> extension deletion -> (necessity if) half angle letter in full size //conversion -> in the MediaWiz cord/code conversion -> SJIS, illegitimate character code 0x7C (the '|') deleting //the letter which is included.
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

		debug_log_output("file_uri_link(reencoded):'%s'", file_uri_link);


		// ------------------------------------
		// プレイリストを生成
		// ------------------------------------
		send_printf(accept_socket, "%s|%d|%d|http://%s%s|\r\n"
			, file_name
			, 0, 0
			, http_recv_info_p->recv_host,
			file_uri_link
		);
		}
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
#define		SKIN_KEYWORD_CURRENT_PATH_LINK_DVDOPT	"<!--WIZD_INSERT_CURRENT_PATH_LINK_DVDOPT-->"

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
	// Data generation for substitution 
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
	REPLACE(CURRENT_PATH_LINK_DVDOPT, current_uri_link_no_sort);
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
// Japanese character code conversion.
// (The wrapper function of libnkf)
//
//	The type which is supported is as follows.
//		in_flag:	CODE_AUTO, CODE_SJIS, CODE_EUC, CODE_UTF8, CODE_UTF16
//		out_flag: 	CODE_SJIS, CODE_EUC
/********************************************************************************/
void convert_language_code(const unsigned char *in, unsigned char *out, size_t len, int in_flag, int out_flag)
{
	unsigned char	nkf_option[128];

	memset(nkf_option, '\0', sizeof(nkf_option));

	if(   ((out_flag & CODE_DISABLED) == CODE_DISABLED)
	   || ((in_flag  & CODE_DISABLED) == CODE_DISABLED)) {
		// Bypass the character set conversion routines
		while(*in && (--len>0))
			*(out++)=*(in++);
		/* must null terminate */
		*(out) = 0;
		return;
	}

	//=====================================================================
	// in_flag, out_flagをみて、libnkfへのオプションを組み立てる。
	//=====================================================================
	switch( in_flag & 0xf )
	{
		case CODE_SJIS:
			strncpy(nkf_option, "S", sizeof(nkf_option)); // sjis-input
			break;

		case CODE_EUC:
			strncpy(nkf_option, "E", sizeof(nkf_option)); // euc-input
			break;

		case CODE_UTF8:
			strncpy(nkf_option, "W", sizeof(nkf_option)); // utf8-input
			break;

		case CODE_UTF16:
			strncpy(nkf_option, "W16", sizeof(nkf_option)); // utf16-input
			break;

		case CODE_AUTO:
		default:
			strncpy(nkf_option, "", sizeof(nkf_option));
			break;
	}


	switch( out_flag )
	{
		case CODE_EUC:
			strncat(nkf_option, "e", sizeof(nkf_option) - strlen(nkf_option) ); // euc-output
			break;

		case CODE_UNIX:
			strncat(nkf_option, "eLu", sizeof(nkf_option) - strlen(nkf_option) ); // unix-output
			break;

		case CODE_WINDOWS:
			strncat(nkf_option, "sLw", sizeof(nkf_option) - strlen(nkf_option) ); // windows-output
			break;

		case CODE_UTF8:
			strncat(nkf_option, "w", sizeof(nkf_option) - strlen(nkf_option) ); // utf8-output
			break;

		case CODE_UTF16:
			strncat(nkf_option, "w16", sizeof(nkf_option) - strlen(nkf_option) ); // utf16-output
			break;

		case CODE_SJIS:
		default:
			strncat(nkf_option, "s", sizeof(nkf_option) - strlen(nkf_option) ); // sjis-output
			break;
	}

	// It leaves also the CAP/HEX letter conversion of SAMBA, to nkf.
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
	// Tag information read
	//
	//	When the character string 0xFF ' ' has been attached lastly, deletion.
	//  It converts to the client character code.
	// ------------------------------------------------------------


	// Tune name
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 30);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_title,
							sizeof(skin_rep_data_line_p->mp3_id3v1_title),
							CODE_AUTO, global_param.client_language_code);


	// Artist
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 30);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_artist,
							sizeof(skin_rep_data_line_p->mp3_id3v1_artist),
							CODE_AUTO, global_param.client_language_code);

	// Album name 
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 30);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_album,
							sizeof(skin_rep_data_line_p->mp3_id3v1_album),
							CODE_AUTO, global_param.client_language_code);

	// Vintage 
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 4);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_year,
							sizeof(skin_rep_data_line_p->mp3_id3v1_year),
							CODE_AUTO, global_param.client_language_code);

	// Comment
	memset(buf, '\0', sizeof(buf));
	read(fd, buf, 28);
	cut_character_at_linetail(buf, 0xFF);
	cut_character_at_linetail(buf, ' ');
	convert_language_code( 	buf,
							skin_rep_data_line_p->mp3_id3v1_comment,
							sizeof(skin_rep_data_line_p->mp3_id3v1_comment),
							CODE_AUTO, global_param.client_language_code);

	// ---------------------
	// Existence flag
	// ---------------------
	skin_rep_data_line_p->mp3_id3v1_flag = 1;

	close(fd);
}

static unsigned long id3v2_len(unsigned char *buf)
{
	return buf[0] * 0x200000 + buf[1] * 0x4000 + buf[2] * 0x80 + buf[3];
}

// **************************************************************************
// * From the MP3 file, the tag data of ID3v2 type is obtained
// * 0: Success -1: The failure (there is no tag)
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
	// "ID3" character string verification
	// ------------------

	// 10byteをread.
	read(fd, buf, 10);
	// debug_log_output("buf='%s'", buf);

	// "ID3" character string check
	if ( strncmp( buf, "ID3", 3 ) != 0 )
	{
		/*
		 *  Adhering to the rear of the file, the る ID3v2 tag
		 *  When there is on the middle of the file therefore trouble you do not read.
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
	// Tag information read 
	//
	//  It converts to the client character code.
	// ------------------------------------------------------------

	while (len > 0) {
		int frame_len;

		/* The frame header is read*/
		if (read(fd, buf, 10) != 10) {
			close(fd);
			return -1;
		}

		/* Calculating the length of the frame */
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
// The indicatory file name which transfers to playlist, is adjusted the type which is not problem
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


// Adjusting the file name inside the play list one for Windows
//
// Concretely, it does below
// Pass '\' of pause modification to '/'
// Windows corresponds being to be case insensitive
//
// Furthermore, there is a function, for the characters on screen line, playlist_filename_adjustment but do not confuse! ! !
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

	// '\' of pass pause modification to '/'
	// It is the processing which designates that it is sjis as prerequisite
	curp = filename;
	sjis_f = FALSE;
	while (*curp){
		if (!sjis_f){
			if (isleadbyte(*curp)){
				// sjisの1byte目

				
				sjis_f = TRUE;
			} else if (*curp == '\\'){
				*curp = '/';			// Replacement
			}
		} else {
			// sjisの2byte目
			sjis_f = FALSE;
		}
		curp++;
	}


	// Acquiring the directory name of the single step eye from the first
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


	// Loop every of directory class
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
				strncpy(curp, dent->d_name, strlen(dent->d_name));		
// 該当部分を本来の名前に置き換え
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

