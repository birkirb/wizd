// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_send_vob.c
//											$Revision: 1.3 $
//											$Date: 2004/10/10 07:26:56 $
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
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <dirent.h>
#include <errno.h>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>

#include "wizd.h"

static off_t http_vob_file_send(int	accept_socket, JOINT_FILE_INFO_T *joint_file_info_p, off_t content_length, off_t range_start_pos );




// **************************************************************************
// JOINTファイルを、連結して返信する。
// **************************************************************************
int http_vob_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p  )
{
	int	send_header_data_len;
	int	result_len;

	unsigned char	send_http_header_buf[2048];
	unsigned char	work_buf[1024];

	off_t		joint_content_length = 0;
	off_t		written = 0;

	JOINT_FILE_INFO_T *joint_file_info_p;

	int	ret;
	int	i;
	int split_vob = 0;


	debug_log_output("---------------------------------------------------");
	debug_log_output("http_vob_file_response() start" );

	debug_log_output("dvdopt is %s\n", http_recv_info_p->dvdopt);
	if (strlen(http_recv_info_p->dvdopt) > 0)
	{
		if (strcasecmp(http_recv_info_p->dvdopt ,"splitchapters") == 0 ) {
			global_param.flag_split_vob_chapters = 1;
			split_vob = 1;
		} else if (strcasecmp(http_recv_info_p->dvdopt ,"nosplitchapters") == 0 )
			global_param.flag_split_vob_chapters = 0;
		else if (strcasecmp(http_recv_info_p->dvdopt ,"notitlesplit") == 0 )
			global_param.flag_split_vob_chapters = 2;
	} else if (global_param.flag_split_vob_chapters)
		split_vob = 1;

	// -----------------------
	// 作業用変数初期化
	// -----------------------
	memset(send_http_header_buf, '\0', sizeof(send_http_header_buf) );

	// -----------------------
	// ワーク領域確保
	// -----------------------
	joint_file_info_p = malloc( sizeof(JOINT_FILE_INFO_T) );
	if ( joint_file_info_p == NULL )
	{
		debug_log_output("malloc() error.");
		return ( -1 );
	}


	// --------------------------
	// vobファイル情報をGET
	// --------------------------
	ret = analyze_vob_file(http_recv_info_p->send_filename, joint_file_info_p );
	debug_log_output("http_vob_file_response() end (ret=%d)" );
	if ( ret >= 0 )
	{
		debug_log_output("joint_file_info.file_num = %d", joint_file_info_p->file_num);
		debug_log_output("joint_file_info.total_size = %llu", joint_file_info_p->total_size);
		for ( i = 0; i< joint_file_info_p->file_num; i++ )
		{
			debug_log_output("[%02d] '%s' %lld\n", i, joint_file_info_p->file[i].name, joint_file_info_p->file[i].size );
		}

		// 送信するcontnet_length 計算
		if ( http_recv_info_p->range_end_pos > 0 )
		{
			joint_content_length = (http_recv_info_p->range_end_pos - http_recv_info_p->range_start_pos) + 1;
		}
		else
		{
			joint_content_length = joint_file_info_p->total_size - http_recv_info_p->range_start_pos;
		}
	}

	/*
	 * If requesting a chapter number, check the IFO file for chapter start position
	 */
	if(http_recv_info_p->page > 0)
	{
		dvd_reader_t	*dvd = NULL;
		ifo_handle_t	*vmg_file = NULL;
		tt_srpt_t	*tt_srpt;
		ifo_handle_t	*vts_file = NULL;
		vts_ptt_srpt_t	*vts_ptt_srpt;
		pgc_t		*cur_pgc;
		int		ttn, pgc_id, start_cell, pgn;
		off_t		start_sector = 0;
		int		titleid = (http_recv_info_p->page / 1000) - 1;
		int	    chapid = (http_recv_info_p->page % 1000);

		debug_log_output("title %d, chapter %d, split %d\n", titleid, chapid, split_vob);

		// Get the title set number
		char *ptr = strstr(http_recv_info_p->send_filename, "VTS_");
		if(ptr == NULL) // Should always be uppercase, but just in case...
			ptr = strstr(http_recv_info_p->send_filename, "vts_");
		if(ptr != NULL) {
			int vts = atoi(ptr+4);

			// Get the DVD root directory		
			strncpy(work_buf, http_recv_info_p->send_filename, sizeof(work_buf));
			cut_after_last_character(work_buf, '/');
			cut_after_last_character(work_buf, '/');

			debug_log_output("Checking for DVD in '%s'\n", work_buf);

			dvd = DVDOpen( work_buf );
			if( dvd ) {
				vmg_file = ifoOpen( dvd, 0 );
				if( vmg_file ) {
					debug_log_output("Found DVD in '%s', looking for VTS=%d, Chapter=%d\n", work_buf, vts, chapid);
					tt_srpt = vmg_file->tt_srpt;

					// Find the titleid matching this VTS
							vts_file = ifoOpen( dvd, vts );
	
					if( vts_file ) {
						/**
						 * Determine which program chain we want to watch.  This is based on the
						 * chapter number.
						 */
						if( chapid < tt_srpt->title[ titleid ].nr_of_ptts ) {
							debug_log_output("Found vts=%d, chapter=%d in title %d\n", vts, chapid, titleid);
							ttn = tt_srpt->title[ titleid ].vts_ttn;
							vts_ptt_srpt = vts_file->vts_ptt_srpt;
							pgc_id = vts_ptt_srpt->title[ ttn - 1 ].ptt[ chapid ].pgcn;
							pgn = vts_ptt_srpt->title[ ttn - 1 ].ptt[ chapid ].pgn;
							cur_pgc = vts_file->vts_pgcit->pgci_srp[ pgc_id - 1 ].pgc;
							start_cell = cur_pgc->program_map[ pgn - 1 ] - 1;
							start_sector = cur_pgc->cell_playback[ start_cell ].first_sector;

							debug_log_output("Chapter start sector %llu\n", start_sector);

							/* Offset the file by the start sector */
							start_sector *= 2048;
							http_recv_info_p->range_start_pos += start_sector;
							debug_log_output("New range start pos = %llu\n", http_recv_info_p->range_start_pos);
							if ( http_recv_info_p->range_end_pos <= 0 )
							{
								joint_content_length = joint_file_info_p->total_size - http_recv_info_p->range_start_pos;
								/* Keep each chapter as a separate pseudo-file */
								if( split_vob ) {
									int end_cell;
									off_t end_sector;

									if (pgn < cur_pgc->nr_of_programs)
										end_cell = cur_pgc->program_map[pgn]-1;
									else
										end_cell = cur_pgc->nr_of_cells;

									end_sector = cur_pgc->cell_playback[ end_cell - 1].last_sector;
									debug_log_output("Next chapter start sector = %llu\n", end_sector);
									end_sector *= 2048;
									end_sector -= http_recv_info_p->range_start_pos;
									if( (end_sector>0) && (end_sector < joint_content_length)) {
										joint_content_length = end_sector;
										debug_log_output("Restricting content length to only chapter %d\n", chapid+1);
									}
								}
								debug_log_output("Adjusted content length: %llu\n", joint_content_length);
							}
						} else {
							debug_log_output("Invalid chapter=%d, title %d only has %d chapters\n", chapid, titleid, tt_srpt->title[ titleid ].nr_of_ptts);
						}
						ifoClose( vts_file );
					} else {
						debug_log_output("No title found for vts=%d\n", vts);
					}
					ifoClose( vmg_file );
				}
				DVDClose( dvd );
			}
		}
	}

	// -------------------------
	// HTTP_OK ヘッダ生成
	// -------------------------
	strncpy(send_http_header_buf, HTTP_OK, sizeof(send_http_header_buf));
	strncat(send_http_header_buf, HTTP_CONNECTION, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );

	snprintf(work_buf, sizeof(work_buf), HTTP_SERVER_NAME, SERVER_NAME);
	strncat(send_http_header_buf, work_buf, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );

	snprintf(work_buf, sizeof(work_buf), HTTP_CONTENT_LENGTH, joint_content_length);
	strncat(send_http_header_buf, work_buf, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );

	snprintf(work_buf, sizeof(work_buf), HTTP_CONTENT_TYPE, http_recv_info_p->mime_type);
	strncat(send_http_header_buf, work_buf, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );
	strncat(send_http_header_buf, HTTP_END, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );



	send_header_data_len = strlen(send_http_header_buf);
	debug_log_output("send_header_data_len = %d\n", send_header_data_len);
	debug_log_output("--------\n");
	debug_log_output("%s", send_http_header_buf);
	debug_log_output("--------\n");


	// --------------
	// ヘッダ返信
	// --------------
	result_len = send(accept_socket, send_http_header_buf, send_header_data_len, 0);
	debug_log_output("result_len=%d, send_header_data_len=%d\n", result_len, send_header_data_len);


	// --------------
	// 実体返信
	// --------------
	written = http_vob_file_send(	accept_socket, 	joint_file_info_p, 
											joint_content_length,
											http_recv_info_p->range_start_pos );


	free( joint_file_info_p );	// Memory Free.

	if(global_param.bookmark_threshold) {
		if(   (written < global_param.bookmark_threshold)
		   || (joint_content_length && ((written+global_param.bookmark_threshold) > joint_content_length))) {
			// Less than 10 MB written, or within 10 MB of the end of file
			// Remove any existing bookmark
			snprintf(work_buf, sizeof(work_buf), "%s.wizd.bookmark", http_recv_info_p->send_filename);
			if(0 == unlink(work_buf)) {
				debug_log_output("Removed bookmark: '%s'", work_buf);
			}
		} else {
			// Wrote more than 10MB, save a bookmark
			snprintf(work_buf, sizeof(work_buf), "%s.wizd.bookmark", http_recv_info_p->send_filename);
			written += http_recv_info_p->range_start_pos;
			written -= global_param.bookmark_threshold;
			if(written > 0) {
				FILE *fp = fopen(work_buf, "w");
				if(fp != NULL) {
					fprintf(fp, "%lld\n", written);
					fclose(fp);
					debug_log_output("Wrote bookmark: '%s' %lld", work_buf, written);
				}
			} else {
				// Remove any old bookmarks
				if(0 == unlink(work_buf)) {
					debug_log_output("Removed bookmark: '%s'", work_buf);
				}
			}
		}
	}
	return 0;
}





