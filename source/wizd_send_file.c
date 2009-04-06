// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_send_file.c
//											$Revision: 1.9 $
//											$Date: 2004/12/18 08:29:37 $
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
#include <sys/timeb.h>

#include "wizd.h"

#define BUFFER_COUNT global_param.buffer_size
#define BUFFER_ATOM global_param.stream_chunk_size
#define SOCKET_ATOM global_param.socket_chunk_size
#define		SEND_BUFFER_SIZE	(1024*128)

// extern int errno;

static off_t http_simple_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos );
static off_t http_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos );
static int next_file(int *in_fd_p, JOINT_FILE_INFO_T *joint_file_info_p);


// **************************************************************************
// ファイル実体の返信。
// ヘッダ生成＆送信準備
// **************************************************************************
int http_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	int	send_header_data_len;
	int	result_len;

	unsigned char	send_http_header_buf[2048];
	unsigned char	work_buf[1024];

	off_t	content_length;
	off_t	written;

	struct	stat	file_stat;
	int				result;



	// ---------------
	// 作業用変数初期化
	// ---------------
	memset(send_http_header_buf, '\0', sizeof(send_http_header_buf));



	// -------------------------------
	// ファイルサイズチェック
	// -------------------------------
	if ( http_recv_info_p->range_end_pos > 0 )	// end位置指定有り。
	{
		content_length = (http_recv_info_p->range_end_pos - http_recv_info_p->range_start_pos) + 1;
	}
