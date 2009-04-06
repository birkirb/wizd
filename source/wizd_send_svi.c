// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_send_svi.c
//											$Revision: 1.8 $
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


#include "wizd.h"


static int	analyze_svi_file(unsigned char *svi_filename, JOINT_FILE_INFO_T *joint_file_info_p );
static int http_joint_file_send(int	accept_socket, JOINT_FILE_INFO_T *joint_file_info_p, off_t content_length, off_t range_start_pos );




// **************************************************************************
// JOINT�ե������Ϣ�뤷���ֿ����롣
// **************************************************************************
int http_joint_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p  )
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
	debug_log_output("http_joint_file_response() start" );

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
	// SVI�ե���������GET
	// --------------------------
	ret = analyze_svi_file(http_recv_info_p->send_filename, joint_file_info_p );
	debug_log_output("http_joint_file_response() end (ret=%d)" );
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
	http_joint_file_send(	accept_socket, 	joint_file_info_p,
											joint_content_length,
											http_recv_info_p->range_start_pos );


	free( joint_file_info_p );	// Memory Free.

	return 0;
}





// **************************************************************************
// �ե�����μ��Τ��������롣
// **************************************************************************
static int http_joint_file_send(int	accept_socket, JOINT_FILE_INFO_T *joint_file_info_p, off_t content_length, off_t range_start_pos )
{

	int fd;

	unsigned char 	*send_buf_p;
	unsigned char 	*read_p;

	ssize_t			file_read_len;
	int				data_send_len;
	off_t			lseek_ret;

	off_t			total_read_size;
	unsigned int	target_charge_size;
	unsigned int	buf_charge_size;

	unsigned int	current_file_num;

	off_t			start_file_pos;
	off_t			left_pos;


	unsigned char	filename[WIZD_FILENAME_MAX];
	strncpy(filename, joint_file_info_p->file[0].name, sizeof(filename));


	debug_log_output("---------------------------------------------------");
	debug_log_output("http_joint_file_send() start" );

	debug_log_output("range_start_pos=%d", range_start_pos);
	debug_log_output("content_length=%d", content_length);

	// ======================
	// �����Хåե������
	// ======================
	send_buf_p = malloc(SEND_BUFFER_SIZE);
	if ( send_buf_p == NULL )
	{
		debug_log_output("malloc() error.\n");
		return (-1 );
	}


	// -------------------------
	// �������Ȱ��֤�׻�
	// -------------------------
	debug_log_output("calc start pos.");
	current_file_num = 0;
	start_file_pos = 0;

	left_pos = range_start_pos;
	while ( 1 )
	{
		debug_log_output("[%02d] left_pos = %d", current_file_num, left_pos ) ;

		// �Ĥ�Х��ȿ������ե���������礭���Ȥ���
		if ( left_pos > joint_file_info_p->file[current_file_num].size )
		{
			left_pos -= joint_file_info_p->file[current_file_num].size;
			current_file_num++;
			start_file_pos = 0;
		}
		else // �Ĥ�Х��ȿ������ե������꾮�����Ȥ���
		{
			start_file_pos = left_pos;
			break;
		}
	}

	debug_log_output("start_file_num=%d", current_file_num);
	debug_log_output("start_file_pos=%d", start_file_pos);



	// ------------------------------
	// �ǽ�Υե�����򥪡��ץ�
	// ------------------------------
	fd = open(joint_file_info_p->file[current_file_num].name , O_RDONLY);
	if ( fd < 0 )
	{
		debug_log_output("open() error. '%s'", joint_file_info_p->file[current_file_num].name);
		free(send_buf_p);
		return ( -1 );
	}


	// ------------------------------------------------
	// �������Ȱ���(range_start_pos)�إե����륷����
	// ------------------------------------------------
	lseek_ret = lseek(fd, start_file_pos, SEEK_SET);
	if ( lseek_ret < 0 )	// lseek ���顼�����å�
	{
		debug_log_output("lseek() error.");
		free(send_buf_p);
		close(fd);
		return ( -1 );
	}

	total_read_size = 0;
	// ============================
	// �ե��������ž������
	// ============================
	while ( 1 )
	{
		// ����Хåե����ꥢ
		memset(send_buf_p, 0, SEND_BUFFER_SIZE);

		// ��ɸread�������׻�
		if ( (content_length - total_read_size) > SEND_BUFFER_SIZE )
		{
			target_charge_size = SEND_BUFFER_SIZE;
		}
		else
		{
			target_charge_size = (content_length - total_read_size);
		}

		// �ɤ�ɬ�פʤ��ʤä��顢������λ��
		if ( target_charge_size == 0 )
		{
			debug_log_output("EOF Detect.");
			break;
		}


		// ---------------------------------
		// �ե����뤫��Хåե��˽�Ŷ
		// ---------------------------------
		read_p = send_buf_p;
		buf_charge_size = 0;	// �Хåե��˽�Ŷ���줿�Х��ȿ�

		while ( 1 )
		{
			file_read_len = read(fd, read_p, (target_charge_size - buf_charge_size) );
			// debug_log_output("file_read_len = %d, target_charge_size = %d", file_read_len, (target_charge_size- buf_charge_size));

			buf_charge_size += file_read_len;	// �Хåե��˽�Ŷ���줿�Х��ȿ���׻�
			read_p += file_read_len;

			if ( buf_charge_size >= target_charge_size )	// ��Ŷ��λ��
			{
				break;
			}

			// �ɤ߽���ä��ե������CLOSE()
			debug_log_output("[%02d] '%s' close()", current_file_num, joint_file_info_p->file[current_file_num].name);
			close(fd);

			current_file_num++;
			if ( current_file_num >= joint_file_info_p->file_num )
			{
				debug_log_output("EOF Detect.");
				break;
			}

			// ���Υե������OPEN()
			debug_log_output("[%02d] '%s' open()", current_file_num, joint_file_info_p->file[current_file_num].name);
			fd = open(joint_file_info_p->file[current_file_num].name, O_RDONLY);
			if ( fd < 0 )
			{
				debug_log_output("open() error. '%s'", joint_file_info_p->file[current_file_num].name);
				free(send_buf_p);
				return ( -1 );
			}

			debug_log_output("buf_charge_size = %d", buf_charge_size);
		}



		// --------------------------------
		// SOCKET �˥ǡ���������
		// --------------------------------
		data_send_len = send(accept_socket, send_buf_p, buf_charge_size, 0);
		//debug_log_output("data_send_len=%d, send_data_len=%d\n", data_send_len, file_read_len);
		if ( data_send_len != buf_charge_size )
		{
			debug_log_output("send() error.\n");
			close(fd);	// File Close.
			return ( -1 );
		}

		total_read_size += buf_charge_size;
		if ( content_length != 0 )
		{
			debug_log_output("Streaming..  %lld / %lld ( %lld.%lld%% )\n", total_read_size, content_length, total_read_size * 100 / content_length,  (total_read_size * 1000 / content_length ) % 10 );
		}
		if ( total_read_size >= content_length)
		{
			debug_log_output("send() end.(content_length=%d)\n", content_length );
		}

	}

	free(send_buf_p);	// Memory Free.

	// ���ｪλ
	return 0;
}







