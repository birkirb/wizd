//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_menu.c
//											$Revision: 1.44 $
//											$Date: 2006/11/05 19:05:43 $
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
#include <errno.h>
#include <libgen.h>
#include <regex.h>
//#include <sys/cygwin.h>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>


#include "wizd.h"
#include "wizd_aviread.h"
#define __MAIN
#include "wizd_mp3.h"
#undef __MAIN

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
	unsigned char	chapter_str[WIZD_FILENAME_MAX];
	unsigned char	org_name[WIZD_FILENAME_MAX];	// ���ꥸ�ʥ�ե�����̾
	unsigned char	full_pathname[WIZD_FILENAME_MAX];
	unsigned char	ext[32];						// ���ꥸ�ʥ�ե������ĥ��
	mode_t			type;			// ����
	off_t			size;			// ������
	// use off_t instead of size_t, since it contains st_size of stat.
	time_t			time;			// ����
	dvd_duration	duration;
	int			    title;
	int				chap;
	int				is_longest;
	unsigned int        start_sector;
	unsigned int        end_sector;
	int                 title_set_nr;
	int                 skip;
} FILE_INFO_T;





#define		FILEMENU_BUF_SIZE	(1024*16)


static int count_file_num(unsigned char *path);
static int directory_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num);
static int count_file_num_in_tsv(unsigned char *path);
static int tsv_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num);

static int file_ignoral_check(unsigned char *name, unsigned char *path);
static int directory_same_check_svi_name(unsigned char *name);

static void send_skin_filemenu(int accept_socket, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p, int lines, int dvdfolder, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_ino_p, int file_num, int search);

extern unsigned char *skin_file_read(unsigned char *read_filename, unsigned long *malloc_size);

//static void create_system_filemenu(HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, unsigned char *send_filemenu_buf, int buf_size);
static void create_skin_filemenu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, int search);
static void create_all_play_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int max_items, int file_num);
static char* create_1line_playlist(HTTP_RECV_INFO *http_recv_info_p, char *path, FILE_INFO_T *file_info_p, off_t offset);
static int recurse_all_play_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, char *directory, int count, int max_items); //, int depth);

static void create_shuffle_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, int max_items);
static int recurse_shuffle_list(HTTP_RECV_INFO *http_recv_info_p, char *directory, char **list, int count, int max_items); //, int depth);

static int recurse_directory_list(HTTP_RECV_INFO *http_recv_info_p, char **list, int count, int max_items, int depth);

#define MAX_PLAY_LIST_DEPTH 10

static void  mp3_id3_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p, int limited_length_max, int scan_type );

static void playlist_filename_adjustment(unsigned char *src_name, unsigned char *dist_name, int dist_size, int input_code );

static int file_read_line( int fd, unsigned char *line_buf, int line_buf_size);

static void filename_adjustment_for_windows(unsigned char *filename, const unsigned char *pathname_plw);

static int read_avi_info(char *fname, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p);
static void do_search(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

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
	_file_info_duration_sort,
};


// �ǥ��쥯�ȥ꥽�����Ѵؿ�����
static void * dir_sort_api[] = {
	NULL,
	_file_info_dir_sort_order_up,
	_file_info_dir_sort_order_down
};


static void
dvd_get_duration(ifo_handle_t * ifo_title,
				 tt_srpt_t * tt_srpt,
				 int *accum_hour,
				 int *accum_minute,
				 int *accum_second, int titleid, FILE_INFO_T * file_info_p)
{
	pgc_t              *cur_pgc;
	vts_ptt_srpt_t     *vts_ptt_srpt;
	int                 pgc_id, ttn, pgn, diff;
	int                 hour, minute, second, tmp;
	int                 cur_cell = 0;
	int                 last_pgc = -1;
	int                 i, j;
	int                 end_cell;

	vts_ptt_srpt = ifo_title->vts_ptt_srpt;
	ttn = tt_srpt->title[titleid].vts_ttn;
	*accum_hour = *accum_minute = *accum_second = 0;

	debug_log_output("title %d has %d ptts, ttn is %d\n", titleid + 1,
					 tt_srpt->title[titleid].nr_of_ptts, ttn);

	/* loop through the chapters */
	for (i = 0; i < tt_srpt->title[titleid].nr_of_ptts; i++) {
		pgc_id = vts_ptt_srpt->title[ttn - 1].ptt[i].pgcn;
		if (pgc_id != last_pgc) {
			cur_cell = 0;
			last_pgc = pgc_id;
		}

		pgn = vts_ptt_srpt->title[ttn - 1].ptt[i].pgn;
		cur_pgc = ifo_title->vts_pgcit->pgci_srp[pgc_id - 1].pgc;

		debug_log_output("next pgc is %d\n", cur_pgc->next_pgc_nr);

		if (i == 0)
			file_info_p->start_sector =
				cur_pgc->cell_playback[cur_pgc->program_map[pgn - 1] - 1].first_sector;

		if (pgn == cur_pgc->nr_of_programs) {
			diff = cur_pgc->nr_of_cells - cur_pgc->program_map[pgn - 1] + 1;
			end_cell = cur_pgc->nr_of_cells - 1;
		} else {
			diff = cur_pgc->program_map[pgn] - cur_pgc->program_map[pgn - 1];
			end_cell = cur_pgc->program_map[pgn - 1] - 1;
		}

		/* find last cell that has some time on it */
		for (;;) {
			file_info_p->end_sector =
				cur_pgc->cell_playback[end_cell].last_sector;

			if (cur_pgc->cell_playback[end_cell].playback_time.second > 1
				|| cur_pgc->cell_playback[end_cell].playback_time.minute != 0
				|| cur_pgc->cell_playback[end_cell].playback_time.hour != 0) {

				if (file_info_p->end_sector >= file_info_p->start_sector) {
					debug_log_output("end (%d) > start (%d)\n",
									 file_info_p->end_sector,
									 file_info_p->start_sector);
					break;
				}
			} else
				debug_log_output("skip short cell\n");

			if (end_cell == 0) {
				debug_log_output("cells are equal, %d\n", end_cell);
				break;
			}

			/* diff--; */
			end_cell--;

			debug_log_output
				("skip end_cell %d, end_sector %d, start_sector %d\n",
				 end_cell + 1, file_info_p->end_sector,
				 file_info_p->start_sector);
		}

		debug_log_output("        end_sector %d\n", file_info_p->end_sector);
		debug_log_output("PGC_%d, Ch %d, Pg %d (%d cells)\n", pgc_id, i + 1,
						 pgn, diff);

		/* loop through cells of this chapter */
		hour = minute = second = 0;
		for (j = 0; j < diff; j++) {
			tmp = cur_pgc->cell_playback[cur_cell].playback_time.second;
			tmp = (tmp & 0xf) + ((tmp & 0xf0) >> 4) * 10;
			second += tmp;
			if (second >= 60) {
				minute++;
				second -= 60;
			}

			tmp = cur_pgc->cell_playback[cur_cell].playback_time.minute;
			tmp = (tmp & 0xf) + ((tmp & 0xf0) >> 4) * 10;
			minute += tmp;
			if (minute >= 60) {
				hour++;
				minute -= 60;
			}

			tmp = cur_pgc->cell_playback[cur_cell].playback_time.hour;
			tmp = (tmp & 0xf) + ((tmp & 0xf0) >> 4) * 10;
			hour += tmp;

			debug_log_output("    Cell %02d, %02x:%02x:%02x\n", cur_cell + 1,
							 cur_pgc->cell_playback[cur_cell].playback_time.
							 hour,
							 cur_pgc->cell_playback[cur_cell].playback_time.
							 minute,
							 cur_pgc->cell_playback[cur_cell].playback_time.
							 second);

			cur_cell++;
		}

		*accum_second += second;
		if (*accum_second >= 60) {
			(*accum_minute)++;
			*accum_second = (*accum_second) - 60;
		}

		*accum_minute = (*accum_minute) + minute;
		if (*accum_minute >= 60) {
			(*accum_hour)++;
			(*accum_minute) = *accum_minute - 60;
		}

		*accum_hour += hour;
	}

	debug_log_output("title %d: start_sector %d, end_sector %d\n",
					 titleid + 1, file_info_p->start_sector,
					 file_info_p->end_sector);
}

static int
dvd_check_for_subsets(FILE_INFO_T * file_info_p, int file_num)
{
	int                 i, j;
	int                 skip_count;
	int                 time_a;
	int                 time_b;

	skip_count = 0;
	for (i = 1; i < file_num; i++) {
		for (j = 0; j < file_num; j++) {
			if (i == j)
				continue;

			if (file_info_p[j].title_set_nr != file_info_p[i].title_set_nr)
				continue;

			if (file_info_p[j].skip == 1)
				continue;

			if (file_info_p[j].start_sector <= file_info_p[i].start_sector &&
				file_info_p[j].end_sector >= file_info_p[i].end_sector) {

				time_a =
					(file_info_p[i].duration.hour * 3600) +
					(file_info_p[i].duration.minute * 60) +
					file_info_p[i].duration.second;

				time_b =
					(file_info_p[j].duration.hour * 3600) +
					(file_info_p[j].duration.minute * 60) +
					file_info_p[j].duration.second;

				if (file_info_p[j].start_sector != file_info_p[i].start_sector||
					file_info_p[j].end_sector != file_info_p[i].end_sector ||
					time_a < time_b) {

					file_info_p[i].duration.hour = 0;
					file_info_p[i].duration.minute = 0;
					file_info_p[i].duration.second = 0;
					skip_count++;

					debug_log_output
						("Title %d in VTS %d is a subset of %d ((%d,%d) subset of (%d,%d))\n",
						 file_info_p[i].title, file_info_p[j].title_set_nr,
						 file_info_p[j].title, file_info_p[i].start_sector,
						 file_info_p[i].end_sector, file_info_p[j].start_sector,
						 file_info_p[j].end_sector);

					file_info_p[i].skip = 1;

					break;
				}
			}
		}
	}

	return (skip_count);
}

static void
set_sort_rule(HTTP_RECV_INFO *http_recv_info_p, int file_num)
{
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
	} else if (file_num == 1)
		global_param.sort_rule = SORT_NONE;
}


