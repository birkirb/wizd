// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_detect.c
//											$Revision: 1.4 $
//											$Date: 2004/06/27 06:50:00 $
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
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>


#include "wizd.h"


#define		SSDP_IP_ADDRESS		"239.255.255.250"
#define		SSDP_PORT			(1900)

#define		SSDP_CHECK_KEY		"Server"
#define		SSDP_CHECK_VALUE	"Syabas myiBox"

#define debug_log_output (void)

// =============================================================
// MediaWiz������
// =============================================================
void	server_detect()
{
    int    ret;

	int		ssdp_socket;	// SSDP �Ԥ�����Socket
    struct 	sockaddr_in		ssdp_server_addr;		// ������¦�����åȥ��ɥ쥹��¤��
    struct 	sockaddr_in		ssdp_client_addr;		// ���������åȥ��ɥ쥹��¤��
	struct	ip_mreq			ssdp_mreq;				// �ޥ�����㥹������

	int		ssdp_client_addr_len = sizeof(ssdp_client_addr);


	int		sock_opt_val;
	int		recv_len;

	int						regist_socket;
	struct 	sockaddr_in		connect_addr, self_addr;
	int						self_addr_len;
	int		flag_media_wiz;

    unsigned char	recv_buf[1024*2];
	unsigned char	send_buf[1024*2];
	unsigned char	work_buf[1024];

	unsigned char	line_buf[1024];
	unsigned char	ssdp_check_key_buf[1024];
	unsigned char	*work_p;


	unsigned char	self_ip_address[32];

	debug_log_output("AutoDetect Process start.\n");

    // =============================
    // �����å�����
    // =============================
	ssdp_socket = socket(AF_INET, SOCK_DGRAM, 0 );
	if ( ssdp_socket < 0 ) // �����å��������ԥ����å�
	{
		debug_log_output("AutoDetect: socket() error.");
		exit( 1 );
    }

    // REUSEADDR ����
    sock_opt_val = 1;
    setsockopt(ssdp_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt_val, sizeof(sock_opt_val));


    // ===========================================
    // �����åȥ��ɥ쥹��¤�Τ��ͤ򥻥å�
    // ===========================================
	memset( (char *)&ssdp_server_addr, 0, sizeof(ssdp_server_addr) );
    ssdp_server_addr.sin_family = AF_INET;
    ssdp_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ssdp_server_addr.sin_port = htons(SSDP_PORT);


    // =============================
    // bind �¹�
    // =============================
    ret = bind(ssdp_socket, (struct sockaddr *)&ssdp_server_addr, sizeof(ssdp_server_addr));
    if ( ret < 0 ) // bind ���ԥ����å�
    {
		debug_log_output("AutoDetect: bind() error.");
		exit( 1 );
    }


	// ===============================
	// �����åȤ˥��ץ����򥻥å�
	// ===============================


	// SSDP�ޥ�����㥹�� ���Х��å�����
	ssdp_mreq.imr_multiaddr.s_addr = inet_addr(SSDP_IP_ADDRESS);

	if ( strlen(global_param.auto_detect_bind_ip_address) == 0 )
	{
		debug_log_output("SSDP Listen:INADDR_ANY");
		ssdp_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	}
	else
	{
		debug_log_output("SSDP Listen:%s", global_param.auto_detect_bind_ip_address);
		ssdp_mreq.imr_interface.s_addr = inet_addr(global_param.auto_detect_bind_ip_address);
	}
	setsockopt(ssdp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&ssdp_mreq, sizeof(ssdp_mreq)); // ���Х��å�


	// =====================
	// �ᥤ��롼��
	// =====================
    while ( 1 )
    {
		// SSDP�ѥ��åȼ���
		memset(recv_buf, '\0', sizeof(recv_buf));
   		recv_len = recvfrom(ssdp_socket, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&ssdp_client_addr, &ssdp_client_addr_len );

		debug_log_output("=============== SSDP Receive. ================");

		// ssdp_client_addr ����ɽ��
		debug_log_output("SSDP client addr = %s\n", inet_ntoa(ssdp_client_addr.sin_addr) );
		debug_log_output("SSDP client port = %d\n", ntohs(ssdp_client_addr.sin_port) );

		// ������ä����Ƥ�ɽ��
		debug_log_output("SSDP recv_len=%d, recv_buf=(ry ", recv_len);
		//debug_log_output("SSDP recv = '%s'", recv_buf );



		// ======================
		// MediaWiz�������å���
		// ======================
		flag_media_wiz = 0;


		debug_log_output("");
		debug_log_output("--- SSDP ReceiveData Info Start. -----------------------------");

		work_p = recv_buf;
		while ( work_p != NULL )
		{
			// �����Хåե����顢�����ڤ�Ф���
			work_p = buffer_distill_line(work_p, line_buf, sizeof(line_buf) );
			debug_log_output("recv:'%s'", line_buf );

			//  ':'����(Key)���ڤ�Ф�
			strncpy(ssdp_check_key_buf, line_buf, sizeof(ssdp_check_key_buf) );
			cut_after_character( ssdp_check_key_buf, ':');

			// Key �� "Server" �������å�
			if ( strcasecmp(ssdp_check_key_buf, SSDP_CHECK_KEY ) == 0 ) 
			{
				// value�� SSDP_CHECK_VALUE ���ޤޤ�뤫�����å���
				cut_before_character(line_buf, ':');
				if ( strstr(line_buf, SSDP_CHECK_VALUE) != NULL )
				{
					// �ޤޤ�Ƥ���MediaWiz�Ǥ��롣
					debug_log_output( "HIT!!  This is MediaWiz!!", SSDP_CHECK_VALUE );

					flag_media_wiz = 1;
					break;
				}
			}
		}
		debug_log_output("--- SSDP ReceiveData Info End. -----------------------------");
		debug_log_output("");

		// MediaWiz����ʤ��ä����ʤˤ⤷�ʤ���
		if ( flag_media_wiz == 0 ) 
		{
			debug_log_output("Not MeidaWiz.");
			debug_log_output("=============== SSDP process end. ================ \n");
			continue;
		}



		// ======================
		// ��ư��Ͽ�¹�
		// ======================


	    // ----------------------------
	    // ��Ͽ�ѥ����å�����
	    // ----------------------------
		regist_socket = socket(AF_INET, SOCK_STREAM, 0 );
		if ( regist_socket < 0 ) // �����å��������ԥ����å�
		{
			debug_log_output("AutoDetect: socket() error.");
			continue;
	    }



		// ----------------------
		// MediaWiz��CONNECT�¹�
		// ----------------------
		memset(&connect_addr, 0, sizeof(connect_addr) );
		connect_addr.sin_family = AF_INET;
		connect_addr.sin_addr = ssdp_client_addr.sin_addr;
		connect_addr.sin_port = ssdp_client_addr.sin_port;

		ret = connect(regist_socket, (struct sockaddr *)&connect_addr, sizeof(connect_addr));
		if ( regist_socket < 0 )
		{
			debug_log_output("AutoDetect: connect() error.");
			continue;
		}

		// ------------------------------------
		// MediaWiz����³������ʬ���Ȥ�IP��Get.
		// ------------------------------------

		memset(&self_addr, 0, sizeof(self_addr) );
		self_addr_len = sizeof(self_addr);
		ret = getsockname(regist_socket, (struct sockaddr *)&self_addr, &self_addr_len );
		if ( ret < 0 ) // getsockname() ���顼�����å�
		{
			debug_log_output("getsockname() error. ret = %d", ret );
			continue;
		}

		strncpy( self_ip_address, inet_ntoa(self_addr.sin_addr), sizeof(self_ip_address) );
		debug_log_output("SSDP self addr = %s\n", self_ip_address );




		// --------------------------
		// MediaWiz�ؤ���������������
		// --------------------------
		debug_log_output("to MediaWiz Registration Start.");
		
		memset(send_buf, '\0', sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), 
				"GET /myiBoxUPnP/description.xml?POSTSyabasiBoxURLpeername=%s&peeraddr=%s:%d& HTTP/1.1\r\n",
					global_param.server_name, 
					self_ip_address, 
					global_param.server_port 			);

		snprintf(work_buf, sizeof(work_buf), "User-Agent: %s\r\n", SERVER_NAME);
		strncat(send_buf, work_buf, sizeof(send_buf) - strlen(send_buf));
		snprintf(work_buf, sizeof(work_buf), "Host: %s:%d\r\n", inet_ntoa(ssdp_client_addr.sin_addr), ntohs(ssdp_client_addr.sin_port) );
		strncat(send_buf, work_buf, sizeof(send_buf) - strlen(send_buf));
		strncat(send_buf, "\r\n", sizeof(send_buf) - strlen(send_buf));

		debug_log_output("%s", send_buf);


		// --------------------------
		// �����¹�
		// --------------------------
		send(regist_socket, send_buf, strlen(send_buf), 0);

		// --------------------------
		// �ֿ���������롣
		// --------------------------
		while( 1 ) 
		{
			recv_len = recv(regist_socket, recv_buf, sizeof(recv_buf), 0);
			if ( recv_len <= 0 ) 
				break;
			work_p = recv_buf;
			while ((work_p = buffer_distill_line(work_p, line_buf, sizeof(line_buf) )) != NULL) {
				debug_log_output("recv: %s", line_buf);
			}
		}


		// --------------------------
		// ��Ͽ��Socket��close.
		// --------------------------
		close(regist_socket); 


		debug_log_output("=============== SSDP process end. ================ \n");

    }


	return;
}


