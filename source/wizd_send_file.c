// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_send_file.c
//											$Revision: 1.27 $
//											$Date: 2006/07/09 17:04:09 $
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
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <dirent.h>
#include <errno.h>
#include <sys/timeb.h>
#include <dvdread/dvd_reader.h>

#include "wizd.h"
#include "wizd_tools.h"

// This is the size of the buffer used in the http_simple_file_send function
// Files smaller than 128kB will always get sent using http_simple_file_send
#define	SEND_BUFFER_SIZE	(1024*128)

// extern int errno;

static off_t http_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos );
static int next_file(int *in_fd_p, JOINT_FILE_INFO_T *joint_file_info_p);
static off_t copy_FILE_to_descriptor(FILE *in_fp, int out_fd, off_t content_length);


// **************************************************************************
// �ե�������Τ��ֿ���
// �إå���������������
// **************************************************************************
int http_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	int	send_header_data_len;
	int	result_len;

	unsigned char	send_http_header_buf[2048];
	unsigned char	work_buf[1024];

	off_t	content_length;
	off_t	written;
	off_t	offset;

	struct	stat	file_stat;
	int				result;

	int mpeg2video = (strstr(http_recv_info_p->mime_type, "video/mpeg") != NULL);

	// ---------------
	// ������ѿ������
	// ---------------
	memset(send_http_header_buf, '\0', sizeof(send_http_header_buf));