// **************************************************************************
// �ե�����ꥹ�Ȥ����������ֿ�
// **************************************************************************
void http_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	int		file_num=0;	// DIR��Υե������
	int				sort_rule;  // temp�֤�������
	unsigned char	*file_info_malloc_p;
	FILE_INFO_T		*file_info_p;
	dvd_reader_t *dvd=NULL;
	ifo_handle_t *ifo_file=NULL;
	ifo_handle_t *ifo_title=NULL;
	int got_title_ifo;
	tt_srpt_t *tt_srpt=NULL;
	pgc_t		*cur_pgc;
	vts_ptt_srpt_t     *vts_ptt_srpt;
	int                 accum_hour, accum_minute, accum_second;
	int pgc_id, ttn;
	int real_count;
	int count;

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
	off_t	start_offset,len;

	debug_log_output("http_menu start, recv_uri='%s', send_filename='%s', action %s, option %s, file_type 0x%x, alias '%s', search '%s', search type %d, lsearch %s, default type %d\n", http_recv_info_p->recv_uri, http_recv_info_p->send_filename, http_recv_info_p->action, http_recv_info_p->option, http_recv_info_p->file_type, http_recv_info_p->alias, http_recv_info_p->search, http_recv_info_p->search_type, http_recv_info_p->lsearch, http_recv_info_p->default_file_type);

	if (http_recv_info_p->search[0] != '\0') {
		do_search(accept_socket, http_recv_info_p);
		return;
	}

	if (http_recv_info_p->file_type == ISO_TYPE ) {
		dvd = DVDOpen( http_recv_info_p->send_filename );
		if(dvd != NULL) {
			debug_log_output("DVD image found in '%s'", http_recv_info_p->send_filename );
		} else {
			debug_log_output("Failed to open DVD from image '%s'", http_recv_info_p->send_filename );
		}
	} else if(http_recv_info_p->file_type != DIRECTORY_TYPE ) {
		// It counts the number of files of the recv_uri directory.			
		if (http_recv_info_p->file_type == TSV_TYPE)
			file_num = count_file_num_in_tsv( http_recv_info_p->send_filename );
		else
			file_num = 1;
	} else {
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

		// See if we have a DVD directory structure in the subdirectories
		snprintf(work_data, sizeof(work_data), "%sVIDEO_TS/VIDEO_TS.IFO", http_recv_info_p->send_filename);
		debug_log_output( "Check for DVD structure: %s \n", work_data );
		if(0 == stat(work_data, &dir_stat)) {
			debug_log_output( "VIDEO_TS.IFO found\n" );
			dvd = DVDOpen( http_recv_info_p->send_filename );
			if(dvd != NULL) {
				debug_log_output( "DVD file structure found: %s \n", http_recv_info_p->send_filename );
			}
		} else {
			debug_log_output( "VIDEO_TS.IFO not found\n" );
			// ==================================
			// Standard directory information GET
			// ==================================
			// counts the number of files of the recv_uri directory.				
			if (http_recv_info_p->alias[0] == '\0')
				file_num = count_file_num( http_recv_info_p->send_filename );
			else {
				file_num = 0;
				for (i = 0; i < global_param.num_aliases; i++) {
					if (strcmp(http_recv_info_p->alias, "movie") == 0) {
						if (global_param.alias_default_file_type[i] == TYPE_MOVIE)
							file_num++;
					} else if (strcmp(http_recv_info_p->alias, "music") == 0) {
						if (global_param.alias_default_file_type[i] == TYPE_MUSIC)
							file_num++;
					} else if (strcmp(http_recv_info_p->alias, "photo") == 0) {
						if (global_param.alias_default_file_type[i] == TYPE_JPEG)
							file_num++;
					} else if (strcmp(http_recv_info_p->alias, "alias") == 0) {
						if (global_param.alias_default_file_type[i] == TYPE_UNKNOWN)
							file_num++;
					} else if(global_param.alias_default_file_type[i] != TYPE_SECRET)
						file_num++;
				}
			}
		}
	}

	if( dvd != NULL ) {
		ifo_file = ifoOpen( dvd, 0 );
		if( ifo_file ) {
			if (print_ifo)
				ifoPrint_VTS_ATRT(ifo_file->vts_atrt);
			tt_srpt = ifo_file->tt_srpt;

			debug_log_output( "Found DVD with %d titles.\n", tt_srpt->nr_of_srpts );

			if (strncmp(http_recv_info_p->action, "showchapters", 12) == 0 ){
				int titleid = http_recv_info_p->title - 1;
				if((titleid >= 0) && (titleid < tt_srpt->nr_of_srpts))
					file_num = tt_srpt->title[ titleid ].nr_of_ptts;
				else
					file_num = 0;
				debug_log_output("showchapters got %d files\n", file_num);
			} else {
				file_num = tt_srpt->nr_of_srpts;
			}
			if(file_num == 0) {
				// Fall back to normal directory listings
				ifoClose( ifo_file );
				DVDClose( dvd );
				dvd = NULL;
			}
		} else {
			debug_log_output("The DVD '%s' does not contain an IFO\n", http_recv_info_p->send_filename );
			DVDClose( dvd );
			dvd = NULL;
		}
		if( dvd == NULL ) {
			// Fall back to normal directory listing
			file_num = count_file_num( http_recv_info_p->send_filename );
		}
	}

	debug_log_output("at top file_num = %d", file_num);
	if ( file_num < 0 )
	{
		return;
	}

	// ɬ�פʿ��������ե����������¸���ꥢ��malloc()
	// Always allocate one extra block, in case we need it for the VIDEO_TS directory on DVDs
	file_info_malloc_p = malloc( sizeof(FILE_INFO_T)*(file_num+1) );
	if ( file_info_malloc_p == NULL )
	{
		debug_log_output("malloc() error");
		return;
	}

	memset(file_info_malloc_p, 0, sizeof(FILE_INFO_T)*(file_num+1));
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
	real_count = file_num;
	if (dvd) {
		if (global_param.sort_rule != SORT_DURATION)
			global_param.sort_rule = SORT_NONE;

		if (strcasecmp(http_recv_info_p->action, "dvdplay") == 0) {
			int titleid = http_recv_info_p->title - 1;
			int chap = http_recv_info_p->page;

			debug_log_output("got dvdplay, file_info_p[i].chap %s\n", http_recv_info_p->action);
			if ((titleid < 0) || (titleid >= file_num))
				titleid = 0;

			offset = 0;

			// Unless starting at a chapter point, check for any bookmarks
			if(global_param.bookmark_threshold && (chap==0)) {
				// Check for a bookmark
				FILE 				*fp;
				ifo_handle_t		*vts_file = NULL;

				if (http_recv_info_p->file_type == ISO_TYPE) {
					snprintf(work_data2, sizeof(work_data2), "%s.wizd.bookmark", 
						http_recv_info_p->send_filename);
				} else {
					snprintf(work_data2, sizeof(work_data2),
							 "%sVIDEO_TS/VTS_%02d_1.VOB.wizd.bookmark", 
							 http_recv_info_p->send_filename,
							 tt_srpt->title[titleid].title_set_nr);
				}

				debug_log_output("Checking for bookmark: '%s'", work_data2);

				fp = fopen(work_data2, "r");
				if (fp != NULL) {
					int bookmark_page=0;
					fgets(work_data2, sizeof(work_data2), fp);
					offset = atoll(work_data2);
					if (fgets(work_data2, sizeof(work_data2), fp) != NULL) {
						len = atoll(work_data2);
						if (fgets(work_data2, sizeof(work_data2), fp) != NULL) {
							bookmark_page = atol(work_data2);
						}
						debug_log_output("Read bookmark: %lld/%lld (page %d)", 
							offset, len, bookmark_page);

						// Ignore bookmarks at the EOF
						if(offset >= len)
							offset = 0;
					} else
						len = 0;

					fclose(fp);
					// Make sure the bookmark is sector-aligned
					// (2048-byte sectors on a DVD)
					offset -= (offset & 0x7ff);
					debug_log_output("Bookmark offset: %lld/%lld", offset, len);

					vts_file = ifoOpen( dvd, tt_srpt->title[ titleid ].title_set_nr );
					if (vts_file != NULL) {
						int	pgn, start_cell;

						// Compute the offset relative to the chapter start
						// and adjust the starting "chap" to match the overall offset from the start of the file
						start_offset = 0;
						ttn = tt_srpt->title[titleid].vts_ttn;
						for (chap=0; chap < tt_srpt->title[ titleid ].nr_of_ptts; chap++) {
							// Find the beginning of this chapter
							pgc_id = vts_file->vts_ptt_srpt->title[ ttn - 1 ].ptt[ chap ].pgcn;
							pgn = vts_file->vts_ptt_srpt->title[ ttn - 1 ].ptt[ chap ].pgn;
							cur_pgc = vts_file->vts_pgcit->pgci_srp[ pgc_id - 1 ].pgc;
							start_cell = cur_pgc->program_map[ pgn - 1 ] - 1;
							len = cur_pgc->cell_playback[ start_cell ].first_sector;
							len *= 2048;
							if(offset < len)
								break;

							// Save this chapter's starting offset for next pass
							start_offset=len;
						}

						// Start from this chapter plus the relative offset
						offset -= start_offset;
						ifoClose(vts_file);
					}
				}
			}

			debug_log_output("DVD Title %d Playlist Create Start!!! ",
							 titleid + 1);

			http_send_ok_header(accept_socket, 0, NULL);

			// Change any chapter selection to base-0
			if (chap > 0)
				chap--;
			for(j=0; j<tt_srpt->title[ titleid ].nr_of_ptts; j++) {
				i=chap+j;
				if(i >= tt_srpt->title[ titleid ].nr_of_ptts)
					i -= tt_srpt->title[ titleid ].nr_of_ptts;
				if(http_recv_info_p->file_type == ISO_TYPE) {
					strncpy(work_data2, http_recv_info_p->recv_uri, sizeof(work_data));
				} else {
					snprintf(work_data, sizeof(work_data),
							 "%sVIDEO_TS/VTS_%02d_1.VOB",
							 http_recv_info_p->recv_uri,
							 tt_srpt->title[titleid].title_set_nr);

					uri_encode(work_data2, sizeof(work_data2), work_data,
							   strlen(work_data));
				}

				// Always use page= method of selecting chapter,
			   	// since LinkPlayer doesn't support the other method
				// Use title= to indicate which title to play
				// Use the page= to indicate which chapter to start from
              		  	snprintf(work_data, sizeof(work_data),
                        		 "Chapter %d%s|%lld|0|http://%s%s?title=%d&page=%d&%sfile=chapter%d.mpg|\n",
                        		 i+1, (offset == 0) ? "" : ">>", offset,
                         		http_recv_info_p->recv_host, work_data2,
                         		(titleid + 1), i, dvdoptbuf, i+1);
				offset = 0;
				debug_log_output("%d: %s", i + 1, work_data);
				send(accept_socket, work_data, strlen(work_data), 0);
			}

			debug_log_output("DVD Title Playlist Create End!!! ");

			ifoClose(ifo_file);
			DVDClose(dvd);
			return;
		} else if (strncmp(http_recv_info_p->action, "showchapters", 12) == 0) {
			int                 titleid;
			int                 pgn, diff;
			int                 hour, minute, second, tmp;
			int                 cur_cell = 0;
			int                 last_pgc = -1;

			debug_log_output("got showchapters %s\n", http_recv_info_p->action);
			titleid = http_recv_info_p->title - 1;
			if((titleid >= 0) && (titleid < tt_srpt->nr_of_srpts)) {
			ifo_title = ifoOpen(dvd, tt_srpt->title[titleid].title_set_nr);

			vts_ptt_srpt = ifo_title->vts_ptt_srpt;
			ttn = tt_srpt->title[titleid].vts_ttn;
			accum_hour = accum_minute = accum_second = 0;

			debug_log_output("title %d has %d ptts, ttn is %d\n", titleid + 1,
							 tt_srpt->title[titleid].nr_of_ptts, ttn);

			real_count = 0;
			for (i = 0; i < tt_srpt->title[titleid].nr_of_ptts; i++) {
				pgc_id = vts_ptt_srpt->title[ttn - 1].ptt[i].pgcn;
				if (pgc_id != last_pgc) {
					cur_cell = 0;
					last_pgc = pgc_id;
				}

				pgn = vts_ptt_srpt->title[ttn - 1].ptt[i].pgn;
				cur_pgc = ifo_title->vts_pgcit->pgci_srp[pgc_id - 1].pgc;
				if (pgn == cur_pgc->nr_of_programs)
					diff =
						cur_pgc->nr_of_cells - cur_pgc->program_map[pgn - 1] +
						1;
				else
					diff =
						cur_pgc->program_map[pgn] - cur_pgc->program_map[pgn -
																		 1];
				debug_log_output("PGC_%d, Ch %d, Pg %d (%d cells)\n", pgc_id,
								 i + 1, pgn, diff);

				hour = minute = second = 0;
				for (j = 0; j < diff; j++) {
					tmp = cur_pgc->cell_playback[cur_cell].playback_time.second;
					tmp = (tmp & 0xf) + ((tmp & 0xf0) >> 4) * 10;
					second += tmp;
					if (second >= 60) {
						minute++;
						second -= 60;
					}

					tmp = cur_pgc->cell_playback[cur_cell].playback_time.minute;
					tmp = (tmp & 0xf) + ((tmp & 0xf0) >> 4) * 10;
					minute += tmp;
					if (minute >= 60) {
						hour++;
						minute -= 60;
					}

					tmp = cur_pgc->cell_playback[cur_cell].playback_time.hour;
					tmp = (tmp & 0xf) + ((tmp & 0xf0) >> 4) * 10;
					hour += tmp;

					debug_log_output("    Cell %02d, %02x:%02x:%02x\n",
									 cur_cell + 1,
									 cur_pgc->cell_playback[cur_cell].
									 playback_time.hour,
									 cur_pgc->cell_playback[cur_cell].
									 playback_time.minute,
									 cur_pgc->cell_playback[cur_cell].
									 playback_time.second);

					cur_cell++;
				}

				if (hour == 0 && minute == 0 && second <= 1) {
					/* don't include short chapters, typically last one */
					file_info_p[i].duration.hour = 0;
					file_info_p[i].duration.minute = 0;
					file_info_p[i].duration.second = 0;
					continue;
				}

				real_count++;

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

				file_info_p[i].type = TYPE_CHAPTER;
				file_info_p[i].size = -1;
				file_info_p[i].time = 0;
				file_info_p[i].is_longest = 1;
				file_info_p[i].duration.hour = accum_hour;
				file_info_p[i].duration.minute = accum_minute;
				file_info_p[i].duration.second = accum_second;

				file_info_p[i].title = titleid+1;
				file_info_p[i].chap = i+1;
				strncpy(file_info_p[i].ext, "plw",
						 sizeof(file_info_p[i].ext));

				if (hour < 1) 
					snprintf(file_info_p[i].name, sizeof(file_info_p[i].name),
							 "Title %d Chapter %d  ( %d:%02d )", titleid + 1,
							 i + 1, minute, second);
				else
					snprintf(file_info_p[i].name, sizeof(file_info_p[i].name),
							 "Title %d Chapter %d  ( %d:%02d:%02d )",
							 titleid + 1, i + 1, hour, minute, second);

				file_info_p[i].org_name[0] = 0;
			}

			ifoClose(ifo_title);

			http_recv_info_p->sort[0] = '\0';
			global_param.sort_rule = SORT_NONE;

			file_num = 0;
			for (i = 0; (i < tt_srpt->title[titleid].nr_of_ptts) && (file_num < real_count); i++) {
				if (file_info_p[i].duration.hour == 0 &&
					file_info_p[i].duration.minute == 0 &&
					file_info_p[i].duration.second <= 1) {

					debug_log_output("removing chapter %d, %02d:%02d:%02d\n",
									 i,
									 file_info_p[i].duration.hour,
									 file_info_p[i].duration.minute,
									 file_info_p[i].duration.second);

					for (j = i; j < tt_srpt->title[titleid].nr_of_ptts - 1; j++)
						file_info_p[j] = file_info_p[j + 1];

					i--;
				} else
					file_num++;
			}

			debug_log_output("remove done\n");
			} else {
				// Invalid titleid number, so we have no files
				file_num = 0;
			}
		} else { // Create MENU with Titles and Chapters
			int shift = 0;
			debug_log_output("action not set %s\n", http_recv_info_p->action);
# if 0
			file_info_p[0].type = 0;
			file_info_p[0].size = -1;
			file_info_p[0].time = 0;
			file_info_p[0].is_longest = 0;
			file_info_p[0].duration.hour = 99; /* put at top of menu */
			file_info_p[0].title = 0;
			file_info_p[0].chap = 0;
			strncpy(file_info_p[0].ext, "chapter",
					sizeof(file_info_p[0].ext));
			snprintf(file_info_p[0].name, sizeof(file_info_p[0].name),
					"VIDEO_TS");
			file_info_p[0].org_name[0] = 0;
			shift=1;
# else
			// When DVD source is a directory (and not an image)
			// then insert a reference to VIDEO_TS for manual file browsing
			if(http_recv_info_p->file_type != ISO_TYPE) {
				debug_log_output("Inserting a reference to VIDEO_TS");
				file_info_p[0].duration.hour = 99; /* put at top of menu */
				if (global_param.flag_fancy_video_ts_page)
					file_info_p[0].type = TYPE_VIDEO_TS;
				else
					file_info_p[0].type = TYPE_DIRECTORY;
				file_info_p[0].size = 0;
				file_info_p[0].time = 0;
				file_info_p[0].is_longest = 0;
				file_info_p[0].title = 0;
				file_info_p[0].chap = 0;
				file_info_p[0].ext[0] = 0;
				if (http_recv_info_p->page < 2)
					strcpy(http_recv_info_p->focus, "L2");
				else
					strcpy(http_recv_info_p->focus, "L1");
				strncpy(file_info_p[0].name, "VIDEO_TS", sizeof(file_info_p[0].name));
				strncpy(file_info_p[0].org_name, "VIDEO_TS/", sizeof(file_info_p[0].org_name));
				shift = 1;
			} else {
				debug_log_output("Skipping reference to VIDEO_TS, file_num remains %d", file_num);
			}
# endif

			global_param.sort_rule = SORT_DURATION;

			got_title_ifo = -1;

			real_count = 0;
			for(i=0 ; i < file_num; ++i ) {
				if (got_title_ifo != tt_srpt->title[i].title_set_nr) {
					if (got_title_ifo != -1)
							ifoClose(ifo_title);

					ifo_title =
							ifoOpen(dvd, tt_srpt->title[i].title_set_nr);

					if (ifo_title && print_ifo)
						ifoPrint_VTS_ATRT(ifo_title->vts_atrt);

					got_title_ifo = tt_srpt->title[i].title_set_nr;
				}
				
				dvd_get_duration(ifo_title, tt_srpt, &accum_hour,
								 &accum_minute, &accum_second, i,
								 &file_info_p[i + shift]);

				debug_log_output("got hour/minute/second: %02d:%02d:%02d\n",
								 accum_hour, accum_minute, accum_second);

				file_info_p[i + shift].duration.hour = accum_hour;
				file_info_p[i + shift].duration.minute = accum_minute;
				file_info_p[i + shift].duration.second = accum_second;

				if (global_param.flag_hide_short_titles && accum_hour == 0
					&& accum_minute == 0 && accum_second <= 1) {
					file_info_p[i + shift].skip = 1;
					continue;
				}

				real_count++;

				time_a =
					(file_info_p[i + shift].duration.hour * 3600) +
					(file_info_p[i + shift].duration.minute * 60) +
					file_info_p[i + shift].duration.second;

				if (time_a >= longest_duration) {
					longest_title = i + shift;
					longest_duration = time_a;
				}

				file_info_p[i + shift].type = TYPE_PLAYLIST;
				file_info_p[i + shift].size = -1;
				file_info_p[i + shift].time = 0;
				file_info_p[i + shift].is_longest = 0;
				file_info_p[i + shift].title_set_nr = got_title_ifo;
				file_info_p[i + shift].skip = 0;

				strncpy(file_info_p[i + shift].ext, "plw",
						sizeof(file_info_p[i + shift].ext));

				strcpy(work_data, "Chapters ");

				if (ifo_title->vtsi_mat->vts_video_attr.
					display_aspect_ratio == 3)
					strcat(work_data, "16x9 ");
				else
					strcat(work_data, "4x3 ");

				if (global_param.flag_show_audio_info) {
					if (ifo_title->vtsi_mat->nr_of_vts_audio_streams > 0) {
						first_time = 1;
						for (j = 0;
							 j < ifo_title->vtsi_mat->nr_of_vts_audio_streams;
							 j++) {

							if (ifo_title->vts_pgcit->pgci_srp[0].pgc->audio_control[j] == 0)
								continue;

							if (first_time) {
								strcat(work_data, "(");
								first_time = 0;
							} else
								strcat(work_data, ", ");

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

							if (ifo_title->vtsi_mat->vts_audio_attr[j].
								channels == 5)
								strcat(work_data, "5.1");
							else
								strcat(work_data, "2.0");
	
							if (ifo_title->vtsi_mat->vts_audio_attr[j].lang_code != 0)
								sprintf(work_data2, " %c%c",
									ifo_title->vtsi_mat->vts_audio_attr[j].lang_code >> 8,
									ifo_title->vtsi_mat->vts_audio_attr[j].lang_code % 256);
							strcat(work_data, work_data2);
						}

						if (!first_time) {
							if (tt_srpt->title[ i - 1 ].nr_of_angles > 1)
								sprintf(&work_data[strlen(work_data)], " %d angles)", tt_srpt->title[i - 1].nr_of_angles);
							else
								strcat(work_data, ")");
						}
					}
				}

#if 0
				if (global_param.flag_show_audio_info) {
					if (ifo_title->vtsi_mat->nr_of_vts_subp_streams > 0) {
						first_time = 1;
						for (j = 0;
							 j < ifo_title->vtsi_mat->nr_of_vts_subp_streams;
							 j++) {
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

				file_info_p[i + shift].title = i+1;
				file_info_p[i + shift].chap = 0;
				snprintf(file_info_p[i + shift].name, sizeof(file_info_p[i + shift].name),
						 "Title %d", i+1);

				snprintf(file_info_p[i + shift].chapter_str,
						 sizeof(file_info_p[i + shift].chapter_str), "%d %s",
						 tt_srpt->title[i].nr_of_ptts, work_data);

				debug_log_output("chapter str is %s\n",
								 file_info_p[i + shift].chapter_str);

				memset(file_info_p[i + shift].org_name, 0,
					   sizeof(file_info_p[i + shift].org_name));

				debug_log_output( "Title %d\n", i+1);
				debug_log_output("\tIn VTS: %d [TTN %d]\n",
					tt_srpt->title[ i ].title_set_nr,
					tt_srpt->title[ i ].vts_ttn );
				debug_log_output("\tTitle has %d chapters and %d angles\n",
					tt_srpt->title[ i ].nr_of_ptts,
					tt_srpt->title[ i ].nr_of_angles );
			}

			// If we added VIDEO_TS, need to increment the file count too
			file_num += shift;
			real_count += shift;

			/* see if any titles are subsets of others */
			real_count -= dvd_check_for_subsets(file_info_p, file_num);

			ifoClose(ifo_title);
			ifoClose(ifo_file);
			DVDClose(dvd);
		}
	} else if (http_recv_info_p->file_type != DIRECTORY_TYPE) {
		if (http_recv_info_p->file_type == TSV_TYPE)
			file_num = tsv_stat(http_recv_info_p->send_filename, file_info_p, file_num);
		else {
			file_num = 1;
			if (stat(http_recv_info_p->send_filename, &dir_stat) == 0) {
				file_info_p[0].size = dir_stat.st_size;
				file_info_p[0].time = dir_stat.st_mtime;
				if (strcmp(http_recv_info_p->option, "mp3info") == 0) {
					strcpy(file_info_p[0].ext, "mp3");
					file_info_p[0].type = TYPE_MUSIC;
				} else if (strcmp(http_recv_info_p->option, "aviinfo") == 0) {
					strcpy(file_info_p[0].ext, "avi");
					file_info_p[0].type = TYPE_MOVIE;
				}
				strcpy(file_info_p[0].name, http_recv_info_p->send_filename);
				strcpy(file_info_p[0].org_name,http_recv_info_p->send_filename);
			}
		}
	} else if (http_recv_info_p->alias[0] != '\0') {
		count = 0;
		for (i = 0; i < global_param.num_aliases; i++) {
			if ((strcmp(http_recv_info_p->alias, "movie") == 0 &&
				 global_param.alias_default_file_type[i] != TYPE_MOVIE) ||
			    (strcmp(http_recv_info_p->alias, "alias") == 0 &&
				 global_param.alias_default_file_type[i] != TYPE_UNKNOWN) ||
			    (strcmp(http_recv_info_p->alias, "music") == 0 &&
				 global_param.alias_default_file_type[i] != TYPE_MUSIC) ||
			    (strcmp(http_recv_info_p->alias, "photo") == 0 &&
				 global_param.alias_default_file_type[i] != TYPE_JPEG)) {
				continue;
			}

			if (global_param.alias_default_file_type[i] == TYPE_SECRET)
				continue;

			for (j = 0; (j < i) && (strcmp(global_param.alias_name[i],global_param.alias_name[j]) != 0); j++)
				continue;

			if (j != i)
				continue;

			strncpy(file_info_p[count].name, global_param.alias_name[i], sizeof(file_info_p[count].name));
			snprintf(file_info_p[count].org_name, sizeof(file_info_p[count].org_name), "%s/", global_param.alias_name[i]);
			file_info_p[count].ext[0] = '\0';
			file_info_p[count].type = TYPE_DIRECTORY;
			file_info_p[count].size = 0;
			file_info_p[count].time = 0;
			count++;
		}
		file_num = count;
	} else {
		file_num = directory_stat(http_recv_info_p->send_filename, file_info_p, file_num);
	}

	debug_log_output("file_num = %d", file_num);

	if (longest_title != -1) {
		file_info_p[longest_title].is_longest = 1;
		//file_info_p[longest_title].title = longest_title;
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
	set_sort_rule(http_recv_info_p, file_num);

	sort_rule = global_param.sort_rule;

	// 0.12f4
	if (global_param.flag_specific_dir_sort_type_fix == TRUE) {
	// Locking the SORT method with the specific directory
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

	// If it is necessary, execution of sort
	if ( sort_rule != SORT_NONE )
	{
		if (global_param.flag_sort_dir) {
			if (sort_rule == SORT_NAME_DOWN)
				sort_rule = SORT_NAME_DOWN | SORT_DIR_DOWN;
			else
				sort_rule |= SORT_DIR_UP;
		}

		file_info_sort( file_info_p, file_num, sort_rule);
	}

	if (real_count < file_num)
		file_num = real_count;

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
				create_all_play_list(fd, http_recv_info_p, file_info_p, file_num, global_param.max_play_list_search);
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
				// Allow default file to be up to 10x the max number of items
				create_all_play_list(fd, http_recv_info_p, file_info_p, file_num, global_param.max_play_list_search);
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
			// HTTP_OK�إå�����
			http_send_ok_header(accept_socket, 0, NULL);
			create_all_play_list(accept_socket, http_recv_info_p, file_info_p, file_num, global_param.max_play_list_items);
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
				create_shuffle_list(fd, http_recv_info_p, file_info_p, file_num, global_param.max_play_list_search);
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
				create_shuffle_list(fd, http_recv_info_p, file_info_p, file_num, global_param.max_play_list_search);
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
			// HTTP_OK�إå�����
			http_send_ok_header(accept_socket, 0, NULL);
			create_shuffle_list(accept_socket, http_recv_info_p, file_info_p, file_num, global_param.max_play_list_items);
		}
		debug_log_output("Shuffle List Create End!!! ");
		return ;
	}


	// -------------------------------------------
	// File menu creation and tranmission
	// -------------------------------------------
	create_skin_filemenu(accept_socket, http_recv_info_p, file_info_p, file_num, 0);

	free(file_info_malloc_p);

	return;
}

void
get_uri_path(char *org_name)
{
	int i;
	int len;
	char buf[256];

	for (i = 0; i < global_param.num_aliases; i++) {
		len = strlen(global_param.alias_path[i]);
		if (strncasecmp(org_name, global_param.alias_path[i], len) == 0) {
			sprintf(buf, "/%s/%s", global_param.alias_name[i], &org_name[len]);
			strcpy(org_name, buf);
			return;
		}
	}
}

int
find_real_path(char *org_name, HTTP_RECV_INFO *http_recv_info_p, char *work_filename)
{
	int	i, len;
	char buf[1000];
	struct stat		dir_stat;

	if (strcmp(http_recv_info_p->recv_uri, "/") == 0) {
		sprintf(buf, "%s%s", http_recv_info_p->send_filename, org_name);
		if (stat(buf, &dir_stat) == 0) {
			strcpy(work_filename, buf);
			return(1);
		}
	}

	for (i = 0; i < global_param.num_aliases; i++) {
		len = strlen(global_param.alias_name[i]);
		if (strncmp(&(http_recv_info_p->recv_uri[1]), global_param.alias_name[i], len) == 0) {
			strcpy(buf, global_param.alias_path[i]);
			strcat(buf, http_recv_info_p->recv_uri + len + 1);
			strcat(buf, org_name);
			if (stat(buf, &dir_stat) == 0) {
				strcpy(work_filename, buf);
				return(1);
			}
		}
	}

	return(0);
}

int
is_secret_dir(char *name)
{
	int	i;

	for (i = 0; i < SECRET_DIRECTORY_MAX; i++)
		if (strcmp(name, secret_directory_list[i].dir_name) == 0)
			return (1);

	return(0);
}


// ****************************************************************************************
// Define variety for menu formation.
// ****************************************************************************************

static const char *html_formats[] = { ".htm", ".html", "index.html", "index.htm", NULL };

static void
set_html_file(int menu_file_type, char *html_ret, char *http_name, char *name, char *ext, char *icon_base, char *icon_type)
{
	int				i, j;
	struct stat		dir_stat;
	char			*http_basenamep;
	char			*aname;
	char			html_filename[1000];
	char			path[1000];
	char		    *p = 0;
	int				use_alias = 0;

	if (strcmp(icon_base, "icon") == 0) {
		html_ret[0] = 0;
		// printf("no html found for %s\n", name);
		return;
	}

	// printf("\nmenu_type is %d, http_name %s, name %s\n", menu_file_type, http_name, name);

	// this code is tied to code in http_file_check() in wizd_http.c
	if (( ((menu_file_type & TYPE_MASK) == TYPE_DIRECTORY)
	   || (menu_file_type == TYPE_MOVIE)
	   || (menu_file_type == TYPE_MUSIC)
	   || (menu_file_type == TYPE_SVI) ) ) {

		// first see if any alias matches
		aname = http_name;
		for (i = 0; i < global_param.num_aliases; i++) {
			if (strncmp(http_name, global_param.alias_path[i], strlen(global_param.alias_path[i])) != 0) {
				// printf("skipped, httpname %s != %s\n", http_name, global_param.alias_path[i]);
				continue;
			} else {
				aname = global_param.alias_name[i];
				p = http_name + strlen(global_param.alias_path[i]);
				// printf("found alias name %s, path %s\n", aname, global_param.alias_path[i]);
				use_alias = 1;
				break;
			}
		}

		http_basenamep = basename(http_name);
		for (i = 0; i < global_param.num_aliases; i++) {
			if (use_alias && strcmp(aname, global_param.alias_name[i]) != 0) {
				// printf("skipped, aname %s != %s\n", aname, global_param.alias_name[i]);
				continue;
			}

			// printf("checking path %s, or %s, name %s, i %d, use_alias %d\n", global_param.alias_path[i], http_name, name, i, use_alias);
			for (j = 0; (html_formats[j] != NULL); j++) {
				/* look in current directory */
				if (use_alias) {
					strcpy(path, global_param.alias_path[i]);
					strcat(path, p);
				} else
					strcpy(path, http_name);

				if (j < 2) {
					strcpy(html_filename, "/wizd_");
					strcat(html_filename, name);
					strcat(html_filename, html_formats[j]);
				} else {
					strcpy(html_filename, "/");
					strcat(html_filename, html_formats[j]);
				}
				strcat(path, html_filename);

				// printf("looking for thumb file %s\n", path);
				if (stat(path, &dir_stat) == 0) {
					strcpy(html_ret, "./");
					strcat(html_ret, html_filename);

					// printf("returning %s\n", html_ret);
					return;
				}
			}

			// if path is a directory look in directory below 
			if (use_alias) {
				strcpy(path, global_param.alias_path[i]);
				strcat(path, p);
			} else
				strcpy(path, http_name);
			strcat(path, "/");
			strcat(path, name);
			if (stat(path, &dir_stat) == 0) {
				if ( S_ISDIR(dir_stat.st_mode) != 0 ) {
					for (j = 2; (html_formats[j] != NULL); j++) {
						strcpy(html_filename, path);
						strcat(html_filename, "/");
						strcat(html_filename, html_formats[j]);

						// printf("looking2 for thumb file %s, http name is %s\n", html_filename, http_name);
						if (stat(html_filename, &dir_stat) == 0) {
							strcpy(html_ret, name);
							strcat(html_ret, "/");
							strcat(html_ret, html_formats[j]);

							// printf("returning %s for dir\n", html_ret);
							return;
						}
					}
				}
			}

			// printf("checking dir, i = %d\n", i);
			/* look in the directory above */
			for (j = 0; (html_formats[j] != NULL); j++) {
				if (use_alias) {
					strcpy(html_filename, global_param.alias_path[i]);
					strcat(html_filename, p);
				} else
					strcpy(html_filename, http_name);

				if (j < 2) {
					strcat(html_filename, "/../wizd_");
					strcat(html_filename, http_basenamep);
					strcat(html_filename, html_formats[j]);
				} else {
					strcat(html_filename, "/../");
					strcat(html_filename, html_formats[j]);
				}

				// printf("looking2 for thumb file %s, http name is %s\n", html_filename, http_name);
				if (stat(html_filename, &dir_stat) == 0) {
					if (j < 2) {
						strcpy(html_ret, "../wizd_");
						strcat(html_ret, http_basenamep);
					} else
						strcpy(html_ret, "../");
					strcat(html_ret, html_formats[j]);

					// printf("returning %s for dir\n", html_ret);
					return;
				}
			}

			if (!use_alias)
				break;
		}
	} else
		html_ret[0] = 0;

	// printf("%s not found\n", name);
}

static const char *thumb_formats[] = { ".jpg", ".png", ".gif", "Folder.jpg", "Folder.png", "Folder.gif", "AlbumArtSmall.jpg", "AlbumArtSmall.png", "AlbumArtSmall.gif", NULL };

static void
set_thumb_file(int menu_file_type, char *file_image, char *http_name, char *name, char *ext, char *icon_base, char *icon_type)
{
	int				i, j;
	struct stat		dir_stat;
	char			*http_basenamep;
	char			*aname;
	char			tn_filename[1000];
	char			path[1000];
	char		    *p = 0;
	int				use_alias = 0;
	char			numbuf[20];
	int				no_thumbs;

	if (strcmp(icon_base, "icon") == 0)
		no_thumbs = 1;
	else
		no_thumbs = 0;

	// printf("\nmenu_type is %d, http_name %s, name %s, no_thumbs is %d\n", menu_file_type, http_name, name, no_thumbs);

	// this code is tied to code in http_file_check() in wizd_http.c
	if (!no_thumbs && ( ((menu_file_type & TYPE_MASK) == TYPE_DIRECTORY)
	   || (menu_file_type == TYPE_MOVIE)
	   || (menu_file_type == TYPE_MUSIC)
	   || (menu_file_type == TYPE_SVI) ) ) {

		// first see if any alias matches
		aname = http_name;
		for (i = 0; i < global_param.num_aliases; i++) {
			if (strncmp(http_name, global_param.alias_path[i], strlen(global_param.alias_path[i])) != 0) {
				//printf("skipped, httpname %s != %s\n", http_name, global_param.alias_path[i]);
				continue;
			} else {
				aname = global_param.alias_name[i];
				p = http_name + strlen(global_param.alias_path[i]);
				// printf("found alias name %s, path %s\n", aname, global_param.alias_path[i]);
				use_alias = 1;
				break;
			}
		}

		http_basenamep = basename(http_name);
		for (i = 0; i < global_param.num_aliases; i++) {
			if (use_alias && strcmp(aname, global_param.alias_name[i]) != 0) {
				//printf("skipped, aname %s != %s\n", aname, global_param.alias_name[i]);
				continue;
			}

			// printf("checking path %s, or %s, name %s, i %d, use_alias %d\n", global_param.alias_path[i], http_name, name, i, use_alias);
			for (j = 0; (thumb_formats[j] != NULL); j++) {
				/* look in current directory */
				if (use_alias) {
					strcpy(path, global_param.alias_path[i]);
					strcat(path, p);
				} else
					strcpy(path, http_name);

				if (j < 3) {
					strcpy(tn_filename, "/tn_");
					strcat(tn_filename, name);
					strcat(tn_filename, thumb_formats[j]);
				} else {
					strcpy(tn_filename, "/");
					strcat(tn_filename, thumb_formats[j]);
				}
				strcat(path, tn_filename);

				// printf("looking for thumb file %s\n", path);
				if (stat(path, &dir_stat) == 0) {
					strcpy(file_image, "./");
					strcat(file_image, tn_filename);
					if (use_alias) {
						sprintf(numbuf, ".%02d", i);
						strcat(file_image, numbuf);
					}
					// printf("returning %s\n", file_image);
					return;
				}
			}

			// if path is a directory look in directory below 
			if (use_alias) {
				strcpy(path, global_param.alias_path[i]);
				strcat(path, p);
			} else
				strcpy(path, http_name);
			strcat(path, "/");
			strcat(path, name);
			if (stat(path, &dir_stat) == 0) {
				if ( S_ISDIR(dir_stat.st_mode) != 0 ) {
					for (j = 3; (thumb_formats[j] != NULL); j++) {
						strcpy(tn_filename, path);
						strcat(tn_filename, "/");
						strcat(tn_filename, thumb_formats[j]);

						// printf("looking2 for thumb file %s, http name is %s\n", tn_filename, http_name);
						if (stat(tn_filename, &dir_stat) == 0) {
							strcpy(file_image, name);
							strcat(file_image, "/");
							strcat(file_image, thumb_formats[j]);
							if (use_alias) {
								sprintf(numbuf, ".%02d", i);
								strcat(file_image, numbuf);
							}
							// printf("returning %s for dir\n", file_image);
							return;
						}
					}
				}
			}

			// printf("checking dir, i = %d\n", i);
			/* look in the directory above */
			for (j = 0; (thumb_formats[j] != NULL); j++) {
				if (use_alias) {
					strcpy(tn_filename, global_param.alias_path[i]);
					strcat(tn_filename, p);
				} else
					strcpy(tn_filename, http_name);

				if (j < 3) {
					strcat(tn_filename, "/../tn_");
					strcat(tn_filename, http_basenamep);
					strcat(tn_filename, thumb_formats[j]);
				} else {
					strcat(tn_filename, "/../");
					strcat(tn_filename, thumb_formats[j]);
				}

				// printf("looking2 for thumb file %s, http name is %s\n", tn_filename, http_name);
				if (stat(tn_filename, &dir_stat) == 0) {
					if (j < 3) {
						strcpy(file_image, "../tn_");
						strcat(file_image, http_basenamep);
					} else
						strcpy(file_image, "../");
					strcat(file_image, thumb_formats[j]);
					if (use_alias) {
						sprintf(numbuf, ".%02d", i);
						strcat(file_image, numbuf);
					}
					// printf("returning %s for dir\n", file_image);
					return;
				}
			}

			if (!use_alias)
				break;
		}

		// printf("%s not found\n", name);
	} else if (!no_thumbs && menu_file_type == TYPE_JPEG) {
		// Return a pointer to myself - skin may append Resize.jpg to set the size
		sprintf(file_image, "%s.%s", name, ext);
		// printf("returning %s for jpeg\n", file_image);
		return;
	}

	// If no match found, fall back to default "icon_SKINTYPE.gif"
	for(i=0; (skin_mapping[i].filetype >= 0) && (skin_mapping[i].filetype != menu_file_type); i++)
		;

	if(skin_mapping[i].skin_filename != NULL) {
		sprintf(file_image, "/%s_%s.%s", icon_base, skin_mapping[i].skin_filename, icon_type);
		// printf("returning %s\n", file_image);
	} else {
		file_image[0] = 0;
	}
}

/* XXX */

int
all_files_are_music(HTTP_RECV_INFO *htp_recv_info_p, FILE_INFO_T *file_info_p, int file_num)
{
	int	i;

	for (i = 0; i < file_num; i++) {
		if (file_info_p[i].type != TYPE_MUSIC) {
			// printf("file name is %s\n", file_info_p[i].name);
			return(0);
		}
	}

	return(1);
}

// **************************************************************************
// Forming the file menu which uses the skin
// **************************************************************************

static void create_skin_filemenu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, int search)
{
	int		i,j;
	int     get_tags;

	unsigned char	work_data[WIZD_FILENAME_MAX];
	unsigned char	work_data2[WIZD_FILENAME_MAX];
	unsigned char	work_filename[WIZD_FILENAME_MAX];
	char 		dvdoptbuf[200];

	int		menu_file_type;

	SKIN_REPLASE_GLOBAL_DATA_T	*skin_rep_data_global_p;
	SKIN_REPLASE_LINE_DATA_T	*skin_rep_data_line_p;

	int skin_rep_line_malloc_size;

	unsigned int	rec_time;
	int	count;

	unsigned int	image_width, image_height;

	struct	stat	dir_stat;
	int		result;
	int		special_view = 0;
	int		scan_type;
	int		fancy_cnt;

	// ==========================================
	// Read Skin Config
	// ==========================================

	if (file_num != 0) {
		if (strcasecmp(file_info_p[0].name, "VIDEO_TS") == 0)
			special_view = 1;
		else if (http_recv_info_p->file_type == ISO_TYPE )
			special_view = 2;
		else if (strncasecmp(http_recv_info_p->action, "showchapters", 12) == 0)
			special_view = 3;
		else if (http_recv_info_p->default_file_type == TYPE_MUSIC) {
			if (global_param.flag_fancy_music_page &&
			    all_files_are_music(http_recv_info_p, file_info_p, file_num))
				special_view = 4;
		}
	}

	skin_read_config(SKIN_MENU_CONF);

	fancy_cnt = global_param.fancy_line_cnt;

	// ==========================================
	// HTML Formation
	// ==========================================

	// ===============================
	// Prepare data for display in skin
	// ===============================

	// Initialize the global skin data structure
	skin_rep_data_global_p = skin_create_global_data(http_recv_info_p, file_num);
	if(skin_rep_data_global_p == NULL)
		return;

	if (special_view != 0)
		skin_rep_data_global_p->filename_length_max = global_param.menu_filename_length_max;

	if ((special_view == 4 || file_info_p[0].type == TYPE_VIDEO_TS) && file_num > fancy_cnt) {
		// fix up code to display only fancy_cnt lines for video_ts page
		skin_rep_data_global_p->max_page = ((file_num - fancy_cnt) / skin_rep_data_global_p->items_per_page) + 2;
		if (http_recv_info_p->page <= 1) {
			skin_rep_data_global_p->now_page_line = fancy_cnt;
			skin_rep_data_global_p->start_file_num = 0;
			snprintf(skin_rep_data_global_p->next_page_str, sizeof(skin_rep_data_global_p->next_page_str), "2");
			strcpy(skin_rep_data_global_p->now_page_str, "1");
			skin_rep_data_global_p->now_page = 1;

			sprintf(skin_rep_data_global_p->max_page_str, "%d", 2 + ((file_num - fancy_cnt) / skin_rep_data_global_p->items_per_page));

			skin_rep_data_global_p->end_file_num = fancy_cnt;
			skin_rep_data_global_p->items_per_page = fancy_cnt;
		} else {
			int tmp;

			tmp = fancy_cnt + (skin_rep_data_global_p->items_per_page * (http_recv_info_p->page - 1));
			if (file_num >= tmp)
				skin_rep_data_global_p->now_page_line = skin_rep_data_global_p->items_per_page;
			else {
				tmp = file_num - tmp + skin_rep_data_global_p->items_per_page;
				skin_rep_data_global_p->now_page_line = tmp;
			}
			skin_rep_data_global_p->start_file_num = fancy_cnt + ((http_recv_info_p->page - 2) * skin_rep_data_global_p->items_per_page);
			snprintf(skin_rep_data_global_p->next_page_str, sizeof(skin_rep_data_global_p->next_page_str), "%d", http_recv_info_p->page + 1);
			snprintf(skin_rep_data_global_p->prev_page_str, sizeof(skin_rep_data_global_p->prev_page_str), "%d", http_recv_info_p->page - 1);
			snprintf(skin_rep_data_global_p->now_page_str, sizeof(skin_rep_data_global_p->now_page_str), "%d", http_recv_info_p->page);
			skin_rep_data_global_p->now_page = http_recv_info_p->page;

			sprintf(skin_rep_data_global_p->max_page_str, "%d", 2 + ((file_num - fancy_cnt) / skin_rep_data_global_p->items_per_page));

			skin_rep_data_global_p->end_file_num = skin_rep_data_global_p->items_per_page;
		}
	}

	// Initialize the file items skin data structure
	skin_rep_line_malloc_size = sizeof(SKIN_REPLASE_LINE_DATA_T) * (skin_rep_data_global_p->items_per_page + 1);
	skin_rep_data_line_p 	= malloc( skin_rep_line_malloc_size );
	if ( skin_rep_data_line_p == NULL )
	{
		debug_log_output("malloc() error.");
		return ;
	}
	memset(skin_rep_data_line_p, '\0', skin_rep_line_malloc_size);


	// ---------------------------------
	// Start of information formation for file indication
	// ---------------------------------
	for ( i=0, count=0 ; i < file_num ; i++) {
		// =========================================================
		// File type decision processing
		// =========================================================
		if ( file_info_p[i].type == TYPE_DIRECTORY )
		{
			// �ǥ��쥯�ȥ�
			menu_file_type = file_info_p[i].type;
		} else if( skin_rep_data_global_p->delete_mode ) {
			// In delete mode, display everything else as a deletable file
			menu_file_type = TYPE_DELETE;
		} else {
			// Other than directory
			menu_file_type = file_info_p[i].type;
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
		case TYPE_VIDEO_TS:
			skin_rep_data_global_p->stream_dirs++;
			break;
		}
	if ( (i>=skin_rep_data_global_p->start_file_num) && (i<(skin_rep_data_global_p->start_file_num + skin_rep_data_global_p->now_page_line)) )
	{
		debug_log_output("-----< file info generate, count = %d >-----", count);

		// printf("pathname %s, name %s, orgname %s\n", file_info_p[i].full_pathname, file_info_p[i].name, file_info_p[i].org_name);
		set_thumb_file(menu_file_type,
					   skin_rep_data_line_p[count].file_image,
					   search ? file_info_p[i].full_pathname : http_recv_info_p->send_filename, 
					   file_info_p[i].name,
					   file_info_p[i].ext,
					   ((skin_rep_data_global_p->columns || global_param.flag_thumb_in_details) && special_view != 4 ? "thumb" : "icon"),
					   global_param.menu_icon_type);

		set_html_file(menu_file_type,
					   skin_rep_data_line_p[count].html_link,
					   search ? file_info_p[i].full_pathname : http_recv_info_p->send_filename, 
					   file_info_p[i].name,
					   file_info_p[i].ext,
					   ((skin_rep_data_global_p->columns || global_param.flag_thumb_in_details) && special_view != 4 ? "thumb" : "icon"),
					   global_param.menu_icon_type);

		if (search) {
			// fix up the filename if this is the result of a search
			sprintf(work_filename, "%s/%s", dirname(file_info_p[i].org_name), skin_rep_data_line_p[count].file_image);
			strcpy(skin_rep_data_line_p[count].file_image, work_filename);
			// printf("filename %s\n\n", work_filename);
		}

		// File_info_p [ i ] name EUC (modification wizd 0.12h)
		// When it exceeds length restriction, Cut
		strncpy(work_data, file_info_p[i].name, sizeof(work_data));
		euc_string_cut_n_length(work_data, skin_rep_data_global_p->filename_length_max);

		// Clean up name (remove underscores, change all upper to Title Case
		/*
		printf("before convert %s\n", work_data);
		clean_buffer_text(work_data);
		printf("after convert %s\n", work_data);
		*/

		debug_log_output("file_name(cut)='%s'\n", work_data);

		// MediaWizʸ�������ɤ�
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

		skin_rep_data_line_p[count].menu_file_type = menu_file_type;

		// =========================================================
		// Forming the character string which is necessary for every file type with addition
		// =========================================================

		// ----------------------------
		// Directory specification processing
		// ----------------------------
		if ((skin_rep_data_line_p[count].menu_file_type & TYPE_MASK) == DIRECTORY_BASE) 
		{
			// it adds ��?��
			strncat(skin_rep_data_line_p[count].file_uri_link, "?", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));

			// When sort= is indicated, it takes over that.
			if ( strlen(http_recv_info_p->sort) > 0 )
			{
				snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
				strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			}
# if 0
			if ( strlen(http_recv_info_p->search) > 0)
			{
				snprintf(work_data, sizeof(work_data), "search%s=%s&", http_recv_info_p->search_str, http_recv_info_p->search);
				strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			}
# endif
			// When option= is indicated, append it
			if ( strlen(http_recv_info_p->option) > 0 )
			{
				snprintf(work_data, sizeof(work_data), "option=%s&", http_recv_info_p->option);
				strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			}
			// When dvdopt= is indicated, it takes over that.
			if ( strlen(http_recv_info_p->dvdopt) > 0 )
			{
				snprintf(work_data, sizeof(work_data), "dvdopt=%s&", http_recv_info_p->dvdopt);
				strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			}
			if ( http_recv_info_p->title > 0 )
			{
				snprintf(work_data, sizeof(work_data), "title=%d&", http_recv_info_p->title);
				strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			}
		}

		// New type for ISO. Kinda hacking here...
		if ( skin_rep_data_line_p[count].menu_file_type == TYPE_ISO )
		{
			strncat(skin_rep_data_line_p[count].file_uri_link, "?action=IsoPlay&", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
		}
		// -------------------------------------------------------------


		// -------------------------------------
		// Stream file specification processing
		// -------------------------------------
		if ( (skin_rep_data_line_p[count].menu_file_type & TYPE_MASK) == STREAMED_BASE )
		{
			// vod_string �� vod="0" �򥻥å�
			strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"0\"", sizeof(skin_rep_data_line_p[count].vod_string) );

			// ��ĥ���֤�����������
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
					while(*p++);	// ������õ��
					if (*(p-2) == 'i')	// �Ǹ夬svi��'i'�ʤ�'3'�ˤ���
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

					// The video recording time which was read from SVI, in the character string.					
					snprintf(skin_rep_data_line_p[count].svi_rec_time_data, sizeof(skin_rep_data_line_p[count].svi_rec_time_data),
								"%02d:%02d:%02d", rec_time /3600, (rec_time % 3600) / 60, rec_time % 60 );

					skin_rep_data_line_p[count].menu_file_type = TYPE_SVI;
				} else {
					debug_log_output("File '%s' has no SVI information, reverting to MOVIE type", http_recv_info_p->send_filename);
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
					// SinglePlay �⡼�ɤˤ��롣 �Ȥ������ʤ����� ����2�Ԥ�Фä�����
					strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
					strncat(skin_rep_data_line_p[count].file_uri_link, "?action=SinglePlay&", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
				}

				break;

			case TYPE_PLAYLIST:
			case TYPE_MUSICLIST:
			case TYPE_CHAPTER:
				// ----------------------------
				// playlist file processing
				// ----------------------------

				if(file_info_p[i].org_name[0] == 0) {
					// Special case for DVD directories

					if (global_param.flag_hide_short_titles && file_info_p[i].duration.hour == 0 && file_info_p[i].duration.minute == 0 && file_info_p[i].duration.second <= 1) {
						continue;
					}

					dvdoptbuf[0] = 0;
					if (global_param.flag_split_vob_chapters == 2) {
						if (file_info_p[i].is_longest)
							sprintf(dvdoptbuf, "dvdopt=notitlesplit&");
						else
							sprintf(dvdoptbuf, "dvdopt=splitchapters&");
						debug_log_output("PLAYLIST: dvdoptbuf %s, longest %d\n", dvdoptbuf, i);
					} else if (strlen(http_recv_info_p->dvdopt) != 0)
						sprintf(dvdoptbuf, "dvdopt=%s&", http_recv_info_p->dvdopt);

					snprintf(skin_rep_data_line_p[count].chapter_link,
							 sizeof(skin_rep_data_line_p[count].chapter_link),
							 "%s&action=showchapters&title=%d",
							 skin_rep_data_global_p->current_directory_absolute, file_info_p[i].title);

					// Create the link with absolute path. Needed because of ISO processing.
					snprintf(work_data, sizeof(work_data),
							 "%saction=dvdplay&title=%d&%s",
							 skin_rep_data_global_p->current_directory_absolute,
							 file_info_p[i].title, dvdoptbuf);

					strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
					strcpy(skin_rep_data_line_p[count].chapter_str, file_info_p[i].chapter_str);
					debug_log_output("copied %s to chapter link\n", skin_rep_data_line_p[count].chapter_link);
					debug_log_output("copied %s to chapter str\n", skin_rep_data_line_p[count].chapter_str);
				}
				// vod_string �� vod="playlist"�򥻥å�
				strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
				break;

			case TYPE_ISO:
			default:
				// vod_string �� ���
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
				// GIF�ե�����Υ�������GET
				gif_size( work_filename, &image_width, &image_height );
			}
			else if (strcasecmp( skin_rep_data_line_p[count].file_extension, "png" ) == 0 )
			{
				// PNG�ե�����Υ�������GET
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
				// JPEG�ʤ� SinglePlay�ˤ��ơ������AllPlay�Ǥ������ǽ�ˤ��롣
				strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
				strncat(skin_rep_data_line_p[count].file_uri_link, "action=SinglePlay&", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
			} else {
				if ( strlen(http_recv_info_p->sort) > 0 ) {
				// sort=���ؼ�����Ƥ�����硢���������Ѥ���
					snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
					strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
				}
				if ( strlen(http_recv_info_p->option) > 0 ) {
					snprintf(work_data, sizeof(work_data), "option=%s&", http_recv_info_p->option);
					strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
				}
				if ( strlen(http_recv_info_p->dvdopt) > 0 ) {
					snprintf(work_data, sizeof(work_data), "dvdopt=%s&", http_recv_info_p->dvdopt);
					strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
				}
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
			snprintf(skin_rep_data_line_p[count].avi_duration, sizeof(skin_rep_data_line_p[count].avi_duration), "        ");
			snprintf(skin_rep_data_line_p[count].avi_vcodec, sizeof(skin_rep_data_line_p[count].avi_vcodec), "[none]");
			snprintf(skin_rep_data_line_p[count].avi_acodec, sizeof(skin_rep_data_line_p[count].avi_acodec), "[none]");
			snprintf(skin_rep_data_line_p[count].avi_hvcodec, sizeof(skin_rep_data_line_p[count].avi_hvcodec), "[none]");
			snprintf(skin_rep_data_line_p[count].avi_hacodec, sizeof(skin_rep_data_line_p[count].avi_hacodec), "[none]");
			snprintf(skin_rep_data_line_p[count].file_duration, sizeof(skin_rep_data_line_p[count].file_duration), "        ");

			if (strcasecmp(skin_rep_data_line_p[count].file_extension, "avi") == 0) {
				// ----------------------------------
				// AVI�ե�����Υե�ѥ�����
				// ----------------------------------
				if (strcmp(http_recv_info_p->option, "aviinfo") == 0) {
					char *p;

					skin_rep_data_global_p->columns = 0;
					skin_rep_data_global_p->filename_length_max = global_param.menu_filename_length_max;
					skin_rep_data_line_p[count].menu_file_type = TYPE_AVIINFO;

					strcpy(work_filename, http_recv_info_p->send_filename);
					if ((p = strstr(skin_rep_data_line_p[count].file_uri_link, ".avi")) != 0) {
						if (p[4] == '?') {
							strcpy(work_data, basename(skin_rep_data_line_p[count].file_uri_link));
							strcpy(skin_rep_data_line_p[count].file_uri_link, work_data);
						} else
							p[4] = 0;
					}

					strcpy(work_data2, dirname(http_recv_info_p->send_filename));
					strcpy(work_data, basename(file_info_p[i].name));
					p = strrchr(work_data, '.');
					*p = 0;
					set_thumb_file(TYPE_MOVIE,
								   skin_rep_data_line_p[count].file_image,
								   work_data2,
								   work_data,
								   file_info_p[i].ext,
								   "thumb",
								   global_param.menu_icon_type);
					strcpy(skin_rep_data_line_p[count].file_name_no_ext, work_data);
					get_tags = 1;
				} else {

					strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
					if ( work_filename[strlen(work_filename)-1] != '/' )
					{
						strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
					}
					strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
					debug_log_output("work_filename(avi) = %s", work_filename);

					(void) find_real_path(file_info_p[i].org_name, http_recv_info_p, work_filename);
					get_tags = global_param.flag_read_avi_tag;
				}

				if (get_tags)
					read_avi_info(work_filename, &skin_rep_data_line_p[count]);

				//printf("pathname %s, name %s, orgname %s, currentpath %s\n", file_info_p[i].full_pathname, file_info_p[i].name, file_info_p[i].org_name, skin_rep_data_global_p->current_path_name);
				if (!search)
					strcpy(skin_rep_data_line_p[count].info_link, skin_rep_data_global_p->current_path_name);
				strcat(skin_rep_data_line_p[count].info_link, file_info_p[i].org_name);
# if 0
printf("avi interleaved %s\n", skin_rep_data_line_p[count].avi_is_interleaved);
printf("avi width %s\n", skin_rep_data_line_p[count].image_width);
printf("avi height %s\n", skin_rep_data_line_p[count].image_height);
printf("avi fps %s\n", skin_rep_data_line_p[count].avi_fps);
printf("avi avi duration %s\n", skin_rep_data_line_p[count].avi_duration);
printf("avi vcodec %s\n", skin_rep_data_line_p[count].avi_vcodec);
printf("avi acodec %s\n", skin_rep_data_line_p[count].avi_acodec);
printf("avi hvcodec %s\n", skin_rep_data_line_p[count].avi_hvcodec);
printf("avi hacodec %s\n", skin_rep_data_line_p[count].avi_hacodec);
# endif

				if (skin_rep_data_global_p->columns && skin_rep_data_line_p[count].avi_duration[2] != ' ')
					skin_rep_data_line_p[count].file_size_string[0] = 0;

			} else {
				// AVI����ʤ�
				snprintf(skin_rep_data_line_p[count].avi_vcodec, sizeof(skin_rep_data_line_p[count].avi_vcodec), "[%s]", skin_rep_data_line_p[count].file_extension);
				snprintf(skin_rep_data_line_p[count].avi_hvcodec, sizeof(skin_rep_data_line_p[count].avi_hvcodec), "[%s]", skin_rep_data_line_p[count].file_extension);
			}
		}

		// -------------------------------------
		// MP3 file specification processing
		// -------------------------------------
		if ( (skin_rep_data_line_p[count].menu_file_type == TYPE_MUSIC) &&
			 (strcasecmp(skin_rep_data_line_p[count].file_extension, "mp3") == 0) )
		{
			if (strcmp(http_recv_info_p->option, "mp3info") == 0) {
				char *p;

				skin_rep_data_line_p[count].menu_file_type = TYPE_MP3INFO;
				special_view = 1;
				skin_rep_data_global_p->filename_length_max = global_param.menu_filename_length_max;
				strcpy(work_filename, http_recv_info_p->send_filename);
				if ((p = strstr(skin_rep_data_line_p[count].file_uri_link, ".mp3")) != 0) {
					if (p[4] == '?') {
						strcpy(work_data, basename(skin_rep_data_line_p[count].file_uri_link));
						strcpy(skin_rep_data_line_p[count].file_uri_link, work_data);
					} else
						p[4] = 0;
				}

				strcpy(work_data2, dirname(http_recv_info_p->send_filename));
				strcpy(work_data, basename(file_info_p[i].name));
				p = strrchr(work_data, '.');
				*p = 0;
				set_thumb_file(TYPE_MUSIC,
							   skin_rep_data_line_p[count].file_image,
							   work_data2,
							   work_data,
							   file_info_p[i].ext,
							   "thumb",
							   global_param.menu_icon_type);
				get_tags = 1;
				scan_type = SCAN_QUICK;
			} else {

				// ----------------------------------
				// Full pass formation of MP3 file
				// ----------------------------------
				strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
				if (work_filename[strlen(work_filename)-1] != '/')
				{
					strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
				}
				strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
				debug_log_output("work_filename(mp3) = %s", work_filename);

				(void) find_real_path(file_info_p[i].org_name, http_recv_info_p, work_filename);
				get_tags = global_param.flag_read_mp3_tag;
				scan_type = SCAN_QUICK;
			}

			// set these just in case mp3 has no tags info
			strcpy(skin_rep_data_line_p[count].mp3_id3v1_title, basename(work_filename));
			strcpy(skin_rep_data_line_p[count].mp3_id3v1_title_info, basename(work_filename));
			strcpy(skin_rep_data_line_p[count].mp3_id3v1_title_info_limited, basename(work_filename));

			// ------------------------
			// The ID3V1 data of MP3 GET
			// ------------------------
			if (get_tags)
				mp3_id3_tag_read(work_filename, &(skin_rep_data_line_p[count]), skin_rep_data_global_p->filename_length_max, scan_type);

			skin_rep_data_line_p->mp3_id3v1_flag = get_tags;
			if (!search)
				strcpy(skin_rep_data_line_p[count].info_link, skin_rep_data_global_p->current_path_name);
			strcat(skin_rep_data_line_p[count].info_link, file_info_p[i].org_name);
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
						 "?action=dvdplay&title=%d&page=%d",
						 file_info_p[i].title, file_info_p[i].chap);
			strncpy(work_data, file_info_p[i].name, sizeof(work_data));
		}

# if 0
		if(!special_view && skin_rep_data_global_p->columns != 0) {
            if (skin_rep_data_line_p[count].menu_file_type != TYPE_DIRECTORY)
                skin_rep_data_line_p[count].file_name_no_ext[9] = 0;
            else
                skin_rep_data_line_p[count].file_name[9] = 0;
        }
# else
		if (skin_rep_data_line_p[count].menu_file_type != TYPE_DIRECTORY
		  && skin_rep_data_line_p[count].menu_file_type != TYPE_VIDEO_TS)
			skin_rep_data_line_p[count].file_name_no_ext[skin_rep_data_global_p->filename_length_max] = 0;
		else
			skin_rep_data_line_p[count].file_name[skin_rep_data_global_p->filename_length_max] = 0;
# endif

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

	// Add the secret directory aliases
	for ( i=0; i<global_param.num_aliases; i++) {
		if(global_param.alias_default_file_type[i] == TYPE_SECRET) {
			// HTML formation - TVID and alias name are the same
			// The alias is always relative to the base directory
			snprintf(work_data, sizeof(work_data), "<a href=\"/%s/\" tvid=\"%s\"></a> ", 
				global_param.alias_name[i], global_param.alias_name[i]);

			debug_log_output("alias secret_dir_html='%s'", work_data);
			strncat( skin_rep_data_global_p->secret_dir_link_html, work_data, sizeof(skin_rep_data_global_p->secret_dir_link_html) - strlen(skin_rep_data_global_p->secret_dir_link_html) );
		}
	}
	// Check whether the hiding directory exists.
	for ( i=0; i<SECRET_DIRECTORY_MAX; i++)
	{
		if ( strlen(secret_directory_list[i].dir_name) > 0 )	// �����ǥ��쥯�ȥ����ͭ�ꡩ

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
				if ( S_ISDIR(dir_stat.st_mode) != 0 ) // �ǥ��쥯�ȥ�¸�ߡ�
				{
					// Existing, the cod, URI formation for link
					strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );
					strncat(work_data, secret_directory_list[i].dir_name, sizeof(work_data) - strlen(work_data) );
					uri_encode(work_data2, sizeof(work_data2), work_data, strlen(work_data) );

					// HTML formation
					snprintf(work_data, sizeof(work_data), "<a href=\"%s/\" tvid=%d></a> ", work_data2, secret_directory_list[i].tvid);

					debug_log_output("secret_dir_html='%s'", work_data);
					strncat( skin_rep_data_global_p->secret_dir_link_html, work_data, sizeof(skin_rep_data_global_p->secret_dir_link_html) - strlen(skin_rep_data_global_p->secret_dir_link_html) );

				}
			} else {
				// Assume this is a link off of the base URL
				snprintf(work_data, sizeof(work_data), "/%s/", secret_directory_list[i].dir_name);
				uri_encode(work_data2, sizeof(work_data2), work_data, strlen(work_data) );

				// HTML formation
				snprintf(work_data, sizeof(work_data), "<a href=\"%s\" tvid=%d></a> ", work_data2, secret_directory_list[i].tvid);

				debug_log_output("default location secret_dir_html='%s'", work_data);
				strncat( skin_rep_data_global_p->secret_dir_link_html, work_data, sizeof(skin_rep_data_global_p->secret_dir_link_html) - strlen(skin_rep_data_global_p->secret_dir_link_html) );
			}
		}
		else
		{
			break;
		}
	}
	debug_log_output("secret_dir_html='%s'", skin_rep_data_global_p->secret_dir_link_html);

	send_skin_filemenu(accept_socket, skin_rep_data_global_p, skin_rep_data_line_p, count, special_view, http_recv_info_p, file_info_p, file_num, search);

	free( skin_rep_data_global_p );
	free( skin_rep_data_line_p );
}