// **************************************************************************
// SVI�ե��������Ϥ��ơ�joint_file_info_p ������롣
// **************************************************************************
static int analyze_svi_file(unsigned char *svi_filename, JOINT_FILE_INFO_T *joint_file_info_p )
{
	int		fd;
	unsigned char	read_filename[SVI_FILENAME_LENGTH+10];

	unsigned char	svi_filepath[WIZD_FILENAME_MAX];

	unsigned char	first_filename[WIZD_FILENAME_MAX];
	unsigned char	series_filename[WIZD_FILENAME_MAX];
	unsigned char	file_extension[WIZD_FILENAME_MAX];
	unsigned char	work_buf[32];
	unsigned char	*p;

	off_t		lseek_ret;
	int			ret;
	ssize_t		read_length;
	int			i;

	struct stat file_stat;


	debug_log_output("analyze_svi_file() start.");
	debug_log_output("svi_filename='%s'", svi_filename);

	// ------------------
	// ��������
	// ------------------
	memset(read_filename, '\0', sizeof(read_filename));
	memset(first_filename, '\0', sizeof(first_filename));



	// ----------------------------------------
	// SVI�Υե�����ѥ���GET���Ƥ���
	// �����줬��documet_root������ˤʤ롣
	// ----------------------------------------
	strncpy(svi_filepath, svi_filename, sizeof(svi_filepath));

	// �Ǹ��'/'���������
	cut_after_last_character(svi_filepath, '/');

	// '/'���ɲ�
	strncat(svi_filepath, "/", sizeof(svi_filepath) - strlen(svi_filepath) );


	debug_log_output("svi_filepath = '%s'", svi_filepath);


	// --------------------------------
	// SVI�ե����뤫�������Ф�
	// --------------------------------

	// SVI�ե����볫��
	fd = open(svi_filename, O_RDONLY);
	if ( fd < 0 )
	{
		debug_log_output("oepn() error.");
		return ( -1 );
	}

	// SVI�ե�����Υե�����̾���֤�GET
	lseek_ret = lseek(fd, SVI_FILENAME_OFFSET, SEEK_SET);
	if ( lseek_ret < 0 )
	{
		debug_log_output("lseek() error.");
		return (-1);
	}

	// SVI�ե����뤫�顢m2p�ե�����̾��get
	read_length = read(fd, read_filename, SVI_FILENAME_LENGTH );
	debug_log_output("read_length=%d, SVI_FILENAME_LENGTH=%d", read_length, SVI_FILENAME_LENGTH);

	if (read_length != SVI_FILENAME_LENGTH)
	{
		debug_log_output("read() error.");
		return ( -1 );
	}
	close( fd );

	debug_log_output("read_filename = '%s'", read_filename);

	// -------------------------------------
	// SVI�����Ф����ե�����̾��ù�
	// "C:\SVRECORD\20030701_211500\20030701_211500.m2p"
	//		��
	// "20030701_211500/20030701_211500.m2p"
	// -------------------------------------

	// �Ǹ��'\'�򸡺����ơ�'/'���ִ�
	p = strrchr(read_filename, '\\');
	if ( p != NULL )
	{
		*p = '/';
	}
	debug_log_output("read_filename = '%s'", read_filename);


	// '\'���ޤ����ä��顢�Ǹ�˽ФƤ���'\'�����������
	p = strrchr(read_filename, '\\');
	if ( p != NULL )
	{
		cut_before_last_character(read_filename , '\\' );
	}
	debug_log_output("read_filename = '%s'", read_filename);




	// ----------------
	// �ե�ѥ�����
	// ----------------

	// Ƭ��SVI�ե�����ѥ����ɲá�
	strncpy(first_filename, svi_filepath, sizeof(first_filename));


	// SVI�����ɤ�����Υե�����̾���ɲá�
	strncat(first_filename, read_filename, sizeof(first_filename) - strlen(svi_filepath));

	debug_log_output("first_filename = '%s'", first_filename);



	// ----------------------------------------------------
	// SVI�����ɤ���ե�����Ρ��ե���������Get
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
	for ( i=1; i<JOINT_MAX; i++ )
	{
		// �ե�����̾����
		// hogehoge.m2p �� hogehoge.001.m2p
		strncpy( series_filename, first_filename, sizeof(series_filename));

		// ��ĥ�Ҥ���¸
		memset(file_extension, '\0', sizeof(file_extension));
		p = strrchr( series_filename, '.');
		if ( p != NULL )
		{
			strncpy( file_extension, p, sizeof(file_extension) );
		}

		// ��ĥ�Ҥ�CUT
		cut_after_last_character(series_filename, '.');


		// ���ꥢ����ͤ��ɲá�
		snprintf(work_buf, sizeof(work_buf), ".%03d", i );
		strncat(series_filename, work_buf, sizeof(series_filename) - strlen(series_filename) );

		// ��¸������ĥ�Ҥ��᤹��
		strncat(series_filename, file_extension, sizeof(series_filename) - strlen(series_filename) );

		// �ե���������GET
		ret = stat(series_filename, &file_stat);
		if ( ret != 0 ) // �ե�����̵����������λ��
		{
			debug_log_output("'%s' Not found.", series_filename);
			break;
		}

		// GET�����ե���������joint_file_info_p �˥��å�
		strncpy( joint_file_info_p->file[i].name, series_filename, sizeof(joint_file_info_p->file[0].name));
		joint_file_info_p->file[i].size 	= file_stat.st_size;
		joint_file_info_p->total_size 		+= file_stat.st_size;
		joint_file_info_p->file_num++;

	}


	debug_log_output("analyze_svi_file() end.");

	return 0;
}


