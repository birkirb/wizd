// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_send_vob.c
//											$Revision: 1.18 $
//											$Date: 2006/07/09 17:02:09 $
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
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <dirent.h>
#include <errno.h>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>

// HAVE_LIB_DVDCSS is defined on the compile line in the makefile
# ifndef HAVE_LIB_DVDCSS
    // there is no libdvdcss - default compile of wizd.exe
static int	No_Lib_Dvd_Css=1;
# else
    // libdvdcss exists - compiling wizd_css.exe
#include <dvdread/dvd_udf.h>
#include <dvdread/nav_read.h>
#include <dvdread/nav_print.h>

static int	No_Lib_Dvd_Css=0;
static unsigned char data[ 1024 * DVD_VIDEO_LB_LEN ];
# endif

#include "wizd.h"

static off_t http_vob_file_send(int	accept_socket, JOINT_FILE_INFO_T *joint_file_info_p, off_t content_length, off_t range_start_pos );
static int send_dvd_test(int accept_socket, char *name, int titleid, int chapid, off_t range_start_pos);

static void
print_joint_files(JOINT_FILE_INFO_T *joint_file_info_p)
{
	int i;

	printf("Total size: %lld\n", joint_file_info_p->total_size);
	for (i = 0; i < joint_file_info_p->file_num; i++) {
		if (joint_file_info_p->file[i].start_pos == 0)
			printf(" \n");

		printf("%d: start %lld, size %lld, (%lld, %lld)\n",
						 i,
						 joint_file_info_p->file[i].start_pos,
						 joint_file_info_p->file[i].size,
						 joint_file_info_p->file[i].start_pos / 2048LL,
						 (joint_file_info_p->file[i].start_pos +
						  joint_file_info_p->file[i].size) / 2048LL);
		printf("    filename %s\n",
						 joint_file_info_p->file[i].name);
	}
}