void
get_total_time(FILE_INFO_T *file_info_p, int count, char *duration, char *dname)
{
	int i;
	int total;
    FILE  *fp;
    mp3info mp3;
	char buf[256];

	duration[0] = 0;
	total = 0;

	for (i = 0; i < count; i++) {
		sprintf(buf, "%s/%s.mp3", dname, file_info_p[i].name);
		if (!( fp=fopen(buf, "rb+")))
			continue;

		memset(&mp3,0,sizeof(mp3info));
		mp3.file=fp;
		mp3.filename=buf;
		(void) get_mp3_info(&mp3, SCAN_QUICK, 1);
		total += mp3.seconds;

		fclose(fp);
	}

	if (total)
		sprintf(duration, "%d:%02d", total / 60, total % 60);
}

void
send_menu(int accept_socket, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p, HTTP_RECV_INFO *http_recv_info_p, int skip)
{
	SKIN_T *mid_line_skin;
	SKIN_REPLASE_LINE_DATA_T middle_rep[1];
	struct stat dir_stat;
	char work_buf[1024];
	int mid_items;
	DIR	*dir;
	struct dirent *dent;
	int i;
	int real_count;
	char *menu_work_p;

	mid_items = global_param.thumb_row_max * global_param.thumb_column_max;
	mid_line_skin = skin_open(SKIN_MENU_MID_LINE_HTML);

	// output the "middle" rows
	if (!mid_line_skin) {
		return;
	}

	menu_work_p = malloc(MAX_SKIN_FILESIZE);
	if (menu_work_p == NULL) {
		skin_close(mid_line_skin);
		debug_log_output("malloc failed.");
		return ;
	}

	real_count = 0;
	for (i = 0; i < global_param.num_aliases + 1; i++) {
		strcpy(menu_work_p, mid_line_skin->buffer);
		if (i > 1 && strcmp(global_param.alias_name[i - 1], global_param.alias_name[i - 2]) == 0)
			continue;

		if(global_param.alias_default_file_type[i - 1] == TYPE_SECRET)
			continue;

		if (skip != 0) {
			skip--;
			continue;
		}

		if (global_param.flag_default_thumb) {
			if (i == 0) {
				if (http_recv_info_p->menupage == 0)
					snprintf(middle_rep[0].file_uri_link, WIZD_FILENAME_MAX, "/?option=thumb");
				else
					snprintf(middle_rep[0].file_uri_link, WIZD_FILENAME_MAX, "%s?option=thumb&menupage=%d", skin_rep_data_global_p->current_path_name, http_recv_info_p->menupage - 1);
			} else if (real_count == mid_items - 1 && global_param.num_aliases + 1 > mid_items)
				snprintf(middle_rep[0].file_uri_link, WIZD_FILENAME_MAX, "%s?option=thumb&menupage=%d", skin_rep_data_global_p->current_path_name, http_recv_info_p->menupage+1);
			else
				snprintf(middle_rep[0].file_uri_link, WIZD_FILENAME_MAX, "/%s/?option=thumb", global_param.alias_name[i - 1]);
		} else {
			if (i == 0)
				if (http_recv_info_p->menupage == 0)
					snprintf(middle_rep[0].file_uri_link, WIZD_FILENAME_MAX, "/?option=details");
				else
					snprintf(middle_rep[0].file_uri_link, WIZD_FILENAME_MAX, "%s?option=details&menupage=%d", skin_rep_data_global_p->current_path_name, http_recv_info_p->menupage - 1);
			else if (real_count == mid_items - 1)
				snprintf(middle_rep[0].file_uri_link, WIZD_FILENAME_MAX, "%s?option=details&menupage=%d", skin_rep_data_global_p->current_path_name, http_recv_info_p->menupage+1);
			else
				snprintf(middle_rep[0].file_uri_link, WIZD_FILENAME_MAX, "/%s/?option=details", global_param.alias_name[i - 1]);
		}

		if (i == 0)
			if (http_recv_info_p->menupage == 0)
				strcpy(middle_rep[0].file_name, "Home");
			else
				strcpy(middle_rep[0].file_name, "Less...");
		else if (real_count == mid_items - 1 && global_param.num_aliases + 1 > mid_items)
			strcpy(middle_rep[0].file_name, "More...");
		else
			strcpy(middle_rep[0].file_name, global_param.alias_name[i - 1]);
		middle_rep[0].file_name[9] = 0;
		snprintf(middle_rep[0].tvid_string, 16, "%d", real_count + 1);

		replase_skin_line_data(menu_work_p, MAX_SKIN_FILESIZE, &middle_rep[0] );
		replase_skin_grobal_data(menu_work_p, MAX_SKIN_FILESIZE, skin_rep_data_global_p);

		// Each time transmission
		send(accept_socket, menu_work_p, strlen(menu_work_p), 0);

		real_count++;
		if (real_count == mid_items)
			break;
	}

	if (real_count != mid_items) {
		if (real_count == 0 && http_recv_info_p->menupage) {
			snprintf(middle_rep[0].file_uri_link, WIZD_FILENAME_MAX, "%s?menupage=%d", skin_rep_data_global_p->current_path_name, http_recv_info_p->menupage-1);
			strcpy(middle_rep[0].file_name, "Less...");
			snprintf(middle_rep[0].tvid_string, 16, "%d", real_count + 1);
			replase_skin_line_data(menu_work_p, MAX_SKIN_FILESIZE, &middle_rep[0] );
			replase_skin_grobal_data(menu_work_p, MAX_SKIN_FILESIZE, skin_rep_data_global_p);

			// Each time transmission
			send(accept_socket, menu_work_p, strlen(menu_work_p), 0);
			real_count = 1;
		}

		dir = opendir(global_param.document_root);
		if ( dir == NULL )	{
			debug_log_output("opendir() error");
			return;
		}

		while (real_count != mid_items) {
			dent = readdir(dir);
			if ( dent == NULL  )
				break;
			if (dent->d_name[0] == '.')
				continue;

			if (is_secret_dir(dent->d_name))
				continue;

			sprintf(work_buf, "%s/%s", global_param.document_root, dent->d_name);
			if (stat(work_buf, &dir_stat) != 0) {
				continue;
			}

			if (!S_ISDIR(dir_stat.st_mode) != 0)
				continue;

			if (skip != 0) {
				skip--;
				continue;
			}

			strcpy(menu_work_p, mid_line_skin->buffer);

			if (real_count == mid_items - 1)
				snprintf(middle_rep[0].file_uri_link, WIZD_FILENAME_MAX, "%s/?menupage=%d", skin_rep_data_global_p->current_path_name, http_recv_info_p->menupage+1);
			else
				snprintf(middle_rep[0].file_uri_link, WIZD_FILENAME_MAX, "/%s/", dent->d_name);

			if (real_count == mid_items - 1)
				strcpy(middle_rep[0].file_name, "More...");
			else
				strcpy(middle_rep[0].file_name, dent->d_name);
			middle_rep[0].file_name[9] = 0;

			snprintf(middle_rep[0].tvid_string, 16, "%d", real_count + 1);

			replase_skin_line_data(menu_work_p, MAX_SKIN_FILESIZE, &middle_rep[0] );
			replase_skin_grobal_data(menu_work_p, MAX_SKIN_FILESIZE, skin_rep_data_global_p);

			// Each time transmission
			send(accept_socket, menu_work_p, strlen(menu_work_p), 0);

			real_count++;
		}

		i = real_count;
		while (i != mid_items) {
			sprintf(menu_work_p, "<!pad> <tr><td>&nbsp;</td></tr>\n");
			send(accept_socket, menu_work_p, strlen(menu_work_p), 0);
			i++;
		}
	}

	free(menu_work_p);

	skin_close(mid_line_skin);
}