// **************************************************************************
// SVIϿ����� �ɤ߹���
// **************************************************************************
int read_svi_info(unsigned char *svi_filename, unsigned char *svi_info, int svi_info_size, unsigned int *rec_time )
{
	int		fd;

	off_t		lseek_ret;
	ssize_t		read_length;
	size_t		read_size;
	unsigned char	rec_time_work[2];

	debug_log_output("\n\nread_svi_info() start.");


	read_size = svi_info_size;
	if ( read_size > SVI_INFO_LENGTH )
	{
		read_size = SVI_INFO_LENGTH;
	}

	memset( svi_info, '\0', svi_info_size );
	*rec_time = 0;

	// --------------------------------
	// SVI�ե����뤫�������Ф�
	// --------------------------------

	// SVI�ե����볫��
	fd = open(svi_filename, O_RDONLY);
	if ( fd < 0 )
	{
		debug_log_output("oepn('%s') error.", svi_filename);
		return (-1);
	}

	// SVI�ե�����Υե��������ΰ��֤�SEEK
	lseek_ret = lseek(fd, SVI_INFO_OFFSET, SEEK_SET);
	if ( lseek_ret < 0 )
	{
		debug_log_output("lseek() error.");
		return ( -1 );
	}

	// SVI�ե����뤫�顢�ե���������get
	read_length = read(fd, svi_info, read_size);
	debug_log_output("read_length=%d, read_size=%d", read_length, read_size);
	if (read_length != read_size)
	{
		debug_log_output("fread() error.");
		return (-1);
	}


	// SVI�ե�����Υǡ������֤�SEEK
	lseek_ret = lseek(fd, SVI_REC_TIME_OFFSET, SEEK_SET);
	if ( lseek_ret < 0 )
	{
		debug_log_output("lseek() error.");
		return ( -1 );
	}

	// SVI�ե����뤫�顢Ͽ����֤�GET
	read_length = read(fd, rec_time_work, SVI_REC_TIME_LENGTH );
	debug_log_output("read_length=%d, SVI_REC_TIME_LENGTH=%d", read_length, SVI_FILENAME_LENGTH);
	if (read_length != SVI_REC_TIME_LENGTH)
	{
		debug_log_output("read() error.");
		return ( -1 );
	}

	debug_log_output("read_rec=%02X,%02X", rec_time_work[0], rec_time_work[1]  );

	*rec_time += (unsigned int)( rec_time_work[0] );
	*rec_time += (unsigned int)( rec_time_work[1] << 8 );

	debug_log_output("rec_time=%d", *rec_time );


	close( fd );

	debug_log_output("read_filename = '%s'", svi_info);

	return ( 0 );
}