static void
http_set_cells_to_pseudo_files(JOINT_FILE_INFO_T * joint_file_info_p,
							   off_t *offset, off_t * joint_content_length, int titleid,
							   int chapid, ifo_handle_t * vmg_file,
							   ifo_handle_t * ifo_title, char *filename)
{
	JOINT_FILE_INFO_T   dummy_file;
	tt_srpt_t          *tt_srpt;
	vts_ptt_srpt_t     *vts_ptt_srpt;
	int                 ttn;
	int                 cur_cell;
	int                 pgc_id, pgn;
	off_t               size;
	pgc_t              *cur_pgc;
	int                 i, j;
	int                 cur_ind;
	int                 start_ind;
	int                 join_ind;
	int                 diff;
	off_t               start_pos, start_off;
	off_t		    sector_offset = 0;
	int		    firstchap, lastchap;
	struct
	{
		off_t               start_pos;
		off_t               end_pos;
	} cellinfo[256];

	tt_srpt = vmg_file->tt_srpt;
	ttn = tt_srpt->title[titleid].vts_ttn;

	vts_ptt_srpt = ifo_title->vts_ptt_srpt;

	joint_file_info_p->current_file_num = 0;
	joint_file_info_p->file[0].start_pos = -1;
	*joint_content_length = 0;

	/* loop through the chapters */
	cur_ind = 0;
	cur_cell = 0;
	debug_log_output("Title %d has %d chapters, %d program chains\n",
					 titleid + 1, tt_srpt->title[titleid].nr_of_ptts,
					 ifo_title->vts_pgcit->nr_of_pgci_srp);

	(void)analyze_vob_file(filename, &dummy_file);
	debug_log_output("dummy_file info:\n");
	//print_joint_files(&dummy_file);

	/* find all the cells that aren't < 1 sec. also only angle 1 */
	if (global_param.flag_split_vob_chapters == 1) {
		firstchap = chapid;
		lastchap = chapid + 1;
	} else {
		firstchap = 0;
		lastchap = tt_srpt->title[titleid].nr_of_ptts;
	}

	for (i = firstchap; i < lastchap; i++) {
		pgc_id = vts_ptt_srpt->title[ttn - 1].ptt[i].pgcn;
		pgn = vts_ptt_srpt->title[ttn - 1].ptt[i].pgn;
		cur_pgc = ifo_title->vts_pgcit->pgci_srp[pgc_id - 1].pgc;
		cur_cell = cur_pgc->program_map[pgn - 1] - 1;

		if (pgn == cur_pgc->nr_of_programs)
			diff = cur_pgc->nr_of_cells - cur_pgc->program_map[pgn - 1] + 1;
		else
			diff = cur_pgc->program_map[pgn] - cur_pgc->program_map[pgn - 1];

		debug_log_output
			("Title %d, Chap %d, PGC %d, PGN %d has %d cells, cur_cell %d\n",
			 titleid + 1, i + 1, pgc_id, pgn, diff, cur_cell);

		diff += cur_cell;

		/* loop through the cells of this chapter */
		for (; cur_cell < diff; cur_cell++) {

# if 1
			if (cur_pgc->cell_playback[cur_cell].playback_time.second <= 1 &&
				cur_pgc->cell_playback[cur_cell].playback_time.minute == 0 &&
				cur_pgc->cell_playback[cur_cell].playback_time.hour == 0) {
				/* got a short one */
				debug_log_output
					("skipping cell %d has start of %d, end %d\n",
					 cur_cell + 1,
					 cur_pgc->cell_playback[cur_cell].first_sector,
					 cur_pgc->cell_playback[cur_cell].last_sector);
				continue;
			}
# endif

# if 1
			if (cur_pgc->cell_playback[cur_cell].block_type == 1 &&
				cur_pgc->cell_playback[cur_cell].block_mode != 1) {
				/* angle is not angle 1 */
				debug_log_output("skipping cell %d has angle of %d\n",
								 cur_cell + 1,
								 cur_pgc->cell_playback[cur_cell].block_mode);
				continue;
			}
# endif

			if (((cur_pgc->cell_playback[cur_cell].first_sector+1) * 2048LL) >= dummy_file.total_size) {
				debug_log_output("skipping cell %d, first_sector not in vobs!\n", cur_cell + 1);
				//printf("skipping cell %d, first_sector not in vobs!\n", cur_cell + 1);
				continue;
			}

			cellinfo[cur_ind].start_pos =
				cur_pgc->cell_playback[cur_cell].first_sector * 2048LL;

			if (((cur_pgc->cell_playback[cur_cell].last_sector+1)* 2048LL) >= dummy_file.total_size) {
				debug_log_output("adjusting cell %d, last_sector not in vobs!\n", cur_cell + 1);
				//printf("adjusting cell %d, last_sector not in vobs!\n", cur_cell + 1);
				cellinfo[cur_ind].end_pos = dummy_file.total_size - 2048;
			} else
				cellinfo[cur_ind].end_pos =
					(cur_pgc->cell_playback[cur_cell].last_sector + 1) * 2048LL;

			debug_log_output
				("cell %d (ind %d) has start of %d, end %d, angle %d,%d\n",
				 cur_cell + 1, cur_ind, 
				 cur_pgc->cell_playback[cur_cell].first_sector,
				 cur_pgc->cell_playback[cur_cell].last_sector,
				 cur_pgc->cell_playback[cur_cell].block_mode,
				 cur_pgc->cell_playback[cur_cell].block_type);

			cur_ind++;

			// Keep track of the byte offset for the selected chapter
			if(i < chapid) {
				sector_offset += (cur_pgc->cell_playback[cur_cell].last_sector - cur_pgc->cell_playback[cur_cell].first_sector);
				debug_log_output("Chapter %d starts at sector %lld\n", i+2, sector_offset);
			}
		}
	}

	sector_offset *= 2048LL;
	debug_log_output("Adjusting offset by %lld bytes to get to the beginning of chapter %d",
		sector_offset, chapid+1);
	(*offset) += sector_offset;
	
	start_ind = 0;
	join_ind = 0;
	debug_log_output("Found %d non-zero cells\n", cur_ind);

	/* join together contiguous cells */
	for (i = 0; i < cur_ind; i++) {
		if (i != cur_ind - 1) {
			if (cellinfo[i].end_pos == cellinfo[i + 1].start_pos)
				continue;
		}

		start_pos = cellinfo[start_ind].start_pos;

		debug_log_output("Joining cells %d thru %d:\n", start_ind + 1, i + 1);
		while (cellinfo[i].end_pos != 0) {
			size = 0;
			start_off = 0;
			for (j = 0; j < dummy_file.file_num; j++) {
				debug_log_output("compare %lld with %lld\n",
								 (long long)start_pos,
								 size + dummy_file.file[j].size);

				if (start_pos < size + dummy_file.file[j].size) {
					strcpy(joint_file_info_p->file[join_ind].name, filename);
					joint_file_info_p->file[join_ind].name[strlen(filename) -
														   5] = '1' + j;
					start_off = start_pos - size;
					joint_file_info_p->file[join_ind].start_pos = start_off;

					if (cellinfo[i].end_pos <= size + dummy_file.file[j].size) {
						debug_log_output("end cell %d found in %s\n", i, joint_file_info_p->file[join_ind].name);
						joint_file_info_p->file[join_ind].size =
							cellinfo[i].end_pos - start_pos;
						size = cellinfo[i].end_pos - size - start_off;
						cellinfo[i].end_pos = 0;
					} else {
						debug_log_output("end cell %d NOT found in %s (%lld <= %lld)\n", i, joint_file_info_p->file[join_ind].name, cellinfo[i].end_pos, size + dummy_file.file[j].size, cellinfo[i].end_pos <= size + dummy_file.file[j].size);
						joint_file_info_p->file[join_ind].size =
							dummy_file.file[j].size - start_off;
						size = joint_file_info_p->file[join_ind].size;
					}

					debug_log_output
						("%d: got a match, file %s, startoff %lld, size %lld, startpos %lld, end_pos %lld\n",
						 i, joint_file_info_p->file[join_ind].name, start_off,
						 size, start_pos, cellinfo[i].end_pos);
					break;
				}

				size += dummy_file.file[j].size;
			}

			if (j == dummy_file.file_num) {
				debug_log_output("start_pos not found\n");
				// just finish out this file and hope for the best!
				cellinfo[i].end_pos = 0;
				break;
			}

			debug_log_output("   Contiguous sectors %lld - %lld, %lld\n",
							 start_pos / 2048LL,
							 (start_pos +
							  joint_file_info_p->file[join_ind].size) /
							 2048LL, joint_file_info_p->file[join_ind].size);

			start_pos += size;

			*joint_content_length += size;
			debug_log_output("        total size now %lld\n",
							 *joint_content_length);

			join_ind++;
		}

		start_ind = i + 1;
	}

	joint_file_info_p->file_num = join_ind;
	joint_file_info_p->total_size = *joint_content_length;
	debug_log_output("total size %lld, number of chunks %d\n",
					 *joint_content_length, join_ind);

	for (i = 0; i < joint_file_info_p->file_num; i++) {
		if (joint_file_info_p->file[i].start_pos == 0)
			debug_log_output(" \n");

		debug_log_output("[%d]: start %lld, size %lld, (%lld, %lld)\n",
						 i,
						 joint_file_info_p->file[i].start_pos,
						 joint_file_info_p->file[i].size,
						 joint_file_info_p->file[i].start_pos / 2048LL,
						 (joint_file_info_p->file[i].start_pos +
						  joint_file_info_p->file[i].size) / 2048LL);
		debug_log_output("    filename %s\n",
						 joint_file_info_p->file[i].name);
	}
}


