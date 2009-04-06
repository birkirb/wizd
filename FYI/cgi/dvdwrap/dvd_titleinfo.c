/*
 * Copyright (C) 2001 Billy Biggs <vektor@dumbterm.net>,
 *                    H�kan Hjort <d95hjort@dtek.chalmers.se>
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

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>

int main( int argc, char **argv )
{
	dvd_reader_t *dvd;
	ifo_handle_t *ifo_file;
	tt_srpt_t *tt_srpt;
	int i, j;

	/**
	 * Usage.
	 */
	if( argc != 2 ) {
		fprintf( stderr, "Usage: %s <dvd path>\n", argv[ 0 ] );
		return -1;
	}

	dvd = DVDOpen( argv[ 1 ] );
	if( !dvd ) {
		fprintf( stderr, "Couldn't open DVD: %s\n", argv[ 1 ] );
		return -1;
	}

	ifo_file = ifoOpen( dvd, 0 );
	if( !ifo_file ) {
		fprintf( stderr, "Can't open VMG info.\n" );
		DVDClose( dvd );
		return -1;
	}
	tt_srpt = ifo_file->tt_srpt;

	printf( "There are %d titles.\n", tt_srpt->nr_of_srpts );
	for( i = 0; i < tt_srpt->nr_of_srpts; ++i ) {
		ifo_handle_t *vts_file;
		vts_ptt_srpt_t *vts_ptt_srpt;
		pgc_t *cur_pgc;
		int vtsnum, ttnnum, pgcnum, chapts;

		vtsnum = tt_srpt->title[ i ].title_set_nr;
		ttnnum = tt_srpt->title[ i ].vts_ttn;
		chapts = tt_srpt->title[ i ].nr_of_ptts;

		printf( "\nTitle %d:\n", i + 1 );
		printf( "\tIn VTS: %d [TTN %d]\n", vtsnum, ttnnum );
		printf( "\n" );
		printf( "\tTitle has %d chapters and %d angles\n", chapts,
				tt_srpt->title[ i ].nr_of_angles );

	}

	ifoClose( ifo_file );
	DVDClose( dvd );
	return 0;
}