/*
	if ( http_recv_info_p->range_start_pos == 0 )
	{
		content_length = 0;
	}
*/
	result = stat(http_recv_info_p->send_filename, &file_stat); // �ե����륵���������å���
	if ( result != 0 )
	{
		debug_log_output("file not found.");
		return ( -1 );
	}

	// Set the starting offset based on the requested page
	offset = (file_stat.st_size/10) * http_recv_info_p->page;

	if(    global_param.bookmark_threshold
	    && (http_recv_info_p->file_size > global_param.bookmark_threshold)
	    && (http_recv_info_p->range_start_pos == 0)) {
		// Check for a bookmark
		FILE	*fp;
		int previous_page = 0;
		off_t previous_size = 0;
		off_t bookmark = 0;
		snprintf(work_buf, sizeof(work_buf), "%s.wizd.bookmark", http_recv_info_p->send_filename);
		debug_log_output("Checking for bookmark: '%s'", work_buf);
		fp = fopen(work_buf, "r");
		if(fp != NULL) {
			fgets(work_buf, sizeof(work_buf), fp);
			bookmark = atoll(work_buf);
			if(fgets(work_buf, sizeof(work_buf), fp) != NULL) {
				previous_size = atoll(work_buf);
				if(fgets(work_buf, sizeof(work_buf), fp) != NULL) {
					previous_page = atoi(work_buf);
				}
			}
			fclose(fp);
			debug_log_output("Bookmark offset: %lld/%lld (page %d)", bookmark, previous_size, previous_page);
			debug_log_output("Requested range %lld-%lld", http_recv_info_p->range_start_pos, http_recv_info_p->range_end_pos);
		}
		// Compare the current request with the stored bookmark
		// If requesting the same chapter as before, then don't adjust anything
		// If reqesting an earlier chapter, then go there directly
		if(http_recv_info_p->page < previous_page) {
			// Make sure we never advance the position on a previous-chapter command
			if((bookmark > 0) && (offset > bookmark)) {
				debug_log_output("Forcing offset from %lld to %lld to prevent advance when going to previous chapter", offset, bookmark);
				offset = bookmark;
			} else {
				debug_log_output("Going to previous chapter - no bookmark adjustment needed");
			}
		} else if(http_recv_info_p->page > previous_page) {
			// If the file size is increasing, and bookmark is equal to the previous end-of-file
			// then adjust things so it continues playing where it left off
			// This will allow for (almost) continuous playback of files of increasing size
			if((previous_size > 0) && (file_stat.st_size > previous_size) && (offset < bookmark)) {
				debug_log_output("Forcing offset from %lld to %lld to get continuous playback", offset, bookmark);
				offset = bookmark;
			} else {
				// If requesting a following chapter, then return content_length=1 until we are beyond the bookmark location
				// (Note: I tried content_length=0, but this seems to really confuse the LinkPlayer!!!)
				if((global_param.dummy_chapter_length > 0) && (offset < bookmark)) {
					content_length = global_param.dummy_chapter_length;
					debug_log_output("Returning length=%lld because offset < bookmark (%lld < %lld)",
						content_length, offset, bookmark);
					http_recv_info_p->range_start_pos = offset;
					http_recv_info_p->range_end_pos = offset + content_length - 1;
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
					debug_log_output("Advancing to chapter %d, offset=%lld, bookmark=%lld", http_recv_info_p->page, offset, bookmark);
				}
			}
		} else if(previous_size > 0) {
			// When requesting the same chapter, then compute the offset using the previously saved length
			// so that fast-forward/rewind isn't affected by changing file sizes
			offset = (previous_size/10) * http_recv_info_p->page;
			debug_log_output("Forcing offset=%lld based on previous size (%lld) instead of current size (%lld)",
				offset, previous_size, file_stat.st_size);
		}
	}

	if(offset > 0) {
		debug_log_output("Offsetting start/end position by %lld", offset);
		http_recv_info_p->range_start_pos += offset;
		if(http_recv_info_p->range_end_pos > 0)
			http_recv_info_p->range_end_pos += offset;
	}
	if ( http_recv_info_p->range_end_pos > 0 )	// end���ֻ���ͭ�ꡣ
	{
		content_length = (http_recv_info_p->range_end_pos - http_recv_info_p->range_start_pos) + 1;
	} 
	else 
	{
		content_length = file_stat.st_size - http_recv_info_p->range_start_pos;
	}

	// --------------
	// OK �إå�����
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
	// �إå��ֿ�
	// --------------
	result_len = send(accept_socket, send_http_header_buf, send_header_data_len, 0);
	debug_log_output("result_len=%d, send_data_len=%d\n", result_len, send_header_data_len);


	// Small files go to simple function
	if((content_length > 0) && (content_length <= SEND_BUFFER_SIZE)) {
		written = http_simple_file_send( accept_socket, http_recv_info_p->send_filename,
								content_length,
								http_recv_info_p->range_start_pos );
	} else {
		// --------------
		// �����ֿ�
		// --------------
		written = http_file_send(	accept_socket, 	http_recv_info_p->send_filename,
									content_length,
									http_recv_info_p->range_start_pos );
	}

	if(global_param.bookmark_threshold && (mpeg2video || !global_param.flag_bookmarks_only_for_mpeg)) {
		/* if(  (written < global_param.bookmark_threshold) // Less than 10 MB written
		  ||  (content_length && ((written+global_param.bookmark_threshold) > content_length)) //within 10 MB of the end of file
		  ||  (0 == strcmp(http_recv_info_p->mime_type, "video/mpeg")) ) { // Only create bookmarks for MPEG video files
			// Remove any existing bookmark
			snprintf(work_buf, sizeof(work_buf), "%s.wizd.bookmark", http_recv_info_p->send_filename);
			if(0 == unlink(work_buf)) {
				debug_log_output("Removed bookmark: '%s'", work_buf);
			}
		} else */
		if(written > global_param.bookmark_threshold) {
			// Save a bookmark
			snprintf(work_buf, sizeof(work_buf), "%s.wizd.bookmark", http_recv_info_p->send_filename);
			written += http_recv_info_p->range_start_pos;
			// Back up to account for network buffering in the player
			// (don't back up if the bookmark is at the end of the file)
			if(written < file_stat.st_size)
				written -= global_param.bookmark_threshold;
			if(written > 0) { // always true here!
				FILE *fp = fopen(work_buf, "w");
				if(fp != NULL) {
					// Write the bookmark location, the size (as we saw it), and the requested chapter number
					// which will be used to decide what to do when we receive the next request
					fprintf(fp, "%lld\n", written);
					fprintf(fp, "%lld\n", file_stat.st_size);
					fprintf(fp, "%d\n", http_recv_info_p->page);
					fclose(fp);
					debug_log_output("Wrote bookmark: '%s' page %d %lld of %lld", work_buf, http_recv_info_p->page, written, file_stat.st_size);
				} else {
					debug_log_output("create bookmark failed. err = %s", strerror(errno));
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
// �ե�����μ��Τ������¹���
// **************************************************************************
static off_t http_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos )
{
	off_t			seek_ret;
	off_t		written=0;

	// ---------------------
	// �ե����륪���ץ�
	// ---------------------
	if(global_param.buffer_size < 2) {
		FILE *fp = fopen(filename, "rb");
		if ( fp == NULL )
		{
			debug_log_output("errno = %s\n", strerror(errno));
			debug_log_output("fopen() error. file = '%s'", filename);
			return ( 0 );
		}
		seek_ret = fseek(fp, range_start_pos, SEEK_SET);
		if ( seek_ret < 0 )	// lseek ���顼�����å�
		{
			debug_log_output("fseek() error.");
			fclose(fp);
			return ( 0 );
		}
		written = copy_FILE_to_descriptor(fp, accept_socket, content_length);
		fclose(fp);	// File Close.
	} else {
		int fd = open(filename, O_RDONLY);
		if ( fd < 0 )
		{
			debug_log_output("errno = %s\n", strerror(errno));
			debug_log_output("open() error. file = '%s'", filename);
			return ( 0 );
		}

		// ------------------------------------------
		// range_start_pos�إե����륷����
		// ------------------------------------------
		seek_ret = lseek(fd, range_start_pos, SEEK_SET);
		if ( seek_ret < 0 )	// lseek ���顼�����å�
		{
			debug_log_output("lseek() error.");
			close(fd);
			return ( 0 );
		}
		written = copy_descriptors(fd, accept_socket, content_length, NULL);
		close(fd);
	}

	// ���ｪλ
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
 * �Хåե���󥰤��ʤ��� in_fd ���� out_fd �� �ǡ�����ž��
 * �񤭹���¦�� NONBLOCK �⡼�ɤ����ꤹ�뤳�Ȥˤ�ä�
 * write(send?) ��˥֥�å�����Τ��ɤ���
 * �������뤳�Ȥǡ��ɤ߹��ߤ��Ǥ�����ˤϤ���˥���å���˥ǡ�����
 * ����뤳�Ȥ��Ǥ��롣
 *
 * ���ΤȤ����Υե�������������ȡ��ץ�������ƤӽФ��Ƥ���.
 * content_length �򤢤ޤ���׻뤷�Ƥ��ʤ�..
 *
 * vob��Ϣ³�������б��Τ��ᡢʣ���ե������Ϣ³ž�����б�
 *
 */
//#define USE_PTHREAD
#ifdef USE_PTHREAD

typedef struct thread_param_tag {
	int flags;
	int fd;
	off_t written;
	int count;
	int length;
	pthread_mutex_t *mutex;
	int *bytes;
	off_t *total;
	char **buffer;
} thread_param_type;

#define FLAG_ABORT 1
#define FLAG_READ_COMPLETE 2
#define FLAG_WRITE_COMPLETE 4
#include <pthread.h>

void *send_buffers(void *p)
{
	thread_param_type *param = (thread_param_type *)p;
	int last_index = param->count-1;
	int index=0;
	int i,len;
	int flag_verbose = 0; // Set to 1 for debugging sessions, set to 0 for release

	if((param == NULL) || (param->count <= 0) || (param->length <= 0))
		return NULL;

	// Lock the last block to prevent overrun
	pthread_mutex_lock(&param->mutex[last_index]);

	debug_log_output("Threaded send started");

	while((param->flags & (FLAG_ABORT|FLAG_WRITE_COMPLETE)) == 0) {
		if(pthread_mutex_lock(&param->mutex[index])==0) {

			if(param->bytes[index] > 0) {
				if(flag_verbose && (param->flags & FLAG_READ_COMPLETE)) {
					debug_log_output("Sending block %d with %d bytes\n", index, param->bytes[index]);
				}
				// We have a buffer with data - send it out
				for(i=0; i<param->bytes[index]; ) {
					len = param->bytes[index] - i;
					if(len > global_param.socket_chunk_size)
						len = global_param.socket_chunk_size;
					len = send(param->fd, param->buffer[index] + i, len, 0);
					// Free up the mutex after we complete a send, so file read is triggered
					// after we have fresh data going out the network
					if(last_index >= 0) {
						pthread_mutex_unlock(&param->mutex[last_index]);
						last_index=-1;
					}
					if(len <= 0) {
						// Failed to write - end the stream
						if (len < 0) {
							debug_log_output("send failed. err = %s", strerror(errno));
						} else {
							debug_log_output("socket closed by player");
						}
						param->flags |= FLAG_ABORT;
						break;
					} else {
						i += len;
						param->written += len;
					}
				}
				if(param->written != param->total[index]) {
					debug_log_output("Read/send mismatch, read=%lld, sent=%lld, buffer=%d, sent=%d",
						param->total[index], param->written, param->bytes[index], i);
				}
			} else if((param->flags & FLAG_READ_COMPLETE) == FLAG_READ_COMPLETE) {
				// No more data - so flag that we are finished sending
				param->flags |= FLAG_WRITE_COMPLETE;
			} else if(flag_verbose) {
				debug_log_output("ERROR: Got a zero-length block without a read-complete flag!!!");
			}
			// Flag this block as complete
			param->bytes[index] = 0;
			// Free up the previous block if it hasn't been freed yet
			if(last_index >= 0) {
				pthread_mutex_unlock(&param->mutex[last_index]);
				last_index=-1;
			}
			last_index=index; // Release this block only after we acquire the next

			if(++index >= param->count)
				index = 0;
		} else {
			debug_log_output("Failed to lock mutex %d, aborting!!!", index);
			param->flags |= FLAG_ABORT;
		}
	}
	// Release the last block
	pthread_mutex_unlock(&param->mutex[last_index]);

	// Count the number of bytes in the queue
	for(i=0,len=0; i<param->count; i++) {
		len += param->bytes[i];
	}
	debug_log_output("Send complete, sent %lld bytes, %d bytes left in the buffer", param->written, len);

	return NULL;
}

off_t copy_descriptors(int in_fd, int out_fd, off_t content_length, JOINT_FILE_INFO_T *joint_file_info_p)
{
	int i;

	pthread_t id;
	pthread_attr_t attr;
	thread_param_type param;

	int index = 0;
	int next_index;
	int oneback,twoback,shortcount;

	off_t	total_read_size = 0;
	int	len;

	off_t read_cnt = 0;

	int flag_verbose = 0; // Set to 1 for debugging sessions, set to 0 for release
	int flag_send_asap = global_param.flag_buffer_send_asap;

	ssize_t blocks_read = 0;
	int offset = 0;
	
	if(global_param.buffer_size < 1) global_param.buffer_size = 1;
	if(global_param.stream_chunk_size < 512) global_param.stream_chunk_size = 512;

	if (joint_file_info_p) offset = joint_file_info_p->iso_seek;
	
	if(flag_verbose && global_param.flag_debug_log_output) {
		// Redirect streaming debug log to stderr
		debug_log_initialize(NULL);
	}
	// ======================
	// �����Хåե������
	// ======================

	param.count = global_param.buffer_size;

	// Need at least 4 buffers for this to be reasonable
	if(param.count < 4)
		param.count = 4;

	param.length = global_param.stream_chunk_size;

	debug_log_output("Allocating %d buffers of %d bytes each\n", param.count, param.length);

	param.bytes = (int *)malloc(param.count * sizeof(int));
	param.total = (off_t *)malloc(param.count * sizeof(off_t));
	param.buffer = (char **)malloc(param.count * sizeof(char *));
	param.mutex = (pthread_mutex_t *)malloc(param.count * sizeof(pthread_mutex_t));

	if ( ( param.bytes == NULL ) || ( param.buffer == NULL ) || ( param.mutex == NULL ) )
	{
		debug_log_output("malloc() error.\n");
		return (-1 );
	}

	for(i=0; i<param.count; i++) {
		param.bytes[i] = 0;
		param.buffer[i] = (char *)malloc(param.length);
		if(param.buffer[i] == NULL) {
			debug_log_output("malloc() error.\n");
			return ( 0 );
		}
		pthread_mutex_init(&param.mutex[i], NULL);
	}

	// Launch a separate thread to send the data out
	param.fd = out_fd;
	param.flags = 0;
	param.written = 0;

	// Lock the next block in advance
	pthread_mutex_lock(&param.mutex[index]);

	pthread_attr_init(&attr);

// This doesn't seek to work well under Cygwin
//#define USE_PTHREAD_PRIORITY
#ifdef USE_PTHREAD_PRIORITY
	{
		int policy,retcode,curr,mn,mx;
		struct sched_param schp;

		memset(&schp, 0, sizeof(schp));
		// Get the current settings
		pthread_getschedparam(pthread_self(), &policy, &schp);
		curr=schp.sched_priority;
		mn=sched_get_priority_min(policy);
		mx=sched_get_priority_max(policy);
		schp.sched_priority = (curr+mx)/2;
		debug_log_output("Setting policy %d, priority %d (was=%d, min=%d, max=%d)\n", policy, schp.sched_priority, curr, mn, mx);
		retcode = pthread_attr_setschedpolicy(&attr,policy);
		if(retcode != 0) {
			debug_log_output("pthread_attr_setschedpolicy returned %d\n",retcode);
		}
		retcode = pthread_attr_setschedparam(&attr,&schp);
		if(retcode != 0) {
			debug_log_output("pthread_attr_setschedparam returned %d\n",retcode);
		}
	}
#endif
	if((i = pthread_create(&id, &attr, send_buffers, &param)) != 0) {
		debug_log_output("pthread_create returned %d\n",i);
		return ( 0 );
	}

	// Watch the two bins behind, and shorten the buffer if the write thread is catching up
	twoback = param.count - 2;
	oneback = param.count - 1;
	shortcount = param.length/4;
	if(shortcount < 4096)
		shortcount = 4096;

	while ( param.flags == 0 )
	{
		// Wait for a buffer to free up
		next_index = index+1;
		if(next_index >= param.count)
			next_index = 0;

		if(pthread_mutex_lock(&param.mutex[next_index]) == 0) {
			for(i=0 ; i < param.length; ) {
				len = param.length - i;
				if(len > global_param.file_chunk_size)
					len = global_param.file_chunk_size;
				if ((joint_file_info_p!=NULL) && (joint_file_info_p->dvd_file != NULL)) {
					blocks_read = DVDReadBlocks(joint_file_info_p->dvd_file, offset, len/2048, param.buffer[index] + i); 
					len = (off_t)blocks_read*2048;
					offset += blocks_read;
				} else if (joint_file_info_p && read_cnt >= joint_file_info_p->file[joint_file_info_p->current_file_num].size) {
					len = 0;
					read_cnt = 0;
					debug_log_output("finished file chunk %d\n", joint_file_info_p->current_file_num);
					//printf("finished file chunk %d\n", joint_file_info_p->current_file_num);
				} else {
					if (joint_file_info_p && len + read_cnt > joint_file_info_p->file[joint_file_info_p->current_file_num].size) {
						len = joint_file_info_p->file[joint_file_info_p->current_file_num].size - read_cnt;
						debug_log_output("finishing last block of %d\n", joint_file_info_p->current_file_num);
						//printf("finishing last block of %d\n", joint_file_info_p->current_file_num);
					}

					len = read(in_fd, param.buffer[index] + i, len);
					read_cnt += len;
				}
				if(len == 0) {
					// End of file
					if(    (joint_file_info_p==NULL)
						|| (joint_file_info_p->dvd_file != NULL)
						|| next_file(&in_fd, joint_file_info_p)) {
						// No more files
						if (flag_verbose) {
							debug_log_output("recv: len = %d, idx = %d finish!", len, index);
						} else {
							debug_log_output(
							   "*********************************************************\n"
							   "**                    RECV FINISHED!                   **\n"
							   "*********************************************************");
						}
						param.flags |= FLAG_READ_COMPLETE;
						break;
					} else {
						// We opened up a new file, so continue the loop
					}
				} else if (len > 0) {
					i += len;
					total_read_size += len;
					if((content_length > 0) && (total_read_size >= content_length)) {
						// Only send as much as is requested
						if(total_read_size > content_length) {
							debug_log_output("Restricting content length, read=%lld, allowed=%lld", total_read_size, content_length);
							total_read_size -= content_length;
							i -= (int)total_read_size;;
							total_read_size = content_length;
						}
						param.flags |= FLAG_READ_COMPLETE;
						break;
					}
				} else {
					// Read error
					if (flag_verbose) {
						debug_log_output("read err?: len = %d, idx = %d, err: %s", len, index, strerror(errno));
					} else {
						debug_log_output(
					   "*********************************************************\n"
					   "**                    RECV FINISHED!(ret = %d)          **\n"
					   "*********************************************************", len);
					}
					param.flags |= FLAG_READ_COMPLETE;
					break;
				}
				// Check the trailing bins to see if the write thread is catching us
				// with the exception that if we already fell behind, 
				// then we allow it to rebuffer completely for one block
				if((i >= shortcount) && (param.bytes[twoback] == 0) && (total_read_size > param.length)) {
					if(param.bytes[oneback] == 0) {
						//fputc('.',stderr); // Only one buffer full - reloading
					} else {
						//fputc(':',stderr); // Only two buffers full, cut this buffer short
						break;
					}
				}
			}
			// Flag this block as ready-to-write
			param.bytes[index] = i;
			param.total[index] = total_read_size;
			if(flag_send_asap) {
				pthread_mutex_unlock(&param.mutex[index]);
			} else if(next_index == (param.count-2)) {
				// Buffers are filled, so release the sending thread
				flag_send_asap = TRUE;
				for(i=0; i<next_index; i++)
					pthread_mutex_unlock(&param.mutex[i]);
			}

			twoback = oneback;
			oneback = index;
			index = next_index;
		}
	}

	// Count the number of bytes in the queue
	for(i=0,len=0; i<param.count; i++) {
		len += param.bytes[i];
	}
	debug_log_output("Read complete, read %lld bytes, %d bytes to transmit", total_read_size, len);

	// If we haven't released the sending thread yet, do so now
	if(!flag_send_asap) {
		for(i=0; i<index; i++)
			pthread_mutex_unlock(&param.mutex[i]);
	}

	// Release the last block, unused
	pthread_mutex_unlock(&param.mutex[index]);

	// Wait for the created thread to complete
	pthread_join(id,NULL);

	// Free up the allocated memory
	if(param.bytes != NULL)
		free(param.bytes);
	for(i=0; i<param.count; i++) {
		if(param.buffer[i] != NULL)
			free(param.buffer[i]);
		pthread_mutex_destroy(&param.mutex[i]);
	}
	debug_log_output("copy descriptors end.");

	return param.written;
}

#else // not USE_PTHREAD

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
	off_t read_cnt = 0;

	off_t			total_read_size = 0;
	size_t			target_read_size;
	int				len;

	int flag_finish = 0;
	int flag_first_time = 0;
	int flag_verbose = 0;
	time_t first_timeout = 0;
	off_t written = 0;
	
	struct timeb marker1,marker2; //,marker3;
	int marker_count;

	if(global_param.buffer_size < 1) global_param.buffer_size = 1;
	if(global_param.stream_chunk_size < 512) global_param.stream_chunk_size = 512;

	ssize_t blocks_read = 0;
	int offset = 0;
	if (joint_file_info_p) offset = joint_file_info_p->iso_seek;
	
	// ======================
	// �����Хåե������
	// ======================

	debug_log_output("Allocating %d buffers of %d bytes each\n", global_param.buffer_size, global_param.stream_chunk_size);

	buff = malloc(global_param.buffer_size * sizeof(struct _buff));
	if ( buff == NULL )
	{
		debug_log_output("malloc() error.\n");
		return (-1 );
	}
	p = malloc(global_param.buffer_size * global_param.stream_chunk_size);
	if ( p == NULL )
	{
		debug_log_output("malloc() error.\n");
		return ( 0 );
	}
	for (i=0; i<global_param.buffer_size; i++) {
		buff[i].pos = 0;
		buff[i].inuse = 0;
		buff[i].len = 0;
		buff[i].p = p + i*global_param.stream_chunk_size;
	}

	// �֥�å��⡼�ɤ�����
	//set_blocking_mode(in_fd, 0);	/* blocking */
	set_blocking_mode(out_fd, (global_param.buffer_size>1));	/* non-blocking if multiple buffers */
	//debug_log_output("set non-blocking mode");

	// Do the calculation this way so global_param.buffer_size<4 always gets flag_first_time
	//No, don't force this!!!  :  if (content_length < global_param.stream_chunk_size * (global_param.buffer_size>>2)) flag_first_time = 1;
	if (global_param.buffer_size < 4) flag_first_time = 1;
	// If only one buffer, then we work the same as the http_simple_file_send()
	if (global_param.buffer_size == 1) global_param.flag_buffer_send_asap = 1;

	ftime(&marker1);
	marker_count = 0;

	first_timeout = time(NULL);
	// ================
	// ����ž������
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
		if (flag_finish || flag_first_time || (idx_count >= (global_param.buffer_size-2))) 
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
						(buff[write_idx].len < global_param.socket_chunk_size) ? buff[write_idx].len : global_param.socket_chunk_size );
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
								/* fprintf(stderr, "%g Mbps\n", rate); */
							}
							marker1 = marker2;
							marker_count = 0;
						}

						if (flag_verbose) {
							debug_log_output("sent: len =%6d, idx = %4d, idxcount = %4d", len, write_idx, idx_count);
						} else if(0 && !global_param.flag_daemon) {
							show_progress(idx_count * 100 / global_param.buffer_size, idx_count);
						}
						if (buff[write_idx].len <= 0) {
							buff[write_idx].inuse = 0;
							buff[write_idx].len = 0;
							buff[write_idx].pos = 0;
							write_idx = (write_idx + 1) % global_param.buffer_size;
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
							debug_log_output("write failed after %d bytes. err = %s (%d)", written, strerror(errno), (int)errno);
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
			target_read_size = global_param.stream_chunk_size - buff[read_idx].len;
/*
			if (buff[read_idx].p == NULL) {
				debug_log_output("error! idx: %d", read_idx);
			}
*/
ftime(&marker2);
			if ((joint_file_info_p!=NULL) && (joint_file_info_p->dvd_file != NULL)) {
				blocks_read = DVDReadBlocks(joint_file_info_p->dvd_file, offset, target_read_size/2048, buff[read_idx].p + buff[read_idx].len); 
				len = (off_t)blocks_read*2048;
				offset += blocks_read;
			} else if (joint_file_info_p && read_cnt >= joint_file_info_p->file[joint_file_info_p->current_file_num].size) {
				len = 0;
				read_cnt = 0;
				debug_log_output("finished file chunk %d\n", joint_file_info_p->current_file_num);
				//printf("finished file chunk %d\n", joint_file_info_p->current_file_num);
			} else {
				if (joint_file_info_p && target_read_size + read_cnt > joint_file_info_p->file[joint_file_info_p->current_file_num].size) {
					target_read_size = joint_file_info_p->file[joint_file_info_p->current_file_num].size - read_cnt;
					debug_log_output("finishing last block of %d\n", joint_file_info_p->current_file_num);
					//printf("finishing last block of %d\n", joint_file_info_p->current_file_num);
				}

				len = read(in_fd, buff[read_idx].p + buff[read_idx].len, target_read_size);
				read_cnt += len;
			}
//ftime(&marker3);
//i = (marker3.time-marker2.time)*1000 + (marker3.millitm-marker2.millitm);
//if(i>9) fputc('A'+i/10, stderr);
			if(len == 0) {
				if(    (joint_file_info_p==NULL)
					|| (joint_file_info_p->dvd_file != NULL)
					|| next_file(&in_fd, joint_file_info_p)) {
					// �ɤ߹��߽����
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
					// ���Υե������³��(�����Ǥϲ��⤷�ʤ�)
				}
			} else if (len > 0) {
//fputc(':', stderr);
				if (flag_verbose) {
					debug_log_output("recv: len =%6d, idx = %4d, idxcount = %4d", len, read_idx, idx_count);
				} else if(0 && !global_param.flag_daemon) {
					show_progress(idx_count * 100 / global_param.buffer_size, idx_count);
				}
				buff[read_idx].len += len;
				total_read_size += len;

				if (global_param.flag_buffer_send_asap == TRUE
				|| buff[read_idx].len >= global_param.stream_chunk_size) {
					buff[read_idx].inuse = 1;
					buff[read_idx].pos = 0;
					idx_count ++;
					read_idx = (read_idx + 1) % global_param.buffer_size;
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
				   "**                    RECV FINISHED!(ret = %d)          **\n"
				   "*********************************************************", len);
				}
			}
		}
	}

	free(p);
	free(buff);	// Memory Free.
	debug_log_output("copy descriptors end.");

	// ���ｪλ
	return written;
}
#endif // USE_PTHREAD