// ==================================================
//  It reads the skin, substitutes and transmits
// ==================================================
static void send_skin_filemenu(int accept_socket, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p, int lines, int special_view, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, int search)
{
	SKIN_MAPPING_T *sm_ptr;
	SKIN_T *header_skin;
	SKIN_T *mid_skin;
	SKIN_T *nav_skin;
	SKIN_T *nav_line_skin;
	SKIN_T *line_skin[MAX_TYPES];
	SKIN_T *tail_skin;
	SKIN_REPLASE_LINE_DATA_T nav_rep[1];
	int i;
	int count;
	unsigned char *menu_work_p;
	char work_buf[1024];
	int	skip;
	int navpages;

	// HTTP_OK header transmission
	http_send_ok_header(accept_socket, 0, NULL);

	if (http_recv_info_p->alias[0] == '\0') {
		// if this the open page see if we should use start.html
		// rather than the normal skin page
		if (strcmp(skin_rep_data_global_p->current_path_name, "/") == 0 &&
							global_param.flag_use_index &&
							strcmp(http_recv_info_p->action, "delete") != 0 &&
							(mid_skin = skin_open("start.html")) != 0) {
			// I'm lazy so just reuse the mid skin variables
			skin_direct_replace_global(mid_skin, skin_rep_data_global_p);
			skin_direct_send(accept_socket, mid_skin);
			skin_close(mid_skin);
			return;
		}
	}

	skip = http_recv_info_p->menupage * (skin_rep_data_global_p->items_per_page - 1);

	// ===============================
	// HEAD skin file reading & substitution & transmission
	// ===============================
	if(skin_rep_data_global_p->delete_mode)
		header_skin = skin_open(SKIN_DELETE_HEAD_HTML);
	else if(!special_view && skin_rep_data_global_p->columns)
		header_skin = skin_open(SKIN_MENU_THUMB_HEAD_HTML);
	else
		header_skin = skin_open(SKIN_MENU_HEAD_HTML);
	if(header_skin == NULL)
		return ;

	// Substituting the data inside SKIN directly
	skin_direct_replace_global(header_skin, skin_rep_data_global_p);
	skin_direct_send(accept_socket, header_skin);
	skin_close(header_skin);

	menu_work_p = malloc(MAX_SKIN_FILESIZE);
	if (menu_work_p == NULL) {
		debug_log_output("malloc failed.");
		return ;
	}

	// ===============================
	// NAVIGATION and MIDDLE skin file reading & substitution & transmission
	// ===============================
	if(!skin_rep_data_global_p->delete_mode) {
		// open navigation skins
		nav_skin = skin_open(SKIN_MENU_NAVHEAD_HTML);
		if (nav_skin != NULL) {
			skin_direct_replace_global(nav_skin, skin_rep_data_global_p);
			skin_direct_send(accept_socket, nav_skin);
			skin_close(nav_skin);

			nav_line_skin = skin_open(SKIN_MENU_NAV_LINE_HTML);

			if (nav_line_skin) {
				int max_page, now_page;

				max_page = skin_rep_data_global_p->max_page;
				now_page = skin_rep_data_global_p->now_page;

				// the number of navigation links at top of page
				navpages = 8;

				for (i = 8; ; i += 6) {
					if (now_page < i) {
						now_page = i - 7;
						break;
					}
				}

				if (max_page - now_page + 1 < navpages) {
					// we have < navpages pages, so add blanks at beginning
					for (i = 0; i < navpages - max_page; i++) {
						strcpy(menu_work_p, nav_line_skin->buffer);

						//strcpy(nav_rep[0].file_name, "&nbsp;&nbsp;"); 
						nav_rep[0].file_name[0] = 0;

						replase_skin_line_data(menu_work_p, MAX_SKIN_FILESIZE, &nav_rep[0] );
						replase_skin_grobal_data(menu_work_p, MAX_SKIN_FILESIZE, skin_rep_data_global_p);

						// Each time transmission
						send(accept_socket, menu_work_p, strlen(menu_work_p), 0);
					}
				}

				for (i = 0; i < navpages; i++) {
					strcpy(menu_work_p, nav_line_skin->buffer);

					snprintf(nav_rep[0].tvid_string, WIZD_FILENAME_MAX, "%d", now_page + i);
					if (!search && strcmp(file_info_p[0].name, "VIDEO_TS") != 0)
						snprintf(nav_rep[0].file_name, WIZD_FILENAME_MAX, "%c%c", file_info_p[((now_page - 1 +i) * skin_rep_data_global_p->items_per_page)].name[0], file_info_p[((now_page - 1 + i) * skin_rep_data_global_p->items_per_page)].name[1]);
					else
						snprintf(nav_rep[0].file_name, WIZD_FILENAME_MAX, "%d", now_page + i);
					if (now_page + i == skin_rep_data_global_p->now_page)
						nav_rep[0].is_current_page = 1;
					else
						nav_rep[0].is_current_page = 0;

					replase_skin_line_data(menu_work_p, MAX_SKIN_FILESIZE, &nav_rep[0] );
					replase_skin_grobal_data(menu_work_p, MAX_SKIN_FILESIZE, skin_rep_data_global_p);

					// Each time transmission
					send(accept_socket, menu_work_p, strlen(menu_work_p), 0);
					if (now_page + i >= max_page)
						break;
				}

				skin_close(nav_line_skin);
			}

			nav_skin = skin_open(SKIN_MENU_NAVTAIL_HTML);
			if (nav_skin != NULL) {
				skin_direct_replace_global(nav_skin, skin_rep_data_global_p);
				skin_direct_send(accept_socket, nav_skin);
				skin_close(nav_skin);
			}
		}

		// open the middle head
		if(skin_rep_data_global_p->columns)
			mid_skin = skin_open(SKIN_MENU_THUMB_MIDHEAD_HTML);
		else 
			mid_skin = skin_open(SKIN_MENU_MIDHEAD_HTML);

		if (mid_skin != NULL) {

			// Substituting the data inside SKIN directly
			skin_direct_replace_global(mid_skin, skin_rep_data_global_p);
			skin_direct_send(accept_socket, mid_skin);

			send_menu(accept_socket, skin_rep_data_global_p, http_recv_info_p, skip);

			// send the middle tail
			if(skin_rep_data_global_p->columns)
				mid_skin = skin_open(SKIN_MENU_THUMB_MIDTAIL_HTML);
			else 
				mid_skin = skin_open(SKIN_MENU_MIDTAIL_HTML);

			if(mid_skin == NULL)
				return ;

			// Substituting the data inside SKIN directly
			skin_direct_replace_global(mid_skin, skin_rep_data_global_p);
			skin_direct_send(accept_socket, mid_skin);
			skin_close(mid_skin);
		}
	}

	// ===============================
	// The skin file reading & substitution & transmission for LINE
	// ===============================
	for (i=0; i<MAX_TYPES; i++) line_skin[i] = NULL;
	for (sm_ptr = skin_mapping; sm_ptr->skin_filename != NULL; sm_ptr++) {
		if (sm_ptr->filetype >= MAX_TYPES) {
			debug_log_output("CRITICAL: check MAX_TYPES or skin_mapping...");
			continue;
		}
		if(!special_view && skin_rep_data_global_p->columns)
			snprintf(work_buf, sizeof(work_buf), "thumb_%s.html", sm_ptr->skin_filename);
		else
			snprintf(work_buf, sizeof(work_buf), "line_%s.html", sm_ptr->skin_filename);
		line_skin[sm_ptr->filetype] = skin_open(work_buf);
		if (line_skin[sm_ptr->filetype] == NULL) {
			debug_log_output("'%s' is not found?", work_buf);
		}
	}
	if (line_skin[TYPE_UNKNOWN] == NULL) {
		debug_log_output("FATAL: cannot find TYPE_UNKNOWN skin definition.");
		return ;
	}

	// Start of LINE substitution processing.
	for (count=0; count < lines; count++) {
		int mtype;

		if (special_view == 4) {
			// add a picture of the album cover first - kludge!
			count = -1;
			special_view = 1;
			nav_line_skin = skin_open(SKIN_MENU_ALBUM_HTML);
			// assume all tracks have the same art
			set_thumb_file(TYPE_MUSIC,
						   nav_rep[0].file_image,
						   http_recv_info_p->send_filename,
						   file_info_p[0].name,
						   file_info_p[0].ext,
						   "thumb",
						   global_param.menu_icon_type);
			set_html_file(TYPE_MUSIC,
						   nav_rep[0].html_link,
						   http_recv_info_p->send_filename, 
						   file_info_p[i].name,
						   file_info_p[i].ext,
						   "thumb",
						   global_param.menu_icon_type);
			sprintf(nav_rep[0].tvid_string, "%d", file_num);
			get_total_time(file_info_p, file_num, nav_rep[0].file_duration, http_recv_info_p->send_filename);
			strcpy(menu_work_p, nav_line_skin->buffer);
			replase_skin_line_data(menu_work_p, MAX_SKIN_FILESIZE, &nav_rep[0] );
			replase_skin_grobal_data(menu_work_p, MAX_SKIN_FILESIZE, skin_rep_data_global_p);
			send(accept_socket, menu_work_p, strlen(menu_work_p), 0);
			skin_close(nav_line_skin);
			continue;
		}

		mtype = skin_rep_data_line_p[count].menu_file_type;
		strncpy(menu_work_p, skin_get_string(line_skin[line_skin[mtype] != NULL ? mtype : TYPE_UNKNOWN]), MAX_SKIN_FILESIZE);
		replase_skin_line_data(menu_work_p, MAX_SKIN_FILESIZE, &skin_rep_data_line_p[count] );
		replase_skin_grobal_data(menu_work_p, MAX_SKIN_FILESIZE, skin_rep_data_global_p);

		// Each time transmission
		send(accept_socket, menu_work_p, strlen(menu_work_p), 0);

		if(	(!special_view && skin_rep_data_global_p->columns>0)
			 && (count != (lines-1))
			 && ((count % skin_rep_data_global_p->columns) == (skin_rep_data_global_p->columns-1)) ) {
			// Send the thumbnail row break skin
			mtype = TYPE_ROW;
			strncpy(menu_work_p, skin_get_string(line_skin[line_skin[mtype] != NULL ? mtype : TYPE_UNKNOWN]), MAX_SKIN_FILESIZE);
			replase_skin_line_data(menu_work_p, MAX_SKIN_FILESIZE, &skin_rep_data_line_p[count] );
			replase_skin_grobal_data(menu_work_p, MAX_SKIN_FILESIZE, skin_rep_data_global_p);
			// Each time transmission
			send(accept_socket, menu_work_p, strlen(menu_work_p), 0);
		}
	}

	i = count % skin_rep_data_global_p->items_per_page;
	if (i != 0) {
		// pad out the rest of the page
		i = skin_rep_data_global_p->items_per_page - i;
		for (; i > 0; i--) {
			sprintf(menu_work_p, "<!pad> <tr><td>&nbsp;</td></tr>\n");
			send(accept_socket, menu_work_p, strlen(menu_work_p), 0);
		}
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
	if(skin_rep_data_global_p->delete_mode)
		tail_skin = skin_open(SKIN_DELETE_TAIL_HTML);
	else if(!special_view && skin_rep_data_global_p->columns)
		tail_skin = skin_open(SKIN_MENU_THUMB_TAIL_HTML);
	else 
		tail_skin = skin_open(SKIN_MENU_TAIL_HTML);
	if(tail_skin == NULL)
		return ;

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

static int count_files_in_directory(unsigned char *path, int recurse)
{
	struct stat		file_stat;
	int				count;

	DIR	*dir;
	struct dirent	*dent;
	char			 workbuf[2056];

	dir = opendir(path);
	if ( dir == NULL )	// ���顼�����å�
	{
		debug_log_output("opendir() error");
		return (0);
	}

	count = 0;
	while ( 1 )
	{
		dent = readdir(dir);
		if ( dent == NULL  )
			break;

		// ̵��ե���������å���
		if (file_ignoral_check(dent->d_name, path) != 0) {
			continue;
		}

		if (recurse) {
			if (strcasecmp(dent->d_name, "video_ts") == 0)
				// don't count dvd directory files
				continue;

			sprintf(workbuf, "%s/%s", path, dent->d_name);
			if (stat(workbuf, &file_stat) < 0) {
				continue;
			}

			if (S_ISDIR( file_stat.st_mode ) != 0)
				count += count_files_in_directory(workbuf, recurse);
		}

		count++;
	}

	closedir(dir);
	return count;
}

static int count_file_num(unsigned char *path)
{
	int i, j, len, count=0;
	debug_log_output("count_file_num() start. path='%s'", path);

	if(global_param.num_aliases) {
		if(strcasecmp(path, global_param.document_root) == 0) {
			debug_log_output("Adding %d aliases to root directory\n", global_param.num_aliases);
			count += global_param.num_real_aliases;
		}
	}
	
	// If this matches an alias, check for duplicate alias names
	// and include the file contents of all of the aliased directories with the same alias name
	len=strlen(path);
	if(len > 0) {
		len--; // Ignore the trailing slash
		for(i=0; (i<global_param.num_aliases) && (strncasecmp(path, global_param.alias_path[i], len) != 0); i++);

		if(i < global_param.num_aliases) {
			// If this matches an alias, check for duplicate alias names
			// and include the file contents of all of the aliased directories with the same alias name
			for(j=0; j<global_param.num_aliases; j++) {
				if(strcasecmp(global_param.alias_name[i],global_param.alias_name[j])==0) {
					debug_log_output("Including alias path '%s'\n", global_param.alias_path[j]);
					count += count_files_in_directory(global_param.alias_path[j], 0);
				}
			}
		} else {
			// Not an alias
			count += count_files_in_directory(path, 0);
		}
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
// Equal to the number of files reads the information which exists in the directory.
//
// Return: The quantity of file information which it reads
// **************************************************************************
static JOINT_FILE_INFO_T joint_file_info;

static int next_directory_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num, regex_t *re, int g_index, int recurse)
{
	int	count;
	DIR	*dir;
	struct dirent	*dent;
	int				result;
	unsigned char	*work_p;
	MIME_LIST_T	*mime;
	unsigned char	 fullpath_filename[WIZD_FILENAME_MAX];
	unsigned char	 dir_name[WIZD_FILENAME_MAX];
	struct stat		 file_stat;

	// printf("\tenter path %s, file_info_p 0x%x, file_num %d, re 0x%x, gindex %d, recurse %d\n", path, file_info_p, file_num, re, g_index, recurse);

	// printf("count 0x%x, dir 0x%x, dent 0x%x, file_stat 0x%x, result 0x%x, work_p 0x%x, mime 0x%x, fullpath_filename 0x%x, dir_name 0x%x\n", (int) &count, (int) &dir, (int) &dent, (int) &file_stat, (int) &result, (int) &work_p, (int) &mime, (int) &fullpath_filename, (int) &dir_name);

	//printf("\tsize count %d, dir %d, dent %d, file_stat %d, result %d, work_p %d, mime %d, fullpath_filename %d, dir_name %d\n", sizeof(count), sizeof(dir), sizeof(dent), sizeof(file_stat), sizeof(result), sizeof(work_p), sizeof(mime), sizeof(fullpath_filename), sizeof(dir_name));

	debug_log_output("next_directory_stat() start. path='%s'", path);

	dir = opendir(path);
	if ( dir == NULL )	// ���顼�����å�
	{
		debug_log_output("opendir() error");
		return (0);
	}

	// ���ȤǺ������Ȥ��Τ���ˡ���ʬ�Υǥ��쥯�ȥ�̾��EUC������
	convert_language_code(	path, dir_name, sizeof(dir_name),
							global_param.server_language_code | CODE_HEX, CODE_EUC );
	// When the '/' has been attached lastly, deletion
	cut_character_at_linetail(dir_name, '/');
	// 'Deleting before from the /'
	cut_before_last_character(dir_name, '/');

	count = 0;

	while ( 1 )
	{
		if ( count >= file_num )
			break;

		// From directory, file name 1 GET
		dent = readdir(dir);
		if ( dent == NULL  )
			break;

		// Disregard file check.
		// printf("path %s, file %s\n", path, dent->d_name);
		if ( file_ignoral_check(dent->d_name, path) != 0 ) {
			debug_log_output("dent->d_name='%s' file_ignoral_check failed ", dent->d_name);
			continue;
		}

		if (recurse && strcasecmp(dent->d_name, "video_ts") == 0) {
			// don't count dvd directory files
			continue;
		}

		//debug_log_output("dent->d_name='%s'", dent->d_name);


		// Full pass file name formation
		strcpy(fullpath_filename, path);
		if(path[strlen(path)-1] != '/')
			strcat(fullpath_filename, "/");
		strcpy(file_info_p[count].full_pathname, fullpath_filename);
		strcat(fullpath_filename, dent->d_name);

		//debug_log_output("fullpath_filename='%s'", fullpath_filename );

		// stat() �¹�
		memset(&file_stat, 0, sizeof(file_stat));
		result = stat(fullpath_filename, &file_stat);
		if ( result < 0 ) {
			debug_log_output("fullpath_filename='%s' stat failed ", fullpath_filename);
			continue;
		}

		if (S_ISDIR(file_stat.st_mode) && recurse) {
			// printf("recursing into %s, count %d\n", fullpath_filename, count);
			count += next_directory_stat(fullpath_filename, &file_info_p[count], file_num - count, re, g_index, 1);
			// printf("return from %s, count %d\n", fullpath_filename, count);
			//continue;

			strcpy(fullpath_filename, path);
			if(path[strlen(path)-1] != '/')
				strcat(fullpath_filename, "/");
			strcpy(file_info_p[count].full_pathname, fullpath_filename);
			// printf("checking dir %s %s\n", fullpath_filename, dent->d_name);
		}

		// printf("looking at file %s\n", fullpath_filename);
		if (re && regexec(re, dent->d_name, 0, NULL, 0) != 0)
			continue;
		// printf("found file %s\n", fullpath_filename);

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

		if (re) {
			char	work_buf[256];

			sprintf(work_buf, "/%s/", global_param.alias_name[g_index]);
			strcat(work_buf, &path[strlen(global_param.alias_path[g_index])]);
			strcat(work_buf, "/");
			strcat(work_buf, file_info_p[count].org_name);
			strcpy(file_info_p[count].org_name, work_buf);
			// printf("org_name is %s\n", work_buf);
		}

		// EUC���Ѵ���
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
				// ���λ����� file_info_p[count].name �� ��ĥ�ҥʥ��ˤʤ롣
				*work_p++ = '\0';
				strncpy(file_info_p[count].ext, work_p, sizeof(file_info_p[count].ext));
				debug_log_output("ext = '%s'", file_info_p[count].ext);
			}

	checkmime:
			if ((mime = lookup_mime_by_ext(file_info_p[count].ext)) == NULL) {
# ifdef HAVE_W32API
				if (strcmp(file_info_p[count].ext, "lnk") == 0) {
					char	retPath1[256];
					char	retPath2[256];

					cygwin_conv_to_full_win32_path(fullpath_filename, retPath1);
					if (get_target(retPath1, retPath2) == 0) {
						cygwin_conv_to_full_posix_path(retPath2, retPath1);
						result = stat(retPath1, &file_stat);
						if (result < 0)
							file_info_p[count].type = TYPE_UNKNOWN;
						else if (S_ISDIR(file_stat.st_mode) == 1) {
							file_info_p[count].ext[0] = '\0';
							file_info_p[count].type = TYPE_DIRECTORY;
							get_uri_path(retPath2);
							cygwin_conv_to_full_posix_path(retPath2, file_info_p[count].org_name);
							strcat(file_info_p[count].org_name, "/");
						} else {
							if ((work_p = strrchr(retPath1, '.')) != NULL) {
								work_p++;
								strcpy(file_info_p[count].ext, work_p);
								get_uri_path(retPath2);
								cygwin_conv_to_full_posix_path(retPath2, file_info_p[count].org_name);
								goto checkmime;
							} else
								file_info_p[count].type = TYPE_UNKNOWN;
						}
					} else
						file_info_p[count].type = TYPE_UNKNOWN;
				} else
# endif
					file_info_p[count].type = TYPE_UNKNOWN;
			} else
				file_info_p[count].type = mime->menu_file_type;
		} else {
			// If the directory there is no extension
			file_info_p[count].ext[0] = '\0';
			file_info_p[count].type = TYPE_DIRECTORY;
		}

# if 0
		if (re) {
			strcat(file_info_p[count].name, " (");
			strcat(file_info_p[count].name, global_param.alias_name[g_index]);
			strcat(file_info_p[count].name, ")");
		}
# endif

		// Limit the minimum file size for JPEG photos
		if(   (    ( strcasecmp(file_info_p[count].ext, "jpg" ) ==  0 )
			|| ( strcasecmp(file_info_p[count].ext, "jpeg") ==  0 ) )
		    && (file_stat.st_size < global_param.minimum_jpeg_size) )
			continue; 

		// In addition retaining information
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
				if (analyze_vob_file(fullpath_filename, &joint_file_info ) == 0) {
					file_info_p[count].size = joint_file_info.total_size;
				}
			} else
				continue;
		}

		// printf("found file path %s, name %s\n", file_info_p[count].full_pathname, file_info_p[count].name);

		count++;
	}

	closedir(dir);

	debug_log_output("next_directory_stat() end. count=%d", count);

	return count;
}