// **************************************************************************
// ファイルの実体を送信する。
// **************************************************************************
static off_t http_vob_file_send(int	accept_socket, JOINT_FILE_INFO_T *joint_file_info_p, off_t content_length, off_t range_start_pos )
{
	int fd;
	off_t			lseek_ret;
	off_t			start_file_pos;
	off_t			left_pos;
	off_t			written;

	debug_log_output("---------------------------------------------------");
	debug_log_output("http_vob_file_send() start" );

	debug_log_output("range_start_pos=%lld", range_start_pos);
	debug_log_output("content_length=%lld", content_length);


	// -------------------------
	// スタート位置を計算
	// -------------------------
	debug_log_output("calc start pos.");
	joint_file_info_p->current_file_num = 0;
	start_file_pos = 0;

	left_pos = range_start_pos;
	while ( 1 ) 
	{
		debug_log_output("[%02d] left_pos = %lld", joint_file_info_p->current_file_num, left_pos ) ;

		// 残りバイト数が、ファイルよりも大きいとき。
		if ( left_pos > joint_file_info_p->file[joint_file_info_p->current_file_num].size )
		{
			left_pos -= joint_file_info_p->file[joint_file_info_p->current_file_num].size;
			joint_file_info_p->current_file_num++;
			start_file_pos = 0;
		}
		else // 残りバイト数が、ファイルより小さいとき。
		{
			start_file_pos = left_pos;
			break;
		}
	}

	debug_log_output("start_file_num=%d", joint_file_info_p->current_file_num);
	debug_log_output("start_file_pos=%d", start_file_pos);



	// ------------------------------
	// 最初のファイルをオープン
	// ------------------------------
	fd = open(joint_file_info_p->file[joint_file_info_p->current_file_num].name , O_RDONLY);
	if ( fd < 0 )
	{	
		debug_log_output("open() error. '%s'", joint_file_info_p->file[joint_file_info_p->current_file_num].name);
		return ( -1 );
	}


	// ------------------------------------------------
	// スタート位置(range_start_pos)へファイルシーク
	// ------------------------------------------------
	lseek_ret = lseek(fd, start_file_pos, SEEK_SET);
	if ( lseek_ret < 0 )	// lseek エラーチェック
	{
		debug_log_output("lseek() error.");
		close(fd);
		return ( -1 );
	}

	written = copy_descriptors(fd, accept_socket, content_length, joint_file_info_p);

	// Note: can't close fd because it may change inside copy_descriptors

	// 正常終了
	return written;
}