static int next_file(int *in_fd_p, JOINT_FILE_INFO_T *joint_file_info_p)
{
	off_t	ret;

	if (in_fd_p && joint_file_info_p)
	{
		// �ɤ߽���ä��ե������CLOSE()
		debug_log_output("[%02d] '%s' close()", joint_file_info_p->current_file_num, joint_file_info_p->file[joint_file_info_p->current_file_num].name);
		close(*in_fd_p);

		// ���Υե����뤬���뤫?
		joint_file_info_p->current_file_num++;
		if ( joint_file_info_p->current_file_num >= joint_file_info_p->file_num )
		{
			debug_log_output("EOF Detect.");
			//printf("EOF Detect.");
			return 1;		// ����ǽ�λ
		}

		// ���Υե������OPEN()
		debug_log_output("[%02d] '%s' open(), start_pos %lld\n", joint_file_info_p->current_file_num, joint_file_info_p->file[joint_file_info_p->current_file_num].name, joint_file_info_p->file[joint_file_info_p->current_file_num].start_pos);
		//printf("[%02d] '%s' open(), start_pos %lld, size %lld\n",
		//	joint_file_info_p->current_file_num,
		//	joint_file_info_p->file[joint_file_info_p->current_file_num].name,
		//	joint_file_info_p->file[joint_file_info_p->current_file_num].start_pos,
		//	joint_file_info_p->file[joint_file_info_p->current_file_num].size
		//	);

		*in_fd_p = open(joint_file_info_p->file[joint_file_info_p->current_file_num].name, O_RDONLY);
		if ( *in_fd_p < 0 )
		{
			debug_log_output("errno = %s\n", strerror(errno));
			debug_log_output("open() error. '%s'", joint_file_info_p->file[joint_file_info_p->current_file_num].name);
			return ( -1 );
		}

		ret = lseek(*in_fd_p, joint_file_info_p->file[joint_file_info_p->current_file_num].start_pos, SEEK_SET);

		debug_log_output("seek to %lld returned %lld\n", joint_file_info_p->file[joint_file_info_p->current_file_num].start_pos, ret);
		//printf("seek to %lld returned %lld\n", joint_file_info_p->file[joint_file_info_p->current_file_num].start_pos, ret);

		// �֥�å��⡼�ɤ�����
		set_blocking_mode(*in_fd_p, 0);	/* blocking */

		return 0;		// ���Υե�����ν�����λ
	} else {
		// �ѥ�᡼����NULL�ξ��ˤ�1�ե�����Τߤν����Ȥ���
		return 1;		// ����ǽ�λ
	}
}