/*
	else if ( http_recv_info_p->range_start_pos == 0 )
	{
		content_length = 0;
	}
*/
	else // end位置指定無し。
	{
		result = stat(http_recv_info_p->send_filename, &file_stat); // ファイルサイズチェック。
		if ( result != 0 )
		{
			debug_log_output("file not found.");
			return ( -1 );
		}

		content_length = file_stat.st_size - http_recv_info_p->range_start_pos;
	}



	// --------------
	// OK ヘッダ生成
	// --------------
	strncpy(send_http_header_buf, HTTP_OK, sizeof(send_http_header_buf));

	strncat(send_http_header_buf, HTTP_CONNECTION, sizeof(send_http_header_buf) - strlen(send_http_header_buf));

	snprintf(work_buf, sizeof(work_buf), HTTP_SERVER_NAME, SERVER_NAME);
	strncat(send_http_header_buf, work_buf, sizeof(send_http_header_buf) - strlen(send_http_header_buf));

	if (content_length) {
		snprintf(work_buf, sizeof(work_buf), HTTP_CONTENT_LENGTH, content_length);
		strncat(send_http_header_buf, work_buf, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );
	}

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
	debug_log_output("result_len=%d, send_data_len=%d\n", result_len, send_header_data_len);


	// Small files go to simple function
	if(content_length <= SEND_BUFFER_SIZE) {
		written = http_simple_file_send( accept_socket, http_recv_info_p->send_filename,
								content_length,
								http_recv_info_p->range_start_pos );
	} else {
		// --------------
		// 実体返信
		// --------------
		written = http_file_send(	accept_socket, 	http_recv_info_p->send_filename,
									content_length,
									http_recv_info_p->range_start_pos );
	}

	if(global_param.bookmark_threshold) {
		if(  /* (written < global_param.bookmark_threshold)
		  || */ (content_length && ((written+global_param.bookmark_threshold) > content_length))
		  || strcmp(http_recv_info_p->mime_type, "video/mpeg") ) { // Only create bookmarks for MPEG video files
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


void set_blocking_mode(int fd, int flag)
{
	int res, nonb = 0;

	nonb |= O_NONBLOCK;

	if ((res = fcntl(fd, F_GETFL, 0)) == -1) {
		debug_log_output("fcntl(fd, F_GETFL) failed");
	}
	if (flag) {
		res |= O_NONBLOCK;
	} else {
		res &= ~O_NONBLOCK;
	}
	if (fcntl(fd, F_SETFL, res) == -1) {
		debug_log_output("fcntl(fd, F_SETFL, nonb) failed");
	}
}

// **************************************************************************
// ファイルの実体の送信実行部
// **************************************************************************
static off_t http_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos )
{
	int		fd;
	off_t			seek_ret;
	off_t		written=0;

	// ---------------------
	// ファイルオープン
	// ---------------------
	fd = open(filename, O_RDONLY);
	if ( fd < 0 )
	{
		debug_log_output("errno = %s\n", strerror(errno));
		debug_log_output("open() error. file = '%s'", filename);
		return ( 0 );
	}

if(0 && range_start_pos != 0) {
	// Send the first 64kB of the file to catch AVI headers
	char buf[65536];
	int len = read(fd, buf, sizeof(buf));
	if(len > 0) {
		write(accept_socket, buf, len);
		// Pad with a bunch of zeros to force a reset
		memset(buf, 0, sizeof(buf));
		write(accept_socket, buf, sizeof(buf));
		write(accept_socket, buf, sizeof(buf));
		write(accept_socket, buf, sizeof(buf));
		write(accept_socket, buf, sizeof(buf));
	}
}
	// ------------------------------------------
	// range_start_posへファイルシーク
	// ------------------------------------------
	seek_ret = lseek(fd, range_start_pos, SEEK_SET);
	if ( seek_ret < 0 )	// lseek エラーチェック
	{
		debug_log_output("lseek() error.");
		close(fd);
		return ( 0 );
	}

	written = copy_descriptors(fd, accept_socket, content_length, NULL);

	close(fd);	// File Close.

	// 正常終了
	return written;
}

void show_progress(int progress , int n)
{
	int i;
	char buf[62];
	char date_and_time[32];

	if (global_param.flag_daemon) return ;
	buf[0] = '|';
	for (i=0; i<50; i++) {
		if (i == progress/2) {
			static int r = 0;
			buf[i+1] = "-\\|/"[r++];
			r %= 4;
		} else {
			buf[i+1] = i*2 < progress ? 'o' : ' ';
		}
	}
	buf[i++] = '|';
	buf[i] = '\0';
	//debug_log_output(buf);
	make_datetime_string(date_and_time);
	fprintf(stdout, "%s %s %5d\r", date_and_time, buf, n);
	fflush(stdout);
}

/*
 * バッファリングしながら in_fd から out_fd へ データを転送
 * 書き込み側を NONBLOCK モードに設定することによって
 * write(send?) 中にブロックするのを防ぐ。
 * こうすることで、読み込みができる場合にはさらにキャッシュにデータを
 * ためることができる。
 *
 * 今のところ上のファイル実体送信と、プロクシから呼び出している.
 * content_length をあまり重要視していない..
 *
 * vobの連続再生に対応のため、複数ファイルの連続転送に対応
 *
 */

off_t copy_descriptors(int in_fd, int out_fd, off_t content_length, JOINT_FILE_INFO_T *joint_file_info_p)
{
	int i;
	struct _buff {
		int inuse;
		int pos;
		int len;
		unsigned char *p;
	} *buff;
	unsigned char *p;
	int read_idx = 0;
	int write_idx = 0;
	int idx_count = 0;

	off_t			total_read_size = 0;
	size_t			target_read_size;
	int				len;

	int flag_finish = 0;
	int flag_first_time = 0;
	int flag_verbose = 0;
	time_t first_timeout = 0;
	off_t written = 0;
	
	struct timeb marker1,marker2,marker3;
	int marker_count;

	if(BUFFER_COUNT < 1) BUFFER_COUNT = 1;
	if(BUFFER_ATOM < 512) BUFFER_ATOM = 512;

	// ======================
	// 送信バッファを確保
	// ======================

	debug_log_output("Allocating %d buffers of %d bytes each\n", BUFFER_COUNT, BUFFER_ATOM);

	buff = malloc(BUFFER_COUNT * sizeof(struct _buff));
	if ( buff == NULL )
	{
		debug_log_output("malloc() error.\n");
		return (-1 );
	}
	p = malloc(BUFFER_COUNT * BUFFER_ATOM);
	if ( p == NULL )
	{
		debug_log_output("malloc() error.\n");
		return ( 0 );
	}
	for (i=0; i<BUFFER_COUNT; i++) {
		buff[i].pos = 0;
		buff[i].inuse = 0;
		buff[i].len = 0;
		buff[i].p = p + i*BUFFER_ATOM;
	}

	// ブロックモードの設定
	//set_blocking_mode(in_fd, 0);	/* blocking */
	set_blocking_mode(out_fd, (BUFFER_COUNT>1));	/* non-blocking if multiple buffers */
	//debug_log_output("set non-blocking mode");

	// Do the calculation this way so BUFFER_COUNT<4 always gets flag_first_time
	if (content_length < BUFFER_ATOM * (BUFFER_COUNT>>2)) flag_first_time = 1;
	if (BUFFER_COUNT == 1) global_param.flag_buffer_send_asap = 1;

	ftime(&marker1);
	marker_count = 0;

	first_timeout = time(NULL);
	// ================
	// 実体転送開始
	// ================
	while ( flag_finish < 2 )
	{
		struct timeval tv;
		fd_set writefds;

		if (flag_first_time == 0 && first_timeout + 13 <= time(NULL)) {
				debug_log_output( 
				   "****************************************************\n"
				   "**  Low bandwidth? send it anyway...              **\n"
				   "****************************************************");
				flag_first_time = 1;
		}
		if (flag_finish || flag_first_time || idx_count > (BUFFER_COUNT/2)) 
		{
			if (flag_first_time == 0) {
				debug_log_output( 
				   "*********************************************************\n"
				   "**                    CACHE FILLED!                    **\n"
				   "*********************************************************");
				flag_first_time = 1;
			}
			while(buff[write_idx].inuse) {
				// If there is nothing more to read, concentrate on writing
				FD_ZERO(&writefds);
				FD_SET(out_fd, &writefds);
				tv.tv_sec = 0;
				if (flag_finish == 0 && !buff[read_idx].inuse) {
					// Don't wait for the select because we have room to read
					tv.tv_usec = 0;
				} else {
					tv.tv_usec = 100000; // 100msec maximum wait = 10Hz polling rate
				}
//ftime(&marker2);
				i = select(FD_SETSIZE, NULL, &writefds, NULL, &tv);
//ftime(&marker3);
//len = (marker3.time-marker2.time)*1000 + (marker3.millitm-marker2.millitm);
//if(len>9) fputc('0'+len/10, stderr);
				if (i < 0) {
					// Select returned an error - socket must be closed
					debug_log_output("select failed. err = %s", strerror(errno));
					flag_finish = 2;
					break;
				} else if(i > 0) {
//ftime(&marker2);
					len = write(out_fd, buff[write_idx].p + buff[write_idx].pos,
						(buff[write_idx].len < SOCKET_ATOM) ? buff[write_idx].len : SOCKET_ATOM );
//ftime(&marker3);
//i = (marker3.time-marker2.time)*1000 + (marker3.millitm-marker2.millitm);
//if(i>9) fputc('a'+i/10, stderr);
					if(len > 0) {
//fputc('.', stderr);
						written += len;
						buff[write_idx].len -= len;

						marker_count += len;
						if(!global_param.flag_daemon && (marker_count > 2000000)) {
							// Display the network transfer rate
							double rate;
							ftime(&marker2);
							rate = (marker2.time-marker1.time) + 0.001*(marker2.millitm-marker1.millitm);
							if(rate > 0) {
								rate = 8e-6 * marker_count / rate;
								fprintf(stderr, "%g Mbps\n", rate);
							}
							marker1 = marker2;
							marker_count = 0;
						}

						if (flag_verbose) {
							debug_log_output("sent: len =%6d, idx = %4d, idxcount = %4d", len, write_idx, idx_count);
						} else if(0 && !global_param.flag_daemon) {
							show_progress(idx_count * 100 / BUFFER_COUNT, idx_count);
						}
						if (buff[write_idx].len <= 0) {
							buff[write_idx].inuse = 0;
							buff[write_idx].len = 0;
							buff[write_idx].pos = 0;
							write_idx = (write_idx + 1) % BUFFER_COUNT;
							idx_count --;
						} else {
							buff[write_idx].pos += len;
						}
					} else {
//fputc('-', stderr);
						// Failed to write - end the stream
						if (len < 0) {
							if(errno == EAGAIN) {
								debug_log_output("write would block");
								break;
							}
							debug_log_output("write failed. err = %s", strerror(errno));
						} else {
							debug_log_output("socket closed by player");
						}
						flag_finish = 2;
						break;
					}
				} else {
					// Not ready to write, exit from the loop to do a read
//if (flag_finish == 0 && !buff[read_idx].inuse) {
//	fputc(',', stderr);
//} else {
//	fputc('o', stderr);
//}
					break;
				}
			}
			if((flag_finish==1) && !buff[write_idx].inuse) {
				flag_finish = 2;
				debug_log_output(
				   "*********************************************************\n"
				   "**                    SEND FINISHED!                   **\n"
				   "*********************************************************");
			}
		}
		// Always attempt a read if we have a buffer available
		//if (FD_ISSET(in_fd, &readfds)) {
		if(flag_finish == 0 && !buff[read_idx].inuse) {
			// target_read_size = (content_length - total_read_size) > 1024 ? 1024 : (content_length - total_read_size);
			target_read_size = BUFFER_ATOM - buff[read_idx].len;
/*
			if (buff[read_idx].p == NULL) {
				debug_log_output("error! idx: %d", read_idx);
			}
*/
//ftime(&marker2);
			len = read(in_fd, buff[read_idx].p + buff[read_idx].len, target_read_size);
//ftime(&marker3);
//i = (marker3.time-marker2.time)*1000 + (marker3.millitm-marker2.millitm);
//if(i>9) fputc('A'+i/10, stderr);
			if (len == 0) {
				if (next_file(&in_fd, joint_file_info_p)){
					// 読み込み終わり
					flag_finish = 1;
					if (flag_verbose) {
						debug_log_output("recv: len = %d, idx = %d finish!", len, read_idx);
					} else {
						debug_log_output(
					   "*********************************************************\n"
					   "**                    RECV FINISHED!                   **\n"
					   "*********************************************************");
					}
					if (buff[read_idx].len > 0) {
						buff[read_idx].inuse = 1;
						buff[read_idx].pos = 0;
					}
				} else {
					// 次のファイルに続く(ここでは何もしない)
				}
			} else if (len > 0) {
//fputc(':', stderr);
				if (flag_verbose) {
					debug_log_output("recv: len =%6d, idx = %4d, idxcount = %4d", len, read_idx, idx_count);
				} else if(0 && !global_param.flag_daemon) {
					show_progress(idx_count * 100 / BUFFER_COUNT, idx_count);
				}
				buff[read_idx].len += len;
				total_read_size += len;

				if (global_param.flag_buffer_send_asap == TRUE
				|| buff[read_idx].len >= BUFFER_ATOM) {
					buff[read_idx].inuse = 1;
					buff[read_idx].pos = 0;
					idx_count ++;
					read_idx = (read_idx + 1) % BUFFER_COUNT;
				}
/*
				if (content_length - total_read_size <= 0) {
					flag_finish = 1;
				}
*/
			} else {
				flag_finish = 1;
				if (flag_verbose) {
					debug_log_output("read err?: len = %d, idx = %d, err: %s", len, read_idx, strerror(errno));
				} else {
					debug_log_output(
				   "*********************************************************\n"
				   "**                    RECV FINISHED!(ret < 0)          **\n"
				   "*********************************************************");
				}
			}
		}
	}

	free(p);
	free(buff);	// Memory Free.
	debug_log_output("copy descriptors end.");

	// 正常終了
	return written;
}


static int next_file(int *in_fd_p, JOINT_FILE_INFO_T *joint_file_info_p)
{
	if (in_fd_p && joint_file_info_p)
	{
		// 読み終わったファイルをCLOSE()
		debug_log_output("[%02d] '%s' close()", joint_file_info_p->current_file_num, joint_file_info_p->file[joint_file_info_p->current_file_num].name);
		close(*in_fd_p);

		// 次のファイルがあるか?
		joint_file_info_p->current_file_num++;
		if ( joint_file_info_p->current_file_num >= joint_file_info_p->file_num )
		{
			debug_log_output("EOF Detect.");
			return 1;		// これで終了
		}

		// 次のファイルをOPEN()
		debug_log_output("[%02d] '%s' open()", joint_file_info_p->current_file_num, joint_file_info_p->file[joint_file_info_p->current_file_num].name);
		*in_fd_p = open(joint_file_info_p->file[joint_file_info_p->current_file_num].name, O_RDONLY);
		if ( *in_fd_p < 0 )
		{
			debug_log_output("errno = %s\n", strerror(errno));
			debug_log_output("open() error. '%s'", joint_file_info_p->file[joint_file_info_p->current_file_num].name);
			return ( -1 );
		}

		// ブロックモードの設定
		set_blocking_mode(*in_fd_p, 0);	/* blocking */

		return 0;		// 次のファイルの準備完了
	} else {
		// パラメータがNULLの場合には1ファイルのみの処理とする
		return 1;		// これで終了
	}
}

// **************************************************************************
// ファイルの実体の送信実行部
// **************************************************************************
static off_t http_simple_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos )
{
	int		fd;

	unsigned char 	*send_buf_p;

	ssize_t			file_read_len;
	int				data_send_len;
	off_t			seek_ret;

	off_t			total_read_size;
	size_t			target_read_size;

	// ======================
	// 送信バッファを確保
	// ======================

	send_buf_p = malloc(SEND_BUFFER_SIZE);
	if ( send_buf_p == NULL )
	{
		debug_log_output("malloc() error.\n");
		return ( 0 );
	}

	// ---------------------
	// ファイルオープン
	// ---------------------
	fd = open(filename, O_RDONLY);
	if ( fd < 0 )
	{	
		debug_log_output("errno = %s\n", strerror(errno));
		debug_log_output("open() error. file='%s'", filename);
		free(send_buf_p);
		return ( 0 );
	}


	// ------------------------------------------
	// range_start_posへファイルシーク
	// ------------------------------------------
	seek_ret = lseek(fd, range_start_pos, SEEK_SET);
	if ( seek_ret < 0 )	// lseek エラーチェック
	{
		debug_log_output("errno = %s\n", strerror(errno));
		debug_log_output("lseek() error.");
		free(send_buf_p);
		close(fd);
		return ( 0 );
	}


	total_read_size = 0;

	// ================
	// 実体転送開始
	// ================
	while ( 1 )
	{
		// 一応バッファクリア
		memset(send_buf_p, 0, SEND_BUFFER_SIZE);

		// 目標readサイズ計算
		if ( (content_length - total_read_size) > SEND_BUFFER_SIZE )
		{
			target_read_size = SEND_BUFFER_SIZE;
		}
		else
		{
			target_read_size = (size_t)(content_length - total_read_size);
		}


		// ファイルからデータを読み込む。
		file_read_len = read(fd, send_buf_p, target_read_size);
		if ( file_read_len <= 0 )
		{
			debug_log_output("EOF detect.\n");
			break;
		}



		// SOCKET にデータを送信
		data_send_len = send(accept_socket, send_buf_p, file_read_len, 0);
		if ( data_send_len != file_read_len ) 
		{
			debug_log_output("send() error.\n");
			close(fd);	// File Close.
			return ( total_read_size );
		}

		total_read_size += file_read_len;
	}

	free(send_buf_p);	// Memory Free.
	close(fd);	// File Close.

	// 正常終了
	return total_read_size;
}