// **************************************************************************
//  SVI m2p�ե�����total�������ɤ߹���
// **************************************************************************
u_int64_t  svi_file_total_size(unsigned char *svi_filename)
{
	int		fd;

	int			i;
	off_t		lseek_ret;
	ssize_t		read_length;

	unsigned char	total_size_work[SVI_TOTAL_SIZE_LENGTH];
	u_int64_t		total_size;

	// --------------------------------
	// SVI�ե����뤫�������Ф�
	// --------------------------------

	// SVI�ե����볫��
	fd = open(svi_filename, O_RDONLY);
	if ( fd < 0 )
	{
		debug_log_output("oepn() error.");
		return 0;
	}

	// SVI�ե�����Υե�������֤�GET
	lseek_ret = lseek(fd, SVI_TOTAL_SIZE_OFFSET, SEEK_SET);
	if ( lseek_ret < 0 )
	{
		debug_log_output("lseek() error.");
		return 0;
	}

	// SVI�ե����뤫�顢m2p�ե����� ��ץ�������get
	read_length = read(fd, total_size_work, SVI_TOTAL_SIZE_LENGTH);
	debug_log_output("read_length=%d", read_length);
	debug_log_output("total_size=%02X,%02X,%02X,%02X,%02X"
		, total_size_work[0], total_size_work[1],total_size_work[2],total_size_work[3], total_size_work[4]);
	if (read_length != SVI_TOTAL_SIZE_LENGTH)
	{
		debug_log_output("read() error.");
		return 0;
	}

	close( fd );

	total_size = 0;
	for ( i=0; i<SVI_TOTAL_SIZE_LENGTH; i++ )
	{
		total_size += (u_int64_t)total_size_work[i] << (8 * i);
	}

	debug_log_output("total_size=%lld", total_size);

	return total_size;


}