off_t copy_FILE_to_descriptor(FILE *in_fp, int out_fd, off_t content_length)
{
	unsigned char *p;

	int		n;
	int		len;
	int		count;
	int		buflen;

	off_t	written = 0;
	off_t	remain = content_length;
	
	if(global_param.stream_chunk_size < 512) global_param.stream_chunk_size = 512;
	
	// ======================
	// �����Хåե������
	// ======================

	buflen = global_param.stream_chunk_size;
	p = malloc(buflen);
	if ( p == NULL )
	{
		debug_log_output("malloc() error.\n");
		return ( 0 );
	}

	debug_log_output("Sending %lld bytes", content_length);

	// If we have no size specified, send an "infinite" number of bytes, or until EOF
	if(remain == 0)
		remain = (off_t)(-1);

	// ================
	// ����ž������
	// ================
	while ( remain )
	{
		// Read a block from the file
		count = fread(p, 1, (remain > buflen) ? buflen : remain, in_fp); 
		if(count > 0) {
			remain -= count;
			// Write this data to the socket
			for(n=0; n < count; n += len) {
				len = write(out_fd, p+n, ((count-n) < global_param.socket_chunk_size) ? (count-n) : global_param.socket_chunk_size);
				if(len <= 0) {
					// Failed to write - end the stream
					remain = 0;
					if(len < 0) {
						debug_log_output("write failed. err = %s", strerror(errno));
					} else {
						debug_log_output("socket closed by player");
					}
					break;
				} else {
					written += len;
				}
			}
		} else {
			if(count < 0) {
				debug_log_output("read failed. err = %s", strerror(errno));
			} else {
				debug_log_output("EOF");
				// Open the next file in the sequence...
			}
			remain = 0;
		}
	}
	debug_log_output("Wrote %llu bytes", written);

	free(p);

	// ���ｪλ
	return written;
}


