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


#include "wizd.h"

#define BUFFER_COUNT ((global_param.buffer_size == 0 ? 1024 : global_param.buffer_size))
#define BUFFER_ATOM (10240)

// extern int errno;

static int http_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos );
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


	// --------------
	// 実体返信
	// --------------
	http_file_send(	accept_socket, 	http_recv_info_p->send_filename,
									content_length,
									http_recv_info_p->range_start_pos );



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
static int http_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos )
{
	int		fd;
	off_t			seek_ret;

	// ---------------------
	// ファイルオープン
	// ---------------------
	fd = open(filename, O_RDONLY);
	if ( fd < 0 )
	{
		debug_log_output("open() error. file = '%s'", filename);
		debug_log_output("errno = %s\n", strerror(errno));
		return ( -1 );
	}

	// ------------------------------------------
	// range_start_posへファイルシーク
	// ------------------------------------------
	seek_ret = lseek(fd, range_start_pos, SEEK_SET);
	if ( seek_ret < 0 )	// lseek エラーチェック
	{
		debug_log_output("lseek() error.");
		close(fd);
		return ( -1 );
	}

	copy_descriptors(fd, accept_socket, content_length, NULL);

	close(fd);	// File Close.

	// 正常終了
	return 0;
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

int copy_descriptors(int in_fd, int out_fd, off_t content_length, JOINT_FILE_INFO_T *joint_file_info_p)
{
	int i;
	struct _buff {
		int inuse;
		int pos;
		int len;
		unsigned char p[BUFFER_ATOM];
	} *buff;
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


	// ======================
	// 送信バッファを確保
	// ======================

	buff = malloc(BUFFER_COUNT * sizeof(struct _buff));
	if ( buff == NULL )
	{
		debug_log_output("malloc() error.\n");
		return (-1 );
	}
	for (i=0; i<BUFFER_COUNT; i++) {
		buff[i].pos = 0;
		buff[i].inuse = 0;
		buff[i].len = 0;
	}

	// ブロックモードの設定
	set_blocking_mode(in_fd, 0);	/* blocking */
	set_blocking_mode(out_fd, 1);	/* non-blocking */
	debug_log_output("set non-blocking mode");
	if (content_length < BUFFER_ATOM * BUFFER_COUNT/4) flag_first_time = 1;

	first_timeout = time(NULL);
	// ================
	// 実体転送開始
	// ================
	while ( 1 )
	{
		struct timeval tv;
		fd_set readfds, writefds;

		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);

		if (flag_finish == 0 && !buff[read_idx].inuse) FD_SET(in_fd, &readfds);
		if (flag_first_time == 0 && first_timeout + 13 <= time(NULL)) {
				debug_log_output( /*
2004/01/06 03:22:48 */ // "\a"
				   "****************************************************\n"
				   "**  Low bandwidth? send it anyway...              **\n"
				   "****************************************************");
				flag_first_time = 1;
		}
		if (flag_finish || flag_first_time || idx_count > BUFFER_COUNT/4) FD_SET(out_fd, &writefds);
		//FD_SET(fd, &readfds);

		//debug_log_output("before select");
		select(FD_SETSIZE, &readfds, &writefds, NULL, NULL); // &tv);
		//debug_log_output("after. select");

		if (FD_ISSET(out_fd, &writefds)) {
			if (flag_first_time == 0) {
				debug_log_output( /*
2004/01/06 03:22:48 */ // "\a"
				   "*********************************************************\n"
				   "**                    CACHE FILLED!                    **\n"
				   "*********************************************************");
				flag_first_time = 1;
			}
			len = 0;

			// クライアントが終了したら SIGPIPE で 強制終了 される。
			while (buff[write_idx].inuse
			&& (len = write(out_fd, buff[write_idx].p + buff[write_idx].pos, buff[write_idx].len)) > 0) {
				buff[write_idx].len -= len;
				if (flag_verbose) {
					debug_log_output("sent: len =%6d, idx = %4d, idxcount = %4d", len, write_idx, idx_count);
				} else {
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
			}
			if (buff[write_idx].inuse == 0 && flag_finish) break;
			if (buff[write_idx].inuse != 0 && len < 0 && errno != EAGAIN) {
				debug_log_output("write failed. err = %s", strerror(errno));
				break;
			}
		}
		if (FD_ISSET(in_fd, &readfds)) {
			// target_read_size = (content_length - total_read_size) > 1024 ? 1024 : (content_length - total_read_size);
			target_read_size = BUFFER_ATOM - buff[read_idx].len;
/*
			if (buff[read_idx].p == NULL) {
				debug_log_output("error! idx: %d", read_idx);
			}
*/
			len = read(in_fd, buff[read_idx].p + buff[read_idx].len, target_read_size);
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
				if (flag_verbose) {
					debug_log_output("recv: len =%6d, idx = %4d, idxcount = %4d", len, read_idx, idx_count);
				} else {
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

	free(buff);	// Memory Free.
	debug_log_output("copy descriptors end.");

	// 正常終了
	return 0;
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