// **************************************************************************
// vobファイルを解析して、joint_file_info_p に入れる。
// **************************************************************************
int analyze_vob_file(unsigned char *vob_filename, JOINT_FILE_INFO_T *joint_file_info_p )
{
	unsigned char	read_filename[SVI_FILENAME_LENGTH+10];

	unsigned char	vob_filepath[WIZD_FILENAME_MAX];

	unsigned char	first_filename[WIZD_FILENAME_MAX];
	unsigned char	series_filename[WIZD_FILENAME_MAX];

	int			ret;
	int			i;

	struct stat file_stat;


	debug_log_output("analyze_vob_file() start.");
	debug_log_output("vob_filename='%s'", vob_filename);

	// ------------------
	// ワーク初期化
	// ------------------
	memset(read_filename, '\0', sizeof(read_filename));
	memset(first_filename, '\0', sizeof(first_filename));



	// ----------------------------------------
	// vobのファイルネームをGETしておく /VIDEO_TS/VTS_01_?.VOB
	// ----------------------------------------
	strncpy(vob_filepath, vob_filename, sizeof(vob_filepath));

	debug_log_output("vob_filepath = '%s'", vob_filepath);

	strncpy(first_filename, vob_filepath, sizeof(vob_filepath));

	debug_log_output("first_filename = '%s'", first_filename);

	// ----------------------------------------------------
	// vobから読んだファイルの、ファイル情報をGet
	// ----------------------------------------------------
	ret = stat(first_filename, &file_stat);
	if ( ret != 0 )
	{
		debug_log_output("'%s' Not found.", first_filename);
		return ( -1 );
	}

	// --------------------------------------------------------
	// 1個目のファイル情報をjoint_file_info_p にセット
	// --------------------------------------------------------
	memset(joint_file_info_p, 0, sizeof(JOINT_FILE_INFO_T));

	strncpy( joint_file_info_p->file[0].name, first_filename, sizeof(joint_file_info_p->file[0].name));
	joint_file_info_p->file[0].size 	= file_stat.st_size; 
	joint_file_info_p->total_size 		= file_stat.st_size;
	joint_file_info_p->file_num = 1;


	// --------------------------------------------------------
	// 2個目以降のファイル情報を調べてjoint_file_info_p にセット
	// --------------------------------------------------------
	strncpy( series_filename, first_filename, sizeof(series_filename));
	for ( i=1; i<JOINT_MAX; i++ )
	{
		// ファイル名生成
		// /VTS_01_1.VOB → VTS_01_2.VOB
		series_filename[strlen(series_filename)-5]++;

		// ファイル情報をGET
		ret = stat(series_filename, &file_stat);
		if ( ret != 0 ) // ファイル無し。検索終了。
		{
			debug_log_output("'%s' Not found.", series_filename);
			break;
		}

		debug_log_output("[%d]_filename = '%s'", i+1, series_filename);

		// GETしたファイル情報を、joint_file_info_p にセット
		strncpy( joint_file_info_p->file[i].name, series_filename, sizeof(joint_file_info_p->file[0].name));
		joint_file_info_p->file[i].size 	= file_stat.st_size; 
		joint_file_info_p->total_size 		+= file_stat.st_size;
		joint_file_info_p->file_num++;

	}

	joint_file_info_p->current_file_num = 0;
	debug_log_output("analyze_vob_file() end.");

	return 0;
}