static int directory_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num)
{
	int	i,j,len,count=0;

	debug_log_output("directory_stat() start. path='%s'", path);

	// If this is the root directory, then add in any aliases defined in wizd.conf
	// Insert these first, so they appear at the top in an unsorted list
	if(global_param.num_aliases && (strcasecmp(path, global_param.document_root) == 0)) {
		for(i=0; (i < global_param.num_aliases) && (count < file_num); i++) {
			if(global_param.alias_default_file_type[i] != TYPE_SECRET) {
				// Omit duplicates
				for(j=0; (j<i) && (strcmp(global_param.alias_name[i],global_param.alias_name[j])!=0); j++);
				if(j==i) {
					debug_log_output("Inserting alias '%s'\n", global_param.alias_name[i]);
					strncpy(file_info_p[count].name, global_param.alias_name[i], sizeof(file_info_p[count].name));
					snprintf(file_info_p[count].org_name, sizeof(file_info_p[count].org_name), "%s/", global_param.alias_name[i]);
					file_info_p[count].ext[0] = '\0';
					file_info_p[count].type = TYPE_DIRECTORY;
					// It may be useful to define skin lines for alias types, different from directories
					// This will require several other changes for checks for TYPE_DIRECTORY
					// file_info_p[count].type = global_param.alias_default_file_type[i];
					file_info_p[count].size = 0;
					file_info_p[count].time = 0;
					count++;
				}
			}
		}
	}

	// If this matches an alias, check for duplicate alias names
	// and include the file contents of all of the aliased directories with the same alias name
	len=strlen(path);
	if(len > 0) {
		len--; // Ignore the trailing slash
		for(i=0; (i<global_param.num_aliases) && (strncasecmp(path, global_param.alias_path[i], len) != 0); i++);

		if(i < global_param.num_aliases) {
			// If this matches an alias, check for duplicate alias names
			// and include the file contents of all of the aliased directories with the same alias name
			for(j=0; j<global_param.num_aliases; j++) {
				if(strcasecmp(global_param.alias_name[i],global_param.alias_name[j])==0) {
					debug_log_output("Including alias path '%s'\n", global_param.alias_path[j]);
					count += next_directory_stat(global_param.alias_path[j], file_info_p+count, file_num-count, 0, 0, 0);
				}
			}
		} else {
			// Not an alias
			count += next_directory_stat(path, file_info_p+count, file_num-count, 0, 0, 0);
		}
	}
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

		// �ե����뤫�顢�����ɤ�
		ret = file_read_line( fd, buf, sizeof(buf) );
		if ( ret < 0 )
		{
			debug_log_output("tsv EOF Detect.");
			break;
		}

		// �����Υ��ڡ���������
		cut_whitespace_at_linetail(buf);

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

		// stat() �¹�
		memset(&file_stat, 0, sizeof(file_stat));
		result = stat(fullpath_filename, &file_stat);
		if ( result >= 0 ) {
			// Substantial discovery
			file_info_p[count].size = file_stat.st_size;
			file_info_p[count].time = file_stat.st_mtime;

			// When the directory being understood, deleting the extension 
			if (S_ISDIR(file_stat.st_mode)) {
				file_info_p[count].ext[0] = '\0';
				file_info_p[count].type = TYPE_DIRECTORY;
			} else {
				file_info_p[count].type = TYPE_SVI; // Not a directory
			}
		} else {
		// There is no substance. Depending, type obscurity of date and size and the file 


			file_info_p[count].type = TYPE_UNKNOWN;
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
	int				len;

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

	len = strlen(name);

	if (len > 7 && strncasecmp(name, "tn_", 3) == 0) {
		if (strncasecmp(&name[len - 4], ".jpg", 4) == 0) {
			return(-1);
		} else if (strncasecmp(&name[len - 4], ".gif", 4) == 0) {
			return(-1);
		} else if (strncasecmp(&name[len - 4], ".png", 4) == 0) {
			return(-1);
		}
	}
# ifdef HAVE_W32API
	else if (strncasecmp(&name[len - 4], ".lnk", 4) == 0) {
		// XXX for now assume it is a link to a valid file
		return(0);
	}
# endif
	else if (strncasecmp(name, "wizd_", 5) == 0) {
		return(-1);
	} else if (len > 8 && strncmp(name, "AlbumArt", 8) == 0) {
		return(-1);
	} else if (len > 7 && strncmp(name, "Folder.", 7) == 0) {
		return(-1);
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

		debug_log_output("file_ignoral_check: filename='%s', file_extension='%s'\n", name, file_extension);
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
			// Check whether file, truly file.
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
	SKIN_REPLASE_LINE_DATA_T	mp3_id3tag_data;	// MP3 ID3�����ɤ߹�����
	int input_code;
	int type;

	// ---------------------------------------------------------
	// �ե����륿����Ƚ�ꡣ�оݳ��ե�����ʤ�������ʤ�
	// ---------------------------------------------------------
	if (file_info_p == NULL) {
		// single
		MIME_LIST_T *mime;
		strncpy( disp_filename, http_recv_info_p->recv_uri, sizeof(disp_filename) );
		cut_before_last_character(disp_filename, '/');
		filename_to_extension(http_recv_info_p->send_filename, file_ext, sizeof(file_ext));
		input_code = global_param.server_language_code;
		if ((mime = lookup_mime_by_ext(file_ext)) == NULL) {
			type = TYPE_UNKNOWN;
		} else {
			type = mime->menu_file_type;
		}
	} else {
		// menu
		strncpy(disp_filename, file_info_p->name, sizeof(disp_filename));
		strncpy(file_ext, file_info_p->ext, sizeof(file_ext));
		input_code = CODE_EUC; // file_info_p->name �� ���EUC (wizd 0.12h)
		type = file_info_p->type;
	}
	debug_log_output("file_extension='%s'\n", file_ext);

	// ------------------------------------------------------
	// If outside the playback object it returns
	// ------------------------------------------------------

	// Restrict the file types included in the playlist
	switch( http_recv_info_p->default_file_type ) {
	default:
		if ( (type != TYPE_MOVIE)
		 &&  (type != TYPE_MUSIC)
		 &&  (type != TYPE_JPEG)
		 &&  (type != TYPE_SVI) ) {
			return NULL;
		}
		break;
	case TYPE_MOVIE:
		if ( (type != TYPE_MOVIE)
		 &&  (type != TYPE_SVI) ) {
			return NULL;
		}
		break;
	case TYPE_MUSIC:
	case TYPE_MUSICLIST:
		if ( type != TYPE_MUSIC ) {
			return NULL;
		}
		break;
	case TYPE_JPEG:
	case TYPE_PLAYLIST:
		if ( type != TYPE_JPEG ) {
			return NULL;
		}
		break;
	}

	// -----------------------------------------
	// If extension mp3, ID3 tag check.
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
			strncat(work_data, path, sizeof(work_data) - strlen(work_data) );
			strncat(work_data, file_info_p->org_name, sizeof(work_data) - strlen(work_data));
		}
		debug_log_output("work_data(mp3) = %s", work_data);

		// ID3���������å�
		memset( &mp3_id3tag_data, 0, sizeof(mp3_id3tag_data));
		mp3_id3_tag_read(work_data , &mp3_id3tag_data, global_param.menu_filename_length_max, SCAN_NONE);
	}


	// MP3 ID3������¸�ߤ����ʤ�С�playlist ɽ���ե�����̾��ID3�������֤������롣
	if ( mp3_id3tag_data.mp3_id3v1_title_info[0] || mp3_id3tag_data.mp3_id3v1_artist[0])
	{
		strncpy(work_data, mp3_id3tag_data.mp3_id3v1_title_info, sizeof(work_data));
		strcat(work_data, "/");
		strcat(work_data, mp3_id3tag_data.mp3_id3v1_artist);
		strncat(work_data, ".mp3", sizeof(work_data) - strlen(work_data));	// ���ߡ��γ�ĥ�ҡ�playlist_filename_adjustment()�Ǻ������롣

		// =========================================
		// playlistɽ���� ID3������Ĵ��
		// If in EUC conversion -> extension deletion -> (necessity if) half angle letter //in full size conversion -> in the MediaWiz cord/code conversion -> SJIS, illegitimate //character code 0x7C (the '|') deleting the letter which is included.
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

# if 0
	// When generating default music playlists, just return the filename
	if (http_recv_info_p->default_file_type == TYPE_MUSICLIST) {
		strncat(work_data, "\r\n", sizeof(work_data)-strlen(work_data));
	}
# endif

	uri_encode(file_uri_link, sizeof(file_uri_link), work_data, strlen(work_data) );

	if (type == TYPE_JPEG) {
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
static void create_all_play_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, int max_items)
{
	int i,count=0,dir_count=0;
	char work_buf[1024];
	char **dir_list=NULL;
	int max_dir_items=0;

	if(global_param.flag_allplay_includes_subdir) {
		// Allocate an array of pointers for directory names
		// Assume a minimum of 10 files per directory
		max_dir_items = global_param.max_play_list_search;
		if(max_dir_items < max_items)
			max_dir_items = max_items;
		max_dir_items /= 10;
		// But allow for scanning of at least 100 directories
		if(max_dir_items < 100) 
			max_dir_items = 100;
		dir_list = (char **)malloc(max_dir_items * sizeof(char *));
		if(dir_list == NULL)
			max_dir_items=0;
	}

	debug_log_output("create_all_play_list() start.");

	// =================================
	// �ե�����ɽ���Ѿ��� ����������
	// =================================
	for ( i=0; (i<file_num) && (count < max_items) ; i++ )
	{
		// �ǥ��쥯�ȥ��̵��
		if ( file_info_p[i].type == TYPE_DIRECTORY ) {
			if(dir_count < max_dir_items) {
				// Accumulate a list of directories
				snprintf(work_buf, sizeof(work_buf), "%s/", file_info_p[i].name);
				dir_list[dir_count++] = strdup(work_buf);
				dir_count = recurse_directory_list(http_recv_info_p, dir_list, dir_count, max_dir_items, 0);
				//count = recurse_all_play_list(accept_socket, http_recv_info_p, work_buf, count, max_items, 0);
			}
		} else {
			char *ptr = create_1line_playlist(http_recv_info_p, "", &file_info_p[i], 0);
			if (ptr != NULL) {
				// 1�ԤŤġ�����������
				write(accept_socket, ptr, strlen(ptr));
				count++;
			}
		}
	}

	if(dir_list != NULL) {
		// Randomly shuffle all of the directories, and then play each one straight through
		srand(time(NULL));
		while((dir_count > 0) && (count < max_items)) {
			i = (int)floor((double)rand() / (double)RAND_MAX * (double)dir_count);
			if(i<dir_count) {
				count = recurse_all_play_list(accept_socket, http_recv_info_p, dir_list[i], count, max_items);
				free(dir_list[i]);
				dir_list[i] = dir_list[--dir_count];
			}
		}
		// Free up any leftover items
		for(i=0; i<dir_count; i++)
			free(dir_list[i]);
		free(dir_list);
	}
	debug_log_output("create_all_play_list() found %d items", count);

	return;
}

static int recurse_directory_list(HTTP_RECV_INFO *http_recv_info_p, char **list, int dir_count, int max_dir_items, int depth)
{
	FILE_INFO_T *file_info_p;
	char work_buf[1024];
	int i, file_num;
	char *directory = list[dir_count-1];

	if(depth >= MAX_PLAY_LIST_DEPTH) {
		debug_log_output("recurse_directory_list: directory '%s', depth %d - Max depth reached - returning\n", directory, depth);
		return dir_count;
	}
	if(dir_count >= max_dir_items) {
		debug_log_output("recurse_directory_list: directory '%s', count %d - Max count reached - returning\n", directory, dir_count);
		return dir_count;
	}

	// recv_uri �ǥ��쥯�ȥ�Υե������������롣
	strncpy(work_buf, http_recv_info_p->send_filename, sizeof(work_buf));
	strncat(work_buf, directory, sizeof(work_buf)-strlen(work_buf));
	file_num = count_file_num( work_buf );

	debug_log_output("directory '%s', depth %d, file_num = %d\n", directory, depth, file_num);
	if ( file_num <= 0 )
		return dir_count;

	// ɬ�פʿ��������ե����������¸���ꥢ��malloc()
	file_info_p = (FILE_INFO_T *)malloc( sizeof(FILE_INFO_T)*file_num );
	if ( file_info_p == NULL )
	{
		debug_log_output("malloc() error");
		return dir_count;
	}

	memset(file_info_p, 0, sizeof(FILE_INFO_T)*file_num);

	file_num = directory_stat(work_buf, file_info_p, file_num);
	debug_log_output("directory_stat(%s) returned %d items\n", directory, file_num);
	for ( i=0; (i<file_num) && (dir_count < max_dir_items) ; i++ )
	{
		// Just pick off the directories
		if ( file_info_p[i].type == TYPE_DIRECTORY ) {
			// Do a depth-first scan of the directories
			snprintf(work_buf, sizeof(work_buf), "%s%s/", directory, file_info_p[i].name);
			list[dir_count++] = strdup(work_buf);
			dir_count = recurse_directory_list(http_recv_info_p, list, dir_count, max_dir_items, depth+1);
			//count = recurse_all_play_list(accept_socket, http_recv_info_p, work_buf, count, max_items, depth+1);
		}
	}
	free(file_info_p);
	return dir_count;
}

static int recurse_all_play_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, char *directory, int count, int max_items) //, int depth)
{
	FILE_INFO_T *file_info_p;
	char work_buf[1024];
	int i, file_num;
	char *ptr;

	/*
	if(depth >= MAX_PLAY_LIST_DEPTH) {
		debug_log_output("recurse_all_play_list: directory '%s', depth %d - Max depth reached - returning\n", directory, depth);
		return count;
	}
	*/
	if(count >= max_items) {
		debug_log_output("recurse_all_play_list: directory '%s', count %d - Max count reached - returning\n", directory, count);
		return count;
	}

	// recv_uri �ǥ��쥯�ȥ�Υե������������롣
	strncpy(work_buf, http_recv_info_p->send_filename, sizeof(work_buf));
	strncat(work_buf, directory, sizeof(work_buf)-strlen(work_buf));
	file_num = count_file_num( work_buf );

	//debug_log_output("directory '%s', depth %d, file_num = %d\n", directory, depth, file_num);
	debug_log_output("directory '%s', file_num = %d\n", directory, file_num);
	if ( file_num <= 0 )
		return count;

	// ɬ�פʿ��������ե����������¸���ꥢ��malloc()
	file_info_p = (FILE_INFO_T *)malloc( sizeof(FILE_INFO_T)*file_num );
	if ( file_info_p == NULL )
	{
		debug_log_output("malloc() error");
		return count;
	}

	memset(file_info_p, 0, sizeof(FILE_INFO_T)*file_num);

	file_num = directory_stat(work_buf, file_info_p, file_num);
	debug_log_output("directory_stat(%s) returned %d items\n", directory, file_num);
	for ( i=0; (i<file_num) && (count < max_items) ; i++ )
	{

		/* printf("got file %s\n", file_info_p[i].name); */
		// �ǥ��쥯�ȥ��̵��
		if ( file_info_p[i].type == TYPE_DIRECTORY ) {
			// Do a depth-first scan of the directories
			//snprintf(work_buf, sizeof(work_buf), "%s%s/", directory, file_info_p[i].name);
			//count = recurse_all_play_list(accept_socket, http_recv_info_p, work_buf, count, max_items, depth+1);
		} else {
			ptr = create_1line_playlist(http_recv_info_p, directory, &file_info_p[i], 0);
			if (ptr != NULL) {
				// 1�ԤŤġ�����������
				write(accept_socket, ptr, strlen(ptr));
				count++;
			}
		}
	}
	free(file_info_p);
	return count;
}


static void create_shuffle_list(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, int max_items)
{
	int i,sent,count=0,dir_count=0;
	char work_buf[1024];
	char **dir_list=NULL;
	int max_dir_items=0;
	char **list;

	// Allocate the max playlist search size so randomized list can still be a subset
	// of all of the available files.  
	// If we don't do this, then if our randomized playlist is limited to only the first N files found
	// so some files will never make the list!
	int alloc=global_param.max_play_list_search;
	if(alloc < max_items)
		alloc = max_items;
	list = (char **)malloc(alloc*sizeof(char *));

	srand(time(NULL));

	if(global_param.flag_allplay_includes_subdir) {
		// Allocate an array of pointers for directory names
		// Assume a minimum of 10 files per directory
		max_dir_items = alloc/10;
		// But allow for scanning at least 100 directories
		if(max_dir_items < 100) 
			max_dir_items = 100;
		dir_list = (char **)malloc(max_dir_items * sizeof(char *));
		if(dir_list == NULL)
			max_dir_items=0;
	}

	debug_log_output("create_shuffle_list() start.");

	// =================================
	// Information formation & transmission for file indication 
	// =================================
	for ( i=0; (i<file_num) && (count < alloc) ; i++ )
	{
		// �ǥ��쥯�ȥ��̵��
		if ( file_info_p[i].type == TYPE_DIRECTORY ) {
			if(dir_count < max_dir_items) {
				// Accumulate a list of directories
				snprintf(work_buf, sizeof(work_buf), "%s/", file_info_p[i].name);
				dir_list[dir_count++] = strdup(work_buf);
				dir_count = recurse_directory_list(http_recv_info_p, dir_list, dir_count, max_dir_items, 0);
				//count = recurse_shuffle_list(http_recv_info_p, work_buf, list, count, max_items, 0);
			}
		} else {
			char *ptr = create_1line_playlist(http_recv_info_p, "", &file_info_p[i], 0);
			if (ptr != NULL) {
				// 1�ԤŤġ�����������
				list[count++] = strdup(ptr);
			}
		}
	}

	if(dir_list != NULL) {
		// Randomly shuffle all of the directories, and then gather the files from each
		// we do this extra bit of randomization so that if our max playlist search is small
		// we don't limit the randomization to the same set of files every time
		while((dir_count > 0) && (count < alloc)) {
			i = (int)floor((double)rand() / (double)RAND_MAX * (double)dir_count);
			if(i<dir_count) {
				count = recurse_shuffle_list(http_recv_info_p, dir_list[i], list, count, alloc);
				free(dir_list[i]);
				dir_list[i] = dir_list[--dir_count];
			}
		}
		if(dir_count > 0) {
			debug_log_output("Shuffle finished with %d directories remaining, and %d/%d items", dir_count, count, alloc);
		}
		// Free up any leftover items
		for(i=0; i<dir_count; i++)
			free(dir_list[i]);
		free(dir_list);
	}
	debug_log_output("create_shuffle_list() found %d items", count);

	// Random shuffle the list
	sent = 0;
	while((count > 0) && (sent<max_items)) {
		i = (int)floor((double)rand() / (double)RAND_MAX * (double)count);
		if(i<count) {
			write(accept_socket, list[i], strlen(list[i]));
			free(list[i]);
			list[i] = list[--count];
			sent++;
		}
	}
	// Free up any leftover unused items
	for(i=0; i<count; i++)
		free(list[i]);

	free(list);

	return;
}

static int recurse_shuffle_list(HTTP_RECV_INFO *http_recv_info_p, char *directory, char **list, int count, int max_items) //, int depth)
{
	FILE_INFO_T *file_info_p;
	char work_buf[WIZD_FILENAME_MAX];
	int i, file_num;
	char *ptr;

	/*
	if(depth >= MAX_PLAY_LIST_DEPTH) {
		debug_log_output("recurse_shuffle_list: directory '%s', depth %d - Max depth reached - returning\n", directory, depth);
		return count;
	}
	*/
	if(count >= max_items) {
		debug_log_output("recurse_shuffle_list: directory '%s', count %d - Max count reached - returning\n", directory, count);
		return count;
	}

	// recv_uri �ǥ��쥯�ȥ�Υե������������롣
	strncpy(work_buf, http_recv_info_p->send_filename, sizeof(work_buf));
	strncat(work_buf, directory, sizeof(work_buf)-strlen(work_buf));
	file_num = count_file_num( work_buf );

	debug_log_output("directory '%s', file_num = %d\n", directory, file_num);
	//debug_log_output("directory '%s', depth %d, file_num = %d\n", directory, depth, file_num);
	if ( file_num <= 0 )
		return count;

	// ɬ�פʿ��������ե����������¸���ꥢ��malloc()
	file_info_p = (FILE_INFO_T *)malloc( sizeof(FILE_INFO_T)*file_num );
	if ( file_info_p == NULL )
	{
		debug_log_output("malloc() error");
		return count;
	}

	memset(file_info_p, 0, sizeof(FILE_INFO_T)*file_num);

	file_num = directory_stat(work_buf, file_info_p, file_num);
	debug_log_output("directory_stat(%s) returned %d items\n", directory, file_num);
	for ( i=0; (i<file_num) && (count < max_items) ; i++ )
	{
		// As for directory disregard 
		if ( file_info_p[i].type == TYPE_DIRECTORY ) {
			// Do a depth-first scan of the directories
			//snprintf(work_buf, sizeof(work_buf), "%s%s/", directory, file_info_p[i].name);
			//count = recurse_shuffle_list(http_recv_info_p, work_buf, list, count, max_items, depth+1);
		} else {
			ptr = create_1line_playlist(http_recv_info_p, directory, &file_info_p[i], 0);
			if (ptr != NULL) {
				// 1�ԤŤġ�����������
				list[count++] = strdup(ptr);
				debug_log_output("shuffle item %d: '%s'\n", count, ptr);
			}
		}
	}
	free(file_info_p);
	return count;
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
		if ( p[ i ].type == TYPE_DIRECTORY )
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
		qsort( &p[ ( SORT_DIR_MASK & type ) == SORT_DIR_DOWN ? num - nDir : 0 ], nDir, sizeof( FILE_INFO_T ), file_sort_api[ (SORT_DIR_MASK & type) == SORT_DIR_UP ? SORT_NAME_UP : SORT_NAME_DOWN] );

		// File position decision
		row = ( SORT_DIR_MASK & type ) == SORT_DIR_DOWN ? 0 : nDir;
	}
	else
	{
		// �ե����륽�����оݤ�����ˤ���
		nFile = num;
	}

	// File sort is done
	// If the directory has not become the object, it makes all the case objects
	if (( type & SORT_FILE_MASK ) && nFile > -1 )
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
	n1 = ( (a->type & TYPE_MASK) == DIRECTORY_BASE );
	n2 = ( (b->type & TYPE_MASK) == DIRECTORY_BASE );
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
	return ( order ? strcasecmp( b->name, a->name ) : strcasecmp( a->name, b->name ) );
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

	// can't return the difference since could be > 4 bytes
	a = (FILE_INFO_T *) in_a;
	b = (FILE_INFO_T *) in_b;
	if (order) {
		if (b->size > a->size)
			return(1);
		else if (b->size < a->size)
			return(-1);
		else
			return(0);
	} else {
		if (a->size > b->size)
			return(1);
		else if (a->size < b->size)
			return(-1);
		else
			return(0);
	}
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

#define		FIT_TARGET_HEIGHT	(638)


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
	if ( strlen(http_recv_info_p->option) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "option=%s&", http_recv_info_p->option);
		strncat(image_viewer_info.parent_directory_link, work_data, sizeof(image_viewer_info.parent_directory_link) - strlen(image_viewer_info.parent_directory_link));
	}
	if ( strlen(http_recv_info_p->dvdopt) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "dvdopt=%s&", http_recv_info_p->dvdopt);
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
	if ( strlen(http_recv_info_p->option) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "option=%s&", http_recv_info_p->option);
		strncat(image_viewer_info.current_uri_link, work_data, sizeof(image_viewer_info.current_uri_link) - strlen(image_viewer_info.current_uri_link));
	}
	if ( strlen(http_recv_info_p->dvdopt) > 0 )
	{
		snprintf(work_data, sizeof(work_data), "dvdopt=%s&", http_recv_info_p->dvdopt);
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

	// It diverges with the extension
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
# if 0
		image_viewer_width = (image_width  * FIT_TERGET_HEIGHT) / image_height;
		image_viewer_height = FIT_TERGET_HEIGHT;

		if ( image_viewer_width > FIT_TERGET_WIDTH ) // ����Ķ���Ƥ�����
		{
			// ���˹�碌�ƥꥵ�������롣
			image_viewer_width = FIT_TERGET_WIDTH;
			image_viewer_height = image_height * FIT_TERGET_WIDTH / image_width;
		}
# else
		image_viewer_height = FIT_TARGET_HEIGHT;
		image_viewer_width = ((float) FIT_TARGET_HEIGHT * (float) image_width) / (float) image_height; 
# endif

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
	// Execution of substitution 
	//   Substituting the data inside SKIN directly 
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
	off_t	offset=0;
	off_t   previous_size=0;
	int previous_page=0;
	int	file_size_increased=0;
	int i;
	char work_buf[WIZD_FILENAME_MAX];
	
	int photo=(strstr(http_recv_info_p->mime_type, "image/") != NULL);
	int mpeg2video = (strstr(http_recv_info_p->mime_type, "video/mpeg") != NULL);
	int mpeg2audio = (strstr(http_recv_info_p->mime_type, "audio/x-mpeg") != NULL);

	if(!photo && global_param.bookmark_threshold && (mpeg2video || !global_param.flag_bookmarks_only_for_mpeg)) {
		// Check for a bookmark
		FILE	*fp;
		snprintf(work_buf, sizeof(work_buf), "%s.wizd.bookmark", http_recv_info_p->send_filename);
		debug_log_output("Checking for bookmark: '%s'", work_buf);
		fp = fopen(work_buf, "r");
		if(fp != NULL) {
			fgets(work_buf, sizeof(work_buf), fp);
			offset = atoll(work_buf);
			if(fgets(work_buf, sizeof(work_buf), fp) != NULL) {
				previous_size = atoll(work_buf);
				if(fgets(work_buf, sizeof(work_buf), fp) != NULL) {
					previous_page = atoi(work_buf);
				}
			}
			if(previous_size == 0)
				previous_size = http_recv_info_p->file_size;
			 else if(previous_size < http_recv_info_p->file_size)
				file_size_increased = 1;

			fclose(fp);

			// Ignore any bookmarks at the EOF
			if((previous_size > 0) && (offset >= previous_size))
				offset = 0;
			debug_log_output("Bookmark offset: %lld/%lld (page %d)", offset, previous_size, previous_page);
		}
	}

	// HTTP_OK�إå�����
	http_send_ok_header(accept_socket, 0, NULL);

	// Clear out any playlist type overrides
	http_recv_info_p->default_file_type = TYPE_UNKNOWN;

	// Override the slide show duration so slides stay up 'forever'
	global_param.slide_show_seconds = 99999;

	ptr = create_1line_playlist(http_recv_info_p, "", NULL, offset);
	if (ptr != NULL) {
		if(    (mpeg2video && global_param.flag_goto_percent_video)
			|| (mpeg2audio && global_param.flag_goto_percent_audio) ) {
			// add 10% interval chapter points, starting at about our offset
			previous_size /= 10;
			if(file_size_increased) {
				// When file size is increasing, then don't offset the starting "chapter"
				// because we want to use all 10 10% increments for semi-continuous playback
				// of files-of-increasing-size
				previous_page = 0;
			} else {
				if(previous_size > 0)
					previous_page = offset / previous_size;
				else
					previous_page = 0;
				// This should never happen, but just in case...
				if(offset < previous_page*previous_size)
					previous_page--;
			}

			// Parse out the URL from the playlist, so we can substitute our info
			for(i=0; *ptr && ((*ptr != '|') || (++i<3)); ptr++);
			if(*ptr=='|') ptr++;
			for(i=0; ptr[i] && (ptr[i] != '|'); i++);
			ptr[i]=0;

			// Change the bookmark offset relative to the chapter number
			// (note: the LinkPlayer only uses the bookmark in the 1st playlist item,
			//        and then only for the first time that item is played)
			snprintf(work_buf, sizeof(work_buf), "Goto %d percent%s|%lld|0|%s?page=%d|\r\n",
				previous_page*10,
				(offset>0) ? ">>" : "",
				offset-previous_page*previous_size,
				ptr, previous_page);
			send(accept_socket, work_buf, strlen(work_buf), 0);
			for(i=1; i<10; i++) {
				if(++previous_page >= 10)
					previous_page = 0;
				snprintf(work_buf, sizeof(work_buf), "Goto %d percent|0|0|%s?page=%d|\r\n",
					previous_page*10, ptr, previous_page);
				send(accept_socket, work_buf, strlen(work_buf), 0);
			}
		} else {
			// ���Ԥ�������
			send(accept_socket, ptr, strlen(ptr), 0);
		}
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

	unsigned char	*ptr;
	unsigned char	buf[WIZD_FILENAME_MAX];

	unsigned char	listfile_path[WIZD_FILENAME_MAX];

	unsigned char	file_extension[32];
	unsigned char	file_name[255];
	unsigned char	file_uri_link[WIZD_FILENAME_MAX];

	unsigned char	work_data[WIZD_FILENAME_MAX *2];
	SKIN_REPLASE_LINE_DATA_T	mp3_id3tag_data;	// MP3 ID3�����ɤ߹�����

	int i,len,alloc=0,count=0;
	unsigned char **list=NULL;

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

	// If requesting a shuffled list, create the list in memory first
	if(strcmp(http_recv_info_p->sort, "shuffle")==0) {
		// Allocate 10x the max playlist size so randomized list can still be a subset
		// of all of the available files.  
		// If we don't do this, then if our randomized playlist is limited to only the first N files found
		// so some files will never make the list!
		alloc = global_param.max_play_list_search;
		list = (unsigned char **)malloc(alloc*sizeof(unsigned char *));
	}

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
		cut_whitespace_at_linetail(buf);

		debug_log_output("read_buf(comment cut):'%s'", buf);

		// ���Ԥʤ顢continue
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

		// ɽ���ե�����̾ ����
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
				snprintf(work_data, sizeof(work_data), "%d|%d|%s|%s|\r\n", 
					global_param.slide_show_seconds, global_param.slide_show_transition, file_name, ptr );
			} else {
				snprintf(work_data, sizeof(work_data), "%s|%d|%d|%s|\r\n",
					file_name, global_param.slide_show_seconds, global_param.slide_show_seconds, ptr );
			}
			if(list != NULL) {
				list[count++] = strdup(work_data);
			} else {
				write(accept_socket, work_data, strlen(work_data));
			}
		} else {
			// URI����
			if ( buf[0] == '/') // ���Хѥ�
			{
				strncpy( file_uri_link, buf, sizeof(file_uri_link) );
			} else {
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
				if(i == global_param.num_aliases) {
					// No alias found - just copy the strings over
					strncpy( file_uri_link, listfile_path, sizeof(file_uri_link) );
					strncat( file_uri_link, buf, sizeof(file_uri_link) - strlen(file_uri_link) );
				}
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
			mp3_id3_tag_read(work_data , &mp3_id3tag_data, global_param.menu_filename_length_max, SCAN_NONE);
		}


// If the MP3 ID3 tag is existed, playlist indicatory file name is replaced with the ID3 tag.
		if ( mp3_id3tag_data.mp3_id3v1_flag == 1 )
		{
			debug_log_output("got flag of 1, change title_info of %s\n", mp3_id3tag_data.mp3_id3v1_title_info);
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
			// If in EUC conversion -> extension deletion -> (necessity if) half angle letter in full size //conversion -> in the MediaWiz cord/code conversion -> SJIS, illegitimate character code 0x7C (the '|') deleting //the letter which is included.
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

		debug_log_output("file_uri_link(reencoded):'%s'", file_uri_link);


		// ------------------------------------
		// �ץ쥤�ꥹ�Ȥ�����
		// ------------------------------------
		snprintf(work_data, sizeof(work_data), "%s|%d|%d|http://%s%s|\r\n"
			, file_name
			, 0, 0
			, http_recv_info_p->recv_host,
			file_uri_link
		);
		if(list != NULL) {
			list[count++] = strdup(work_data);
		} else {
			write(accept_socket, work_data, strlen(work_data));
		}
		}
	}

	if(list != NULL) {
		// Shuffle the list
		int sent=0;
		srand(time(NULL));
		while((count > 0) && (sent<global_param.max_play_list_items)) {
			i = (int)floor((double)rand() / (double)RAND_MAX * (double)count);
			if(i<count) {
				write(accept_socket, list[i], strlen(list[i]));
				free(list[i]);
				list[i] = list[--count];
				sent++;
			}
		}
		// Free up any leftover unused items
		for(i=0; i<count; i++)
			free(list[i]);

		free(list);		
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


// **************************************************************************
// * ���ץ�����˥塼�����������ֿ�
// **************************************************************************
void http_option_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	SKIN_T	*skin;
	SKIN_REPLASE_GLOBAL_DATA_T	*skin_rep_data_global_p;


	// ==========================================
	// Read Skin Config
	// ==========================================
	skin_read_config(SKIN_MENU_CONF);

	// ==========================================
	// Initialize the global skin data structure
	// ==========================================
	skin_rep_data_global_p = skin_create_global_data(http_recv_info_p, 0);
	if(skin_rep_data_global_p == NULL)
		return;

	// ==============================
	// OptionMenu �������ɤ߹���
	// ==============================
	if ((skin = skin_open(SKIN_OPTION_MENU_HTML)) == NULL) {
		return ;
	}

	// =================
	// Send the resulting file with skin substitutions
	// =================

	http_send_ok_header(accept_socket, 0, NULL);

	// Substituting the data inside SKIN directly
	skin_direct_replace_global(skin, skin_rep_data_global_p);
	skin_direct_send(accept_socket, skin);
	skin_close(skin);

	free( skin_rep_data_global_p );

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
	// in_flag, out_flag��ߤơ�libnkf�ؤΥ��ץ������Ȥ�Ω�Ƥ롣
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
	// libnkf �¹�
	//=================================================
	debug_log_output("nkf %s '%s'", nkf_option, in);
	nkf(in, out, len, nkf_option);
	debug_log_output("nkf result '%s'", out);




	return;
}