// **************************************************************************
// JOINT�ե������Ϣ�뤷���ֿ����롣
// **************************************************************************
int http_vob_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p  )
{
	int	send_header_data_len;
	int	result_len;

	unsigned char	send_http_header_buf[2048];
	unsigned char	work_buf[1024];

	off_t		joint_content_length = 0;
	off_t		written = 0;
	off_t		offset = 0;
	
	JOINT_FILE_INFO_T joint_file_info;
	dvd_reader_t	*dvd = NULL;
	ifo_handle_t	*vmg_file = NULL;
	tt_srpt_t	*tt_srpt;
	ifo_handle_t	*vts_file = NULL;
	int		titleid = http_recv_info_p->title - 1;
	int	    chapid = http_recv_info_p->page;
	char	*ptr;
    int		vts;
	int		split_vob = 0;
	ssize_t blocks;

	debug_log_output("---------------------------------------------------");
	debug_log_output("http_vob_file_response() start, titleid %d, chapid %d, page %d, name %s\n" , titleid, chapid, http_recv_info_p->page, http_recv_info_p->send_filename);

	debug_log_output("dvdopt is %s\n", http_recv_info_p->dvdopt);
	if (strlen(http_recv_info_p->dvdopt) > 0) {
		if (strcasecmp(http_recv_info_p->dvdopt ,"splitchapters") == 0 ) {
			global_param.flag_split_vob_chapters = 1;
			split_vob = 1;
		} else if (strcasecmp(http_recv_info_p->dvdopt ,"nosplitchapters") == 0 )
			global_param.flag_split_vob_chapters = 0;
		else if (strcasecmp(http_recv_info_p->dvdopt ,"notitlesplit") == 0 )
			global_param.flag_split_vob_chapters = 2;
	} else if (global_param.flag_split_vob_chapters)
		split_vob = 1;

	debug_log_output("range_start_pos is %lld\n",
					 http_recv_info_p->range_start_pos);

	// -----------------------
	// ������ѿ������
	// -----------------------
	memset(send_http_header_buf, '\0', sizeof(send_http_header_buf) );
	memset(&joint_file_info, '\0', sizeof(JOINT_FILE_INFO_T));

	/*
	 * If requesting a chapter number, check the IFO file for chapter start position
	 */
	if(titleid >= 0)
	{
		debug_log_output("title %d, chapter %d, split %d\n", titleid, chapid, split_vob);

		if(http_recv_info_p->file_type == ISO_TYPE) {
			dvd = DVDOpen( http_recv_info_p->send_filename );
			debug_log_output("Check for DVD in '%s' returned %u\n", http_recv_info_p->send_filename, dvd);
			vts = -1;
		} else {
			// Get the title set number
			ptr = strstr(http_recv_info_p->send_filename, "VTS_");

			// Should always be uppercase, but just in case...
			if (ptr == NULL)
				ptr = strstr(http_recv_info_p->send_filename, "vts_");
			
			if (ptr == NULL)
				dvd = 0;
			else {
				vts = atoi(ptr + 4);

				// Get the DVD root directory		
				strncpy(work_buf, http_recv_info_p->send_filename,
						sizeof(work_buf));
				cut_after_last_character(work_buf, '/');
				cut_after_last_character(work_buf, '/');

				debug_log_output("Checking for DVD in '%s'\n", work_buf);

				dvd = DVDOpen( work_buf );
			}
		}

		if (dvd) {
			/* open video manager file */
			vmg_file = ifoOpen( dvd, 0 );

			if( vmg_file ) {
				debug_log_output
					("Found DVD in '%s', looking for VTS=%d, Chapter=%d\n",
					work_buf, titleid, chapid);

				if (http_recv_info_p->file_type == ISO_TYPE) {
					tt_srpt = vmg_file->tt_srpt;
					vts = tt_srpt->title[titleid].title_set_nr;
				}

				/* open the appropriate video title set */
				vts_file = ifoOpen(dvd, vts);
	
				if (vts_file) {
					/*
					 * Determine which program chain we want to watch.
					 * This is based on the chapter number.
					 */
					tt_srpt = vmg_file->tt_srpt;

					/* make sure chapter is in this title set */
					if (chapid < tt_srpt->title[titleid].nr_of_ptts) {
						debug_log_output
							("Found vts=%d, chapter=%d in title %d\n",
							 vts, chapid, titleid);

						http_set_cells_to_pseudo_files(&joint_file_info,
													   &offset,
													   &joint_content_length,
													   titleid,
													   chapid,
													   vmg_file,
													   vts_file,
													   http_recv_info_p->send_filename);

						if (joint_content_length >= http_recv_info_p->range_start_pos)
							joint_content_length -= http_recv_info_p->range_start_pos;
						else
							/* should not happen, read whole disk */
							http_recv_info_p->range_start_pos = 0;

						debug_log_output
							("Real Adjusted content length: %llu\n",
							 joint_content_length);

						if((http_recv_info_p->file_type == ISO_TYPE) || (global_param.flag_always_use_dvdopen)) {
							joint_file_info.dvd_file = DVDOpenFile(dvd, tt_srpt->title[ titleid ].title_set_nr, DVD_READ_TITLE_VOBS);
							if(joint_file_info.dvd_file != NULL) {
								debug_log_output("Successfully opened selection as a DVD");
								blocks = DVDFileSize(joint_file_info.dvd_file);
	
								// Override the joint file info with the DVD info
								joint_file_info.total_size = (u_int64_t)blocks;
								joint_file_info.total_size *= 2048;							
	
								strncpy( joint_file_info.file[0].name, http_recv_info_p->send_filename, sizeof(joint_file_info.file[0].name));
								joint_file_info.file[0].size = joint_file_info.total_size;
								joint_file_info.file_num = 1;

								// ��������contnet_length �׻�
								if ( http_recv_info_p->range_end_pos > 0 )
								{
									joint_content_length = (http_recv_info_p->range_end_pos - http_recv_info_p->range_start_pos) + 1;
								}
								else
								{
									joint_content_length = joint_file_info.total_size - http_recv_info_p->range_start_pos;
								}
							}
						}
					} else {
						debug_log_output
							("Invalid chapter=%d, title %d only has %d chapters\n",
							 chapid, titleid,
							 tt_srpt->title[titleid].nr_of_ptts);
					}

					ifoClose( vts_file );
				} else
					debug_log_output("No title found for vts=%d\n", vts);

				ifoClose( vmg_file );
			}
		}
	} else {
		(void)analyze_vob_file(http_recv_info_p->send_filename, &joint_file_info);
		joint_content_length = joint_file_info.total_size;
		debug_log_output("opening %s, size %lld, filenum %d, chapid %d, page %d\n", http_recv_info_p->send_filename, joint_file_info.total_size, joint_file_info.file_num, chapid, http_recv_info_p->page);
# if 0
		offset = (off_t) ((0.1 * http_recv_info_p->page) * joint_content_length);
		offset = (offset - (offset % 2048));
		http_recv_info_p->range_start_pos = offset;
# endif
		//joint_content_length -= http_recv_info_p->range_start_pos;
		debug_log_output("start is %lld, len %lld\n", http_recv_info_p->range_start_pos, joint_content_length);
		//chapid = 0;
	}

	if (!dvd && (chapid > 0)) {
		// Handle the as a plain file with 10% intervals
		offset = (joint_file_info.total_size/10) * chapid;
		debug_log_output("Handling VOB as MPEG2 with chapters at 10%% intervals, setting offset=%lld\n", offset);
	}

	if (global_param.bookmark_threshold
		&& (joint_file_info.total_size > global_param.bookmark_threshold)
		&& (http_recv_info_p->range_start_pos == 0)) {
		// Check for a bookmark
		FILE	*fp;
		int previous_page = 0;
		off_t previous_size = 0;
		off_t bookmark = 0;
		snprintf(work_buf, sizeof(work_buf), "%s.wizd.bookmark", 
			http_recv_info_p->send_filename);
		debug_log_output("Checking for bookmark: '%s'", work_buf);
		fp = fopen(work_buf, "r");
		if(fp != NULL) {
			fgets(work_buf, sizeof(work_buf), fp);
			bookmark = atoll(work_buf);
			if(fgets(work_buf, sizeof(work_buf), fp) != NULL) {
				previous_size = atoll(work_buf);
				if(fgets(work_buf, sizeof(work_buf), fp) != NULL) {
					previous_page = atoi(work_buf)%1000;
				}
			}
			fclose(fp);
			debug_log_output("Bookmark offset: %lld/%lld (page %d)", bookmark, previous_size, previous_page);
			debug_log_output("Requested range %lld-%lld (page %d)", http_recv_info_p->range_start_pos, http_recv_info_p->range_end_pos, chapid);
		}

		// Compare the current request with the stored bookmark
		// If requesting the same chapter as before, then don't adjust anything
		// If reqesting an earlier chapter, then go there directly
		if(chapid < previous_page) {
			// Make sure we never advance the position on a previous-chapter command
			if((bookmark > 0) && (offset > bookmark)) {
				debug_log_output("Forcing offset from %lld to %lld to prevent advance when going to previous chapter", offset, bookmark);
				offset = bookmark;
			} else {
				debug_log_output("Going to previous chapter - no bookmark adjustment needed");
			}
		} else if(chapid > previous_page) {
			// If the file size is increasing, and bookmark is equal to the previous end-of-file
			// then adjust things so it continues playing where it left off
			// This will allow for (almost) continuous playback of files of increasing size
			if((previous_size > 0) && (joint_file_info.total_size > previous_size) && (offset < bookmark)) {
				debug_log_output("Forcing offset from %lld to %lld to get continuous playback", offset, bookmark);
				offset = bookmark;
			} else {
				// If requesting a following chapter, then return content_length=1 until we are beyond the bookmark location
				// (Note: I tried content_length=0, but this seems to really confuse the LinkPlayer!!!)
				if((global_param.dummy_chapter_length > 0) && (offset < bookmark)) {
					joint_content_length = global_param.dummy_chapter_length;
					debug_log_output("Returning length=%lld because offset < bookmark (%lld < %lld)",
						joint_content_length, offset, bookmark);
					http_recv_info_p->range_start_pos = offset;
					http_recv_info_p->range_end_pos = offset + joint_content_length - 1;
					offset=0;

					// I also tried sending a not-found, but this confuses it too!!!
					/*
					snprintf(send_http_header_buf, sizeof(send_http_header_buf),
						HTTP_NOT_FOUND
						HTTP_CONNECTION
						HTTP_CONTENT_TYPE
						HTTP_END, "video/mpeg" );
					send_header_data_len = strlen(send_http_header_buf);
					debug_log_output("send_header_data_len = %d\n", send_header_data_len);
					debug_log_output("--------\n");
					debug_log_output("%s", send_http_header_buf);
					debug_log_output("--------\n");
					result_len = send(accept_socket, send_http_header_buf, send_header_data_len, 0);
					debug_log_output("result_len=%d, send_data_len=%d\n", result_len, send_header_data_len);
					*/
					// I also tried simply closing the socket and exiting, but this confuses it too!!!
					//close(accept_socket);
					//return 0;
				} else if(offset > bookmark) {
					debug_log_output("Advancing to chapter %d, offset=%lld, bookmark=%lld", chapid, offset, bookmark);
				}
			}
		} else if(!dvd && (previous_size > 0)) {
			// When requesting the same chapter, then compute the offset using the previously saved length
			// so that fast-forward/rewind isn't affected by changing file sizes
			// But only do this if not using DVD chapter seeking, since we are assuming 10% increments here
			offset = (previous_size/10) * chapid;
			debug_log_output("Forcing offset=%lld based on previous size (%lld) instead of current size (%lld)",
				offset, previous_size, joint_file_info.total_size);
		}
	}

	if(offset > 0) {
		debug_log_output("Offsetting start/end position by %lld", offset);
		http_recv_info_p->range_start_pos += offset;
		if(http_recv_info_p->range_end_pos > 0)
			http_recv_info_p->range_end_pos += offset;
		debug_log_output("New start pos = %lld\n", http_recv_info_p->range_start_pos);
		debug_log_output("New end pos   = %lld\n", http_recv_info_p->range_end_pos);
	}
	if ( http_recv_info_p->range_end_pos > 0 )	// end���ֻ���ͭ�ꡣ
	{
		joint_content_length = (http_recv_info_p->range_end_pos - http_recv_info_p->range_start_pos) + 1;
	} 
	else 
	{
		joint_content_length = joint_file_info.total_size - http_recv_info_p->range_start_pos;
	}
	debug_log_output("joint_content_length = %lld\n", joint_content_length);

	// -------------------------
	// HTTP_OK �إå�����
	// -------------------------
	strncpy(send_http_header_buf, HTTP_OK, sizeof(send_http_header_buf));
	strncat(send_http_header_buf, HTTP_CONNECTION,
			sizeof(send_http_header_buf) - strlen(send_http_header_buf) );

	snprintf(work_buf, sizeof(work_buf), HTTP_SERVER_NAME, SERVER_NAME);
	strncat(send_http_header_buf, work_buf,
			sizeof(send_http_header_buf) - strlen(send_http_header_buf) );

	snprintf(work_buf, sizeof(work_buf), HTTP_CONTENT_LENGTH,
			 joint_content_length);
	strncat(send_http_header_buf, work_buf,
			sizeof(send_http_header_buf) - strlen(send_http_header_buf) );

	snprintf(work_buf, sizeof(work_buf), HTTP_CONTENT_TYPE,
			 http_recv_info_p->mime_type);
	strncat(send_http_header_buf, work_buf,
			sizeof(send_http_header_buf) - strlen(send_http_header_buf) );
	strncat(send_http_header_buf, HTTP_END,
			sizeof(send_http_header_buf) - strlen(send_http_header_buf) );


	send_header_data_len = strlen(send_http_header_buf);
	debug_log_output("send_header_data_len = %d\n", send_header_data_len);
	debug_log_output("--------\n");
	debug_log_output("%s", send_http_header_buf);
	debug_log_output("--------\n");

	// --------------
	// �إå��ֿ�
	// --------------
	result_len =
		send(accept_socket, send_http_header_buf, send_header_data_len, 0);
	debug_log_output("result_len=%d, send_header_data_len=%d\n", result_len,
					 send_header_data_len);

	// --------------
	// �����ֿ�
	// --------------
	if (No_Lib_Dvd_Css || !global_param.flag_dvdcss_lib || !dvd)
		written = http_vob_file_send(accept_socket, &joint_file_info, 
									 joint_content_length,
									 http_recv_info_p->range_start_pos);
	else
	{
		strncpy(work_buf, http_recv_info_p->send_filename,
				sizeof(work_buf));
		cut_after_last_character(work_buf, '/');
		cut_after_last_character(work_buf, '/');
		written = send_dvd_test(accept_socket, work_buf, titleid, chapid, http_recv_info_p->range_start_pos);
	}

	if (global_param.bookmark_threshold) {
		/*
		if(   (written < global_param.bookmark_threshold)
		   || (joint_content_length && ((written+global_param.bookmark_threshold) > joint_content_length))) {
			// Less than 10 MB written, or within 10 MB of the end of file
			// Remove any existing bookmark
			snprintf(work_buf, sizeof(work_buf), "%s.wizd.bookmark", http_recv_info_p->send_filename);
			if(0 == unlink(work_buf)) {
				debug_log_output("Removed bookmark: '%s'", work_buf);
			}
		} else 
		*/

		if (written > global_param.bookmark_threshold) {
			// Wrote more than 10MB, save a bookmark
			snprintf(work_buf, sizeof(work_buf), "%s.wizd.bookmark", 
				http_recv_info_p->send_filename);

			debug_log_output("Bookmark status: written=%lld, range_start=%lld\n", written, http_recv_info_p->range_start_pos);

			written += http_recv_info_p->range_start_pos;
			if (written < joint_file_info.total_size)
				written -= global_param.bookmark_threshold;

			if (written > 0) {
				FILE *fp = fopen(work_buf, "w");

				if (fp != NULL) {
					fprintf(fp, "%lld\n", written);
					fprintf(fp, "%lld\n", joint_file_info.total_size);
					fprintf(fp, "%d\n", chapid);
					fclose(fp);
					debug_log_output("Wrote bookmark: '%s' page %d %lld of %lld", work_buf, chapid, written, joint_file_info.total_size);
				}
			} else {
				// Remove any old bookmarks
				if (0 == unlink(work_buf)) {
					debug_log_output("Removed bookmark: '%s'", work_buf);
				}
			}
		}
	}

	if (dvd != NULL) {
		if (joint_file_info.dvd_file != NULL)
			DVDCloseFile(joint_file_info.dvd_file);

		DVDClose( dvd );
	}

	return 0;
}





