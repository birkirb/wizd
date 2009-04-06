/*
 * Copyright (C) 2001 Billy Biggs <vektor@dumbterm.net>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>


int main( int argc, char **argv )
{
	int titleid, chapid, pgc_id, len, start_cell, cur_cell;
	unsigned int cur_pack;
	int angle, ttn, pgn, next_cell;
	unsigned char data[ DVD_VIDEO_LB_LEN ];
	dvd_reader_t *dvd;
	dvd_file_t *title;
	ifo_handle_t *vmg_file;
	tt_srpt_t *tt_srpt;
	ifo_handle_t *vts_file;
	vts_ptt_srpt_t *vts_ptt_srpt;
	pgc_t *cur_pgc;
	unsigned long count = 0;
	unsigned long long content_length = 0, tmplen;
	unsigned long long cur_length = 0;
	unsigned long long range_start = 0;
	unsigned long long range_end = 0;
	unsigned long long sectors = 0;


	/**
	 * Usage.
	 */
	if( argc < 5 ) {
		fprintf( stderr, "Usage: %s <dvd path> <title number> "
						 "<start chapter> <angle> [start pos] [end pos]\n\n"
						 "Title Number  [1-X]: Movie Title on Disc.\n"
						 "Start Chapter [1-X]: Chapter to start at.\n"
						 "Angle Number  [1-X]: Which angle to show.\n\n"
						 "This app outputs an MPEG2 system stream.\n"
						 "Make sure you redirect the output somewhere.\n\n",
				 argv[ 0 ], argv[ 0 ] );
		return -1;
	}

	titleid = atoi( argv[ 2 ] ) - 1;
	chapid = atoi( argv[ 3 ] ) - 1;
	angle = atoi( argv[ 4 ] ) - 1;
	if (argc > 5) {
		range_start = strtoull( argv[ 5 ], NULL, 0 );
	}
	if (argc > 6) {
		range_end = strtoull( argv[ 6 ], NULL, 0 );
		if (range_end < range_start) {
			fprintf(stderr, "range error\n");
			exit( -1 );
		}
	}


	/**
	 * Open the disc.
	 */
	dvd = DVDOpen( argv[ 1 ] );
	if( !dvd ) {
		fprintf( stderr, "Couldn't open DVD: %s\n", argv[ 1 ] );
		return -1;
	}


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
	fprintf( stderr, "There are %d titles on this DVD.\n",
			 tt_srpt->nr_of_srpts );
	if( titleid < 0 || titleid >= tt_srpt->nr_of_srpts ) {
		fprintf( stderr, "Invalid title %d.\n", titleid + 1 );
		ifoClose( vmg_file );
		DVDClose( dvd );
		return -1;
	}


	/**
	 * Make sure the chapter number is valid for this title.
	 */
	fprintf( stderr, "There are %d chapters in this title.\n",
			 tt_srpt->title[ titleid ].nr_of_ptts );

	if( chapid < 0 || chapid >= tt_srpt->title[ titleid ].nr_of_ptts ) {
		fprintf( stderr, "Invalid chapter %d\n", chapid + 1 );
		ifoClose( vmg_file );
		DVDClose( dvd );
		return -1;
	}


	/**
	 * Make sure the angle number is valid for this title.
	 */
	fprintf( stderr, "There are %d angles in this title.\n",
			 tt_srpt->title[ titleid ].nr_of_angles );
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
		count += (cur_pgc->cell_playback[ cur_cell ].last_sector - cur_pgc->cell_playback[ cur_cell ].first_sector);
	}

	content_length = (long long)DVD_VIDEO_LB_LEN * (long long)count;
	if (range_end != 0 && content_length > range_end) {
		content_length = range_end;
	}

	content_length -= range_start;
	fprintf(stdout, "Content-Type: video/mpeg\r\n");
	fprintf(stdout, "Content-Length: %llu\r\n", content_length);
	fprintf(stdout, "\r\n", content_length);
	fflush(stdout);



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

		sectors = cur_pgc->cell_playback[ cur_cell ].last_sector
				- cur_pgc->cell_playback[ cur_cell ].first_sector + 1;
		if (cur_length + DVD_VIDEO_LB_LEN*sectors < range_start) {
			cur_length += DVD_VIDEO_LB_LEN*sectors;
			continue;
		}

		/**
		 * We loop until we're out of this cell.
		 */
		for( cur_pack = cur_pgc->cell_playback[ cur_cell ].first_sector;
			 cur_pack <= cur_pgc->cell_playback[ cur_cell ].last_sector;
			 cur_pack++ ) {

			cur_length += DVD_VIDEO_LB_LEN;
			if (cur_length < range_start) continue;

			/**
			 * Read in and output cursize packs.
			 */
			len = DVDReadBlocks( title, (int)cur_pack, 1, data );
			if( len != 1 ) {
				fprintf( stderr, "Read failed for %d blocks at %d\n",
						 1, cur_pack );
				ifoClose( vts_file );
				ifoClose( vmg_file );
				DVDCloseFile( title );
				DVDClose( dvd );
				return -1;
			}

		//	fprintf(stderr, "%llu / %llu\n", cur_length, range_start);
			if (cur_length - range_start > DVD_VIDEO_LB_LEN) {
				fwrite( data, 1, DVD_VIDEO_LB_LEN, stdout );
			} else {
				fwrite( data + (DVD_VIDEO_LB_LEN - (cur_length - range_start))
					, 1, cur_length - range_start, stdout);
			}
			fflush(stdout);
			range_start = 0;
		}
	}

	ifoClose( vts_file );
	ifoClose( vmg_file );
	DVDCloseFile( title );
	DVDClose( dvd );
	return 0;
}