char *
text_genre(unsigned char *genre) {
   int genre_num = (int) genre[0];

   if(genre_num <= MAXGENRE)
	return(typegenre[genre_num]);
   else
	return("(UNKNOWN)");
}

static unsigned long id3v2_len(unsigned char *buf)
{
	return buf[0] * 0x200000 + buf[1] * 0x4000 + buf[2] * 0x80 + buf[3];
}

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
		{ "TRCK", skin_rep_data_line_p->mp3_id3v1_track
			, sizeof(skin_rep_data_line_p->mp3_id3v1_track) },
	};
	int list_count = sizeof(copy_list) / sizeof(struct _copy_list);
	int i, j;
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

	// 10byte��read.
	read(fd, buf, 10);
	// debug_log_output("buf='%s'", buf);

	// "ID3" character string check
	if ( strncmp( buf, "ID3", 3 ) != 0 )
	{
		/*
		 *  Adhering to the rear of the file, the �� ID3v2 tag
		 *  When there is on the middle of the file therefore trouble you do not read.
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
			debug_log_output("ID3v2 Tag[%s] found. '%s'\n", copy_list[i].id, frame + 1);
			if (frame[1] == 0xff) {
				for (j = 1; j < frame_len / 2; j++) {
					copy_list[i].container[j-1] = frame[(j * 2) + 1];
					if (copy_list[i].container[j-1] == 0)
						break;
				}
			} else if (frame[1] == 0xfe) {
				for (j = 1; j < frame_len / 2; j++) {
					copy_list[i].container[j-1] = frame[(j * 2)];
					if (copy_list[i].container[j-1] == 0)
						break;
				}
			} else {
				convert_language_code(	frame + 1,
					copy_list[i].container,
					copy_list[i].maxlen,
					CODE_AUTO,
					global_param.client_language_code);
			}

			// printf("tag %s, values %s\n", copy_list[i].id, copy_list[i].container);
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

static void  mp3_id3_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p, int limited_length_max, int scan_type)
{
    FILE  *fp;
    mp3info mp3;
	char *p;

	skin_rep_data_line_p->mp3_id3v1_flag = 0;

    if ( !( fp=fopen(mp3_filename,"rb+") ) ) {
	    return;
    }

    memset(&mp3,0,sizeof(mp3info));
    mp3.file=fp;
    mp3.filename=mp3_filename;
	if (get_mp3_info(&mp3, scan_type, 1) == 1) {
		fclose(fp);

		sprintf(skin_rep_data_line_p->mp3_id3v1_bitrate, "%d Kbps", header_bitrate(&mp3.header));
		strcpy(skin_rep_data_line_p->mp3_id3v1_stereo, header_mode(&mp3.header));
		sprintf(skin_rep_data_line_p->mp3_id3v1_frequency, "%d KHz", header_frequency(&mp3.header) / 1000);

		sprintf(skin_rep_data_line_p->file_duration, "%d:%02d", mp3.seconds / 60, mp3.seconds % 60);

		mp3_id3v2_tag_read(mp3_filename, skin_rep_data_line_p);
		skin_rep_data_line_p->mp3_id3v1_flag = 1;
		return;
	}

# if 0
    printf("\nseconds (%d) %d:%02d\n", mp3.seconds, mp3.seconds / 60, mp3.seconds % 60);

    printf("title: %s\n", mp3.id3.title);
    printf("artist: %s\n", mp3.id3.artist);
    printf("album: %s\n", mp3.id3.album);
    printf("year: %s\n", mp3.id3.year);
    printf("comment: %s\n", mp3.id3.comment);
    printf("track: %c\n", mp3.id3.track[0]);
    printf("genre: %s\n", text_genre(mp3.id3.genre));
    printf("bitrate: %d\n", header_bitrate(&mp3.header));
    printf("stereo/mono: %s\n", header_mode(&mp3.header));
    printf("frequency: %d KHz\n", header_frequency(&mp3.header)/1000);
# endif

	if (mp3.id3.title[0]) {
		strcpy(skin_rep_data_line_p->mp3_id3v1_title, mp3.id3.title);
		strcpy(skin_rep_data_line_p->mp3_id3v1_title_info, mp3.id3.title);
		strcpy(skin_rep_data_line_p->mp3_id3v1_title_info_limited, mp3.id3.title);
	} else {
		strcpy(skin_rep_data_line_p->mp3_id3v1_title, basename(mp3_filename));
		p = strrchr(skin_rep_data_line_p->mp3_id3v1_title, '.');
		p[0] = 0;
		strcpy(skin_rep_data_line_p->mp3_id3v1_title_info, basename(mp3_filename));
		strcpy(skin_rep_data_line_p->mp3_id3v1_title_info_limited, basename(mp3_filename));
	}
	strcpy(skin_rep_data_line_p->mp3_id3v1_album, mp3.id3.album);
	strcpy(skin_rep_data_line_p->mp3_id3v1_artist, mp3.id3.artist);
	strcpy(skin_rep_data_line_p->mp3_id3v1_year, mp3.id3.year);
	strcpy(skin_rep_data_line_p->mp3_id3v1_comment, mp3.id3.comment);

	sprintf(skin_rep_data_line_p->mp3_id3v1_track, "%d", (int) mp3.id3.track[0]);
	strcpy(skin_rep_data_line_p->mp3_id3v1_genre, text_genre(mp3.id3.genre));
	sprintf(skin_rep_data_line_p->mp3_id3v1_bitrate, "%d Kbps", header_bitrate(&mp3.header));
	strcpy(skin_rep_data_line_p->mp3_id3v1_stereo, header_mode(&mp3.header));
	sprintf(skin_rep_data_line_p->mp3_id3v1_frequency, "%d KHz", header_frequency(&mp3.header) / 1000);

	sprintf(skin_rep_data_line_p->file_duration, "%d:%02d", mp3.seconds / 60, mp3.seconds % 60);

	skin_rep_data_line_p->mp3_id3v1_flag = 1;

	fclose(fp);
}


// *************************************************************************************
// The indicatory file name which transfers to playlist, is adjusted the type which is not problem
// *************************************************************************************
static void playlist_filename_adjustment(unsigned char *src_name, unsigned char *dist_name, int dist_size, int input_code )
{
	unsigned char	work_data[WIZD_FILENAME_MAX];


	// ---------------------------------
	// �ե�����̾ ���� (ɽ����)
	// EUC���Ѵ� �� ��ĥ�Һ�� �� (ɬ�פʤ�)Ⱦ��ʸ�������Ѥ��Ѵ� �� MediaWiz�����ɤ��Ѵ� �� SJIS�ʤ顢����ʸ��������0x7C('|')��ޤ�ʸ�����������ˡ�
	// ---------------------------------
# if 0
	convert_language_code(	src_name,
							dist_name,
							dist_size,
							input_code | CODE_HEX,
							CODE_EUC);
# else
	strcpy(dist_name, src_name);
# endif

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
				// sjis��1byte��

				
				sjis_f = TRUE;
			} else if (*curp == '\\'){
				*curp = '/';			// Replacement
			}
		} else {
			// sjis��2byte��
			sjis_f = FALSE;
		}
		curp++;
	}


	// Acquiring the directory name of the single step eye from the first
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


	// Loop every of directory class
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
				strncpy(curp, dent->d_name, strlen(dent->d_name));		
// ������ʬ�������̾�����֤�����
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

typedef unsigned long FOURCC;
char *str_fourcc(FOURCC f) {
	static char buf[5];

	buf[0] = f & 0xff;
	buf[1] = (f >> 8) & 0xff;
	buf[2] = (f >> 16) & 0xff;
	buf[3] = (f >> 24) & 0xff;
	buf[4] = '\0';

	return buf;
}

char *
avi_get_audio(int code)
{
	static char buf[20];

	switch (code) {
	  case 0x55:
		return("MP3");
      case 0x1:
		return("PCM");
      case 0x2:
		return("MS ADPCM");
      case 0x11:
      case 0x31:
      case 0x32:
      case 0x50:
		return("MPEG 1/2");
      case 0x160:
      case 0x161:
		return("DivX WMA");
      case 0x401:
      case 0x2000:
      case 0x2001:
		return("AC3");
	  default:
		sprintf(buf, "?? (%d)", code);
		return("??");
	}
}

static int read_avi_info(char *fname, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p)
{
	avi_t *avifile;
	int     frames;
	double  fps;
	unsigned long     seconds;

	if ((avifile = AVI_open_input_file(fname)) == 0) {
		return(-1);
	}

	snprintf(skin_rep_data_line_p->image_width, sizeof(skin_rep_data_line_p->image_width), "%lu", (long unsigned) AVI_video_width(avifile));

	snprintf(skin_rep_data_line_p->image_height, sizeof(skin_rep_data_line_p->image_height), "%lu", (long unsigned) AVI_video_height(avifile));

	snprintf(skin_rep_data_line_p->avi_fps, sizeof(skin_rep_data_line_p->avi_fps), "%.3f", AVI_frame_rate(avifile));

	snprintf(skin_rep_data_line_p->avi_vcodec, sizeof(skin_rep_data_line_p->avi_vcodec), "%s", AVI_video_compressor(avifile));

	snprintf(skin_rep_data_line_p->avi_acodec, sizeof(skin_rep_data_line_p->avi_acodec), "%s", avi_get_audio(AVI_audio_format(avifile)));

	snprintf(skin_rep_data_line_p->avi_hvcodec, sizeof(skin_rep_data_line_p->avi_hvcodec), "%d Channels", AVI_audio_channels(avifile));

	snprintf(skin_rep_data_line_p->avi_hacodec, sizeof(skin_rep_data_line_p->avi_hacodec), "%ld", AVI_audio_mp3rate(avifile));

	frames = AVI_video_frames(avifile);
	fps = AVI_frame_rate(avifile);

	seconds = (frames / fps);

	snprintf(skin_rep_data_line_p->avi_duration, sizeof(skin_rep_data_line_p->avi_duration), "%lu:%02lu:%02lu", seconds /3600, (seconds % 3600) / 60, seconds % 60);

	(void)  AVI_close(avifile);

	return 0;
}

static
void do_search(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	FILE_INFO_T		*file_info_p;
	unsigned char	*file_info_malloc_p;
	int				 file_num = 0;
	int				 i;
	int				 type;
	int				 nfile_num = 0;
	char			*name;
	regex_t			 re;

	type = http_recv_info_p->search_type;
	// printf("searching for %s in %s\n", http_recv_info_p->search, http_recv_info_p->recv_uri);
	name = 0;
	if (http_recv_info_p->lsearch[0] != '\0') {
		for (i = 0; i < global_param.num_aliases; i++) {
			if (strncmp(&(http_recv_info_p->recv_uri[1]),
						  global_param.alias_name[i],
						  strlen(global_param.alias_name[i])) == 0) {
				name = global_param.alias_name[i];
				break;
			}
		}
	}

	// if they are searching everything then set a high limit for now
	// after the search is done, limit the number of files returned
	if (strcmp(http_recv_info_p->search, ".*") == 0 || global_param.max_search_hits <= 0)
		file_num = 10000;
	else
		file_num = global_param.max_search_hits;

	file_info_malloc_p = malloc(sizeof(FILE_INFO_T) * (file_num + 1));
	if ( file_info_malloc_p == NULL ) {
		debug_log_output("malloc() error");
		return;
	}
	memset(file_info_malloc_p, 0, sizeof(FILE_INFO_T)*(file_num+1));
	file_info_p = (FILE_INFO_T *)file_info_malloc_p;

	nfile_num = 0;

	if (regcomp(&re, http_recv_info_p->search, REG_EXTENDED | REG_ICASE) != 0) {
		// pattern was bad, don't return anything
		create_skin_filemenu(accept_socket, http_recv_info_p,file_info_p, 0, 1);
		return;
	}

	for (i = 0; i < global_param.num_aliases; i++) {
		if (name != 0) {
			if (strcmp(name, global_param.alias_name[i]) != 0)
				continue;
		} else if (type != TYPE_UNKNOWN &&
			       type != global_param.alias_default_file_type[i])
			continue;
		else if (global_param.alias_default_file_type[i] == TYPE_SECRET)
			continue;

		// printf("comparing files in %s\n", global_param.alias_path[i]);
		nfile_num += next_directory_stat(global_param.alias_path[i], &file_info_p[nfile_num], file_num - nfile_num, &re, i, 1);
	}

	// printf("found %d files\n", nfile_num);

	regfree(&re);

	set_sort_rule(http_recv_info_p, nfile_num);

	file_info_sort(file_info_p, nfile_num, global_param.sort_rule);

	if (strcmp(http_recv_info_p->search, ".*") == 0 && nfile_num > global_param.max_search_hits && global_param.max_search_hits > 0)
		nfile_num = global_param.max_search_hits;

	strcpy(global_param.search, http_recv_info_p->search);
	create_skin_filemenu(accept_socket,
					     http_recv_info_p,
						 file_info_p,
						 nfile_num,
						 1);

	free(file_info_malloc_p);
}