// **************************************************************************
// �ե�����μ��Τ��������롣
// **************************************************************************
static off_t http_vob_file_send(int	accept_socket, JOINT_FILE_INFO_T *joint_file_info_p, off_t content_length, off_t range_start_pos )
{
	int fd;
	off_t			lseek_ret;
	off_t			start_file_pos;
	off_t			written = 0;
	int				i;

	debug_log_output("---------------------------------------------------");
	debug_log_output("http_vob_file_send() start" );

	debug_log_output("range_start_pos=%lld\n", range_start_pos);
	debug_log_output("content_length=%lld\n", content_length);


	// -------------------------
	// �������Ȱ��֤�׻�
	// -------------------------
	debug_log_output("calc start pos.");
	joint_file_info_p->current_file_num = 0;
	start_file_pos = 0;

	// printf("range start %lld\n", range_start_pos);
	// print_joint_files(joint_file_info_p);

	/* find our start position */
	for (i = 0; i < joint_file_info_p->file_num; i++) {
		if (range_start_pos < joint_file_info_p->file[i].size) {
			joint_file_info_p->file[i].size -= range_start_pos;
			break;
		}

		range_start_pos -= joint_file_info_p->file[i].size;
		joint_file_info_p->current_file_num++;
	}

	//printf("[%d]: start_file_pos %lld, range_start %lld, seek %lld, size %lld\n",
	//		joint_file_info_p->current_file_num,
	//		joint_file_info_p->file[joint_file_info_p->current_file_num].start_pos,
	//		range_start_pos,
	//		joint_file_info_p->file[joint_file_info_p->current_file_num].start_pos + range_start_pos,
	//		joint_file_info_p->file[joint_file_info_p->current_file_num].size);

	// Small files go to simple function
	if((content_length > 0) && (content_length <= SEND_BUFFER_SIZE)) {
		debug_log_output("using http_simple_file_send since length=%lld", content_length);
		written = http_simple_file_send( accept_socket, 
			joint_file_info_p->file[joint_file_info_p->current_file_num].name,
			content_length,
			start_file_pos );
	} else if (content_length > 0) {
		// ------------------------------
		// �ǽ�Υե�����򥪡��ץ�
		// ------------------------------
		if(joint_file_info_p->dvd_file == NULL) {
			fd = open(joint_file_info_p->file[joint_file_info_p->current_file_num].name , O_RDONLY);
			if ( fd < 0 )
			{	
				debug_log_output("open() error. '%s'", joint_file_info_p->file[joint_file_info_p->current_file_num].name);
				return ( -1 );
			}

			// ------------------------------------------------
			// �������Ȱ���(range_start_pos)�إե����륷����
			// ------------------------------------------------
			lseek_ret = lseek(fd, joint_file_info_p->file[joint_file_info_p->current_file_num].start_pos + range_start_pos, SEEK_SET);
			if ( lseek_ret < 0 )	// lseek ���顼�����å�
			{
				debug_log_output("lseek() error.");
				close(fd);
				return ( -1 );
			}
		} else {
			// ISO image seek
			/* joint_file_info_p->iso_seek = range_start_pos/2048; */
			joint_file_info_p->iso_seek = (joint_file_info_p->file[joint_file_info_p->current_file_num].start_pos + range_start_pos) / 2048;
			fd=0; // Not used
		}

		// print_joint_files(joint_file_info_p);
		written = copy_descriptors(fd, accept_socket, content_length, joint_file_info_p);

		// Note: can't close fd because it may change inside copy_descriptors
	}
	// ���ｪλ
	return written;
}





