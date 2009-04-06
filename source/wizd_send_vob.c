// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_send_vob.c
//											$Revision: 1.3 $
//											$Date: 2004/10/10 07:26:56 $
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


#include "wizd.h"

static int http_vob_file_send(int	accept_socket, JOINT_FILE_INFO_T *joint_file_info_p, off_t content_length, off_t range_start_pos );




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

	JOINT_FILE_INFO_T *joint_file_info_p;

	int	ret;
	int	i;


	debug_log_output("---------------------------------------------------");
	debug_log_output("http_vob_file_response() start" );

	// -----------------------
	// ������ѿ������
	// -----------------------
	memset(send_http_header_buf, '\0', sizeof(send_http_header_buf) );

	// -----------------------
	// ����ΰ����
	// -----------------------
	joint_file_info_p = malloc( sizeof(JOINT_FILE_INFO_T) );
	if ( joint_file_info_p == NULL )
	{
		debug_log_output("malloc() error.");
		return ( -1 );
	}


	// --------------------------
	// vob�ե���������GET
	// --------------------------
	ret = analyze_vob_file(http_recv_info_p->send_filename, joint_file_info_p );
	debug_log_output("http_vob_file_response() end (ret=%d)" );
	if ( ret >= 0 )
	{
		debug_log_output("joint_file_info.file_num = %d", joint_file_info_p->file_num);
		debug_log_output("joint_file_info.total_size = %d", joint_file_info_p->total_size);
		for ( i = 0; i< joint_file_info_p->file_num; i++ )
		{
			debug_log_output("[%02d] '%s' %d\n", i, joint_file_info_p->file[i].name, joint_file_info_p->file[i].size );
		}

		// ��������contnet_length �׻�
		if ( http_recv_info_p->range_end_pos > 0 )
		{
			joint_content_length = (http_recv_info_p->range_end_pos - http_recv_info_p->range_start_pos) + 1;
		}
		else
		{
			joint_content_length = joint_file_info_p->total_size - http_recv_info_p->range_start_pos;
		}
	}


	// -------------------------
	// HTTP_OK �إå�����
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
	// �إå��ֿ�
	// --------------
	result_len = send(accept_socket, send_http_header_buf, send_header_data_len, 0);
	debug_log_output("result_len=%d, send_header_data_len=%d\n", result_len, send_header_data_len);


	// --------------
	// �����ֿ�
	// --------------
	http_vob_file_send(	accept_socket, 	joint_file_info_p, 
											joint_content_length,
											http_recv_info_p->range_start_pos );


	free( joint_file_info_p );	// Memory Free.

	return 0;
}





// **************************************************************************
// �ե�����μ��Τ��������롣
// **************************************************************************
static int http_vob_file_send(int	accept_socket, JOINT_FILE_INFO_T *joint_file_info_p, off_t content_length, off_t range_start_pos )
{
	int fd;
	off_t			lseek_ret;
	off_t			start_file_pos;
	off_t			left_pos;


	debug_log_output("---------------------------------------------------");
	debug_log_output("http_vob_file_send() start" );

	debug_log_output("range_start_pos=%d", range_start_pos);
	debug_log_output("content_length=%d", content_length);


	// -------------------------
	// �������Ȱ��֤�׻�
	// -------------------------
	debug_log_output("calc start pos.");
	joint_file_info_p->current_file_num = 0;
	start_file_pos = 0;

	left_pos = range_start_pos;
	while ( 1 ) 
	{
		debug_log_output("[%02d] left_pos = %d", joint_file_info_p->current_file_num, left_pos ) ;

		// �Ĥ�Х��ȿ������ե���������礭���Ȥ���
		if ( left_pos > joint_file_info_p->file[joint_file_info_p->current_file_num].size )
		{
			left_pos -= joint_file_info_p->file[joint_file_info_p->current_file_num].size;
			joint_file_info_p->current_file_num++;
			start_file_pos = 0;
		}
		else // �Ĥ�Х��ȿ������ե������꾮�����Ȥ���
		{
			start_file_pos = left_pos;
			break;
		}
	}

	debug_log_output("start_file_num=%d", joint_file_info_p->current_file_num);
	debug_log_output("start_file_pos=%d", start_file_pos);



	// ------------------------------
	// �ǽ�Υե�����򥪡��ץ�
	// ------------------------------
	fd = open(joint_file_info_p->file[joint_file_info_p->current_file_num].name , O_RDONLY);
	if ( fd < 0 )
	{	
		debug_log_output("open() error. '%s'", joint_file_info_p->file[joint_file_info_p->current_file_num].name);
		return ( -1 );
	}


	// ------------------------------------------------
	// �������Ȱ���(range_start_pos)�إե����륷����
	// ------------------------------------------------
	lseek_ret = lseek(fd, start_file_pos, SEEK_SET);
	if ( lseek_ret < 0 )	// lseek ���顼�����å�
	{
		debug_log_output("lseek() error.");
		close(fd);
		return ( -1 );
	}

	copy_descriptors(fd, accept_socket, content_length, joint_file_info_p);

	// ���ｪλ
	return 0;
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
	for ( i=1; i<JOINT_MAX; i++ )
	{
		// �ե�����̾����
		// /VTS_01_1.VOB �� VTS_01_2.VOB
		series_filename[strlen(series_filename)-5]++;

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