// **************************************************************************
// �ե�����μ��Τ������¹���
// **************************************************************************
off_t http_simple_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos )
{
	int		fd;

	unsigned char 	*send_buf_p;

	ssize_t			file_read_len;
	int				data_send_len;
	off_t			seek_ret;

	off_t			total_read_size;
	size_t			target_read_size;

	// ======================
	// �����Хåե������
	// ======================

	send_buf_p = malloc(SEND_BUFFER_SIZE);
	if ( send_buf_p == NULL )
	{
		debug_log_output("malloc() error.\n");
		return ( 0 );
	}

	// ---------------------
	// �ե����륪���ץ�
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
	// range_start_pos�إե����륷����
	// ------------------------------------------
	seek_ret = lseek(fd, range_start_pos, SEEK_SET);
	if ( seek_ret < 0 )	// lseek ���顼�����å�
	{
		debug_log_output("errno = %s\n", strerror(errno));
		debug_log_output("lseek() error.");
		free(send_buf_p);
		close(fd);
		return ( 0 );
	}


	total_read_size = 0;

	// ================
	// ����ž������
	// ================
	while ( (content_length==0) || (total_read_size < content_length) )
	{
		// ����Хåե����ꥢ
		memset(send_buf_p, 0, SEND_BUFFER_SIZE);

		// ��ɸread�������׻�
		if ( (content_length - total_read_size) > SEND_BUFFER_SIZE )
		{
			target_read_size = SEND_BUFFER_SIZE;
		}
		else
		{
			target_read_size = (size_t)(content_length - total_read_size);
		}


		// �ե����뤫��ǡ������ɤ߹��ࡣ
		file_read_len = read(fd, send_buf_p, target_read_size);
		if ( file_read_len <= 0 )
		{
			debug_log_output("EOF detect.\n");
			break;
		}



		// SOCKET �˥ǡ���������
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

	// ���ｪλ
	return total_read_size;
}