// **************************************************************************
// vob�ե��������Ϥ��ơ�joint_file_info_p ������롣
// **************************************************************************
int analyze_vob_file(unsigned char *vob_filename, JOINT_FILE_INFO_T *joint_file_info_p )
{
	unsigned char	read_filename[SVI_FILENAME_LENGTH+10];

	unsigned char	vob_filepath[WIZD_FILENAME_MAX];

	unsigned char	first_filename[WIZD_FILENAME_MAX];
	unsigned char	series_filename[WIZD_FILENAME_MAX];

	int			ret;
	int			i;
	int			len;

	char		*p;

	struct stat file_stat;


	debug_log_output("analyze_vob_file() start.");
	debug_log_output("vob_filename='%s'", vob_filename);

	// ------------------
	// ��������
	// ------------------
	memset(read_filename, '\0', sizeof(read_filename));
	memset(first_filename, '\0', sizeof(first_filename));



	// ----------------------------------------
	// vob�Υե�����͡����GET���Ƥ��� /VIDEO_TS/VTS_01_?.VOB
	// ----------------------------------------
	strncpy(vob_filepath, vob_filename, sizeof(vob_filepath));

	debug_log_output("vob_filepath = '%s'", vob_filepath);

	strncpy(first_filename, vob_filepath, sizeof(vob_filepath));

	debug_log_output("first_filename = '%s'", first_filename);

	// ----------------------------------------------------
	// vob�����ɤ���ե�����Ρ��ե���������Get
	// ----------------------------------------------------
	ret = stat(first_filename, &file_stat);
	if ( ret != 0 )
	{
		debug_log_output("'%s' Not found.", first_filename);
		return ( -1 );
	}

	// --------------------------------------------------------
	// 1���ܤΥե���������joint_file_info_p �˥��å�
	// --------------------------------------------------------
	memset(joint_file_info_p, 0, sizeof(JOINT_FILE_INFO_T));

	strncpy( joint_file_info_p->file[0].name, first_filename, sizeof(joint_file_info_p->file[0].name));
	joint_file_info_p->file[0].size 	= file_stat.st_size; 
	joint_file_info_p->total_size 		= file_stat.st_size;
	joint_file_info_p->file_num = 1;


	// --------------------------------------------------------
	// 2���ܰʹߤΥե���������Ĵ�٤�joint_file_info_p �˥��å�
	// --------------------------------------------------------
	strncpy( series_filename, first_filename, sizeof(series_filename));
	p = strrchr(series_filename, '.');
	if (!p)
		return(-1);
	p++;
	if (strcasecmp(p, "vob") == 0)
		len = strlen(series_filename)-5;
	else if ((strcasecmp(p, "ts") == 0) || (strcasecmp(p, "tp") == 0))
		len = strlen(series_filename)-4;
	else
		return(-1);

	for ( i=1; i<JOINT_MAX; i++ )
	{
		// �ե�����̾����
		// /VTS_01_1.VOB �� VTS_01_2.VOB
		if((series_filename[len]=='9') && (len>0)) {
			series_filename[len]='0';
			series_filename[len-1]++;
		} else {
			series_filename[len]++;
		}
		// �ե���������GET
		ret = stat(series_filename, &file_stat);
		if ( ret != 0 ) // �ե�����̵����������λ��
		{
			debug_log_output("'%s' Not found.", series_filename);
			break;
		}

		debug_log_output("[%d]_filename = '%s'", i+1, series_filename);

		// GET�����ե���������joint_file_info_p �˥��å�
		strncpy( joint_file_info_p->file[i].name, series_filename, sizeof(joint_file_info_p->file[0].name));
		joint_file_info_p->file[i].size 	= file_stat.st_size; 
		joint_file_info_p->total_size 		+= file_stat.st_size;
		joint_file_info_p->file_num++;

	}

	joint_file_info_p->current_file_num = 0;
	debug_log_output("analyze_vob_file() end.");

	return 0;
}

# ifndef HAVE_LIB_DVDCSS
int
send_dvd_test(int accept_socket, char *name, int titleid, int chapid, off_t start)
{
	return(-1);
}
# else
/**
 * Returns true if the pack is a NAV pack.  This check is clearly insufficient,
 * and sometimes we incorrectly think that valid other packs are NAV packs.  I
 * need to make this stronger.
 */
int is_nav_pack( unsigned char *buffer )
{
    return ( buffer[ 41 ] == 0xbf && buffer[ 1027 ] == 0xbf );
}

/*
 * The following code will correctly handle angles, but not handle moving
 * a percentage into the dvd.  The problem occurs if the cells are not
 * contiguous.  Right now it just does nothing if you enable this file and
 * try and skip forward by X%
 *
 * this code is taken from the libdvdread play_title example.
 */
int
send_dvd_test(int accept_socket, char *name, int titleid, int chapid, off_t start)
{
	int pgc_id, len, start_cell, cur_cell;
    unsigned int cur_pack;
    int ttn, pgn, next_cell;
    dvd_reader_t *dvd;
    dvd_file_t *title;
    ifo_handle_t *vmg_file;
    tt_srpt_t *tt_srpt;
    ifo_handle_t *vts_file;
    vts_ptt_srpt_t *vts_ptt_srpt;
    pgc_t *cur_pgc;
	int angle = 0;
	int written = 0;
	off_t skip = 0;
	int c;

    /**
     * Open the disc.
     */
    dvd = DVDOpen(name);
    if( !dvd ) {
        fprintf( stderr, "Couldn't open DVD: %s\n", name);
        return -1;
    }

	// fprintf(stderr, "name %s, title %d, chapid %d, start %lld\n", name, titleid, chapid, start);


    /**
     * Load the video manager to find out the information about the titles on
     * this disc.
     */
    vmg_file = ifoOpen( dvd, 0 );
    if( !vmg_file ) {
        fprintf( stderr, "Can't open VMG info.\n" );
        DVDClose( dvd );
        return -1;
    }
    tt_srpt = vmg_file->tt_srpt;


    /**
     * Make sure our title number is valid.
     */
    // fprintf( stderr, "There are %d titles, ", tt_srpt->nr_of_srpts );
    if( titleid < 0 || titleid >= tt_srpt->nr_of_srpts ) {
        fprintf( stderr, "Invalid title %d.\n", titleid + 1 );
        ifoClose( vmg_file );
        DVDClose( dvd );
        return -1;
    }


    /**
     * Make sure the chapter number is valid for this title.
     */
    //fprintf( stderr, "title %d has %d chapters, ", titleid, tt_srpt->title[ titleid ].nr_of_ptts );

    if( chapid < 0 || chapid >= tt_srpt->title[ titleid ].nr_of_ptts ) {
        fprintf( stderr, "Invalid chapter %d\n", chapid + 1 );
        ifoClose( vmg_file );
        DVDClose( dvd );
        return -1;
    }


    /**
     * Make sure the angle number is valid for this title.
     */
    // fprintf( stderr, "and %d angles on this DVD.\n", tt_srpt->title[ titleid ].nr_of_angles );

    if( angle < 0 || angle >= tt_srpt->title[ titleid ].nr_of_angles ) {
        fprintf( stderr, "Invalid angle %d\n", angle + 1 );
        ifoClose( vmg_file );
        DVDClose( dvd );
        return -1;
    }


    /**
     * Load the VTS information for the title set our title is in.
     */
    vts_file = ifoOpen( dvd, tt_srpt->title[ titleid ].title_set_nr );
    if( !vts_file ) {
        fprintf( stderr, "Can't open the title %d info file.\n",
                 tt_srpt->title[ titleid ].title_set_nr );
        ifoClose( vmg_file );
        DVDClose( dvd );
        return -1;
    }


    /**
     * Determine which program chain we want to watch.  This is based on the
     * chapter number.
     */
    ttn = tt_srpt->title[ titleid ].vts_ttn;
    vts_ptt_srpt = vts_file->vts_ptt_srpt;
    pgc_id = vts_ptt_srpt->title[ ttn - 1 ].ptt[ chapid ].pgcn;
    pgn = vts_ptt_srpt->title[ ttn - 1 ].ptt[ chapid ].pgn;
    cur_pgc = vts_file->vts_pgcit->pgci_srp[ pgc_id - 1 ].pgc;
    start_cell = cur_pgc->program_map[ pgn - 1 ] - 1;

    /**
     * We've got enough info, time to open the title set data.
     */
    title = DVDOpenFile( dvd, tt_srpt->title[ titleid ].title_set_nr,
                         DVD_READ_TITLE_VOBS );
    if( !title ) {
        fprintf( stderr, "Can't open title VOBS (VTS_%02d_1.VOB).\n",
                 tt_srpt->title[ titleid ].title_set_nr );
        ifoClose( vts_file );
        ifoClose( vmg_file );
        DVDClose( dvd );
        return -1;
    }

	// NEW
	skip = 0;
	c = start_cell;
	if (skip < start)
		printf("seeking to %lld\n", start);
	while (skip < start) {
		if (skip + (2048LL * (cur_pgc->cell_playback[c].last_sector - cur_pgc->cell_playback[c].first_sector)) > start) {
			fprintf(stderr, "starting on cell %d\n", c);
			break;
		} else
			skip += (2048LL * (cur_pgc->cell_playback[c].last_sector - cur_pgc->cell_playback[c].first_sector));

		if (c >= cur_pgc->nr_of_cells - 1)
			break;

		c++;
	}

	start_cell = c;
	// end NEW

    /**
     * Playback by cell in this pgc, starting at the cell for our chapter.
     */
    next_cell = start_cell;
    for( cur_cell = start_cell; next_cell < cur_pgc->nr_of_cells; ) {

        cur_cell = next_cell;

        /* Check if we're entering an angle block. */
        if( cur_pgc->cell_playback[ cur_cell ].block_type
                                        == BLOCK_TYPE_ANGLE_BLOCK ) {
            int i;

            cur_cell += angle;
            for( i = 0;; ++i ) {
                if( cur_pgc->cell_playback[ cur_cell + i ].block_mode
                                          == BLOCK_MODE_LAST_CELL ) {
                    next_cell = cur_cell + i + 1;
                    break;
                }
            }
        } else {
            next_cell = cur_cell + 1;
        }


        /**
         * We loop until we're out of this cell.
         */
        for( cur_pack = cur_pgc->cell_playback[ cur_cell ].first_sector;
             cur_pack < cur_pgc->cell_playback[ cur_cell ].last_sector; ) {

            dsi_t dsi_pack;
            unsigned int next_vobu, next_ilvu_start, cur_output_size;


            /**
             * Read NAV packet.
             */
            len = DVDReadBlocks( title, (int) cur_pack, 1, data );
            if( len != 1 ) {
                fprintf( stderr, "Read failed for block %d\n", cur_pack );
                ifoClose( vts_file );
                ifoClose( vmg_file );
                DVDCloseFile( title );
                DVDClose( dvd );
                return -1;
            }
            if( !is_nav_pack( data ) ) {
				// fprintf(stderr, "missed nav packet\n");
				cur_pack++;
				continue;
			}


            /**
             * Parse the contained dsi packet.
             */
            navRead_DSI( &dsi_pack, &(data[ DSI_START_BYTE ]) );
            if( cur_pack != dsi_pack.dsi_gi.nv_pck_lbn ) {
				// fprintf(stderr, "bad curpack\n");
			}


            /**
             * Determine where we go next.  These values are the ones we mostly
             * care about.
             */
            next_ilvu_start = cur_pack
                              + dsi_pack.sml_agli.data[ angle ].address;
            cur_output_size = dsi_pack.dsi_gi.vobu_ea;


            /**
             * If we're not at the end of this cell, we can determine the next
             * VOBU to display using the VOBU_SRI information section of the
             * DSI.  Using this value correctly follows the current angle,
             * avoiding the doubled scenes in The Matrix, and makes our life
             * really happy.
             *
             * Otherwise, we set our next address past the end of this cell to
             * force the code above to go to the next cell in the program.
             */
            if( dsi_pack.vobu_sri.next_vobu != SRI_END_OF_CELL ) {
                next_vobu = cur_pack
                            + ( dsi_pack.vobu_sri.next_vobu & 0x7fffffff );
            } else {
                next_vobu = cur_pack + cur_output_size + 1;
            }

            if( cur_output_size >= 1024 ) {
				fprintf(stderr, "bad output size %d\n", cur_output_size);
			}
            cur_pack++;

            /**
             * Read in and output cursize packs.
             */
            len = DVDReadBlocks( title, (int)cur_pack, cur_output_size, data );
            if( len != (int) cur_output_size ) {
                fprintf( stderr, "Read failed for %d blocks at %d\n",
                         cur_output_size, cur_pack );
                ifoClose( vts_file );
                ifoClose( vmg_file );
                DVDCloseFile( title );
                DVDClose( dvd );
                return -1;
            }

			len = send(accept_socket, data, cur_output_size * DVD_VIDEO_LB_LEN, 0);
			if (len != cur_output_size * DVD_VIDEO_LB_LEN) {
				goto done;
			}
			written += len;
			cur_pack = next_vobu;
		}
	}

done:
    ifoClose( vts_file );
    ifoClose( vmg_file );
    DVDCloseFile( title );
    DVDClose( dvd );
    DVDFinish();
    return written;
}
# endif
