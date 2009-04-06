// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_listen.c
//											$Revision: 1.6 $
//											$Date: 2004/03/06 13:52:56 $
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

int volatile child_count = 0;
#define MAX_CHILD_COUNT (global_param.max_child_count)

// **********************************************************************
// HTTP������ �Ԥ�����ư����
// **********************************************************************
void	server_listen()
{
	int    ret;
	int    pid;

	int		listen_socket;	// �Ԥ�����Socket
	int		accept_socket;	// ��³Socket

	struct sockaddr_in    saddr;		// �����Х����åȥ��ɥ쥹��¤��
	struct sockaddr_in    caddr;		// ���饤����ȥ����åȥ��ɥ쥹��¤��
	int    caddr_len = sizeof(caddr);	// ���饤����ȥ����åȥ��ɥ쥹��¤�ΤΥ�����

	int		sock_opt_val;


	int		access_check_ok;
	int			i;
	unsigned char	client_addr_str[32];
	unsigned char	client_address[4];
	unsigned char	masked_client_address[4];
	unsigned char	work1[32];
	unsigned char	work2[32];

	// =============================
	// listen�����å�����
	// =============================
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if ( listen_socket < 0 ) // �����å��������ԥ����å�
	{
		debug_log_output("socket() error.");
		perror("socket");
		exit( 1 );
	}

	// ===============================
	// SO_REUSEADDR�򥽥��åȤ˥��å�
	// ===============================
	sock_opt_val = 1;
	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt_val, sizeof(sock_opt_val));

	// ===========================================
	// �����åȥ��ɥ쥹��¤�Τ��ͤ򥻥å�
	// ===========================================
	memset( (char *)&saddr, 0, sizeof(saddr) );
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(global_param.server_port);

	// =============================
	// bind �¹�
	// =============================
	ret = bind(listen_socket, (struct sockaddr *)&saddr, sizeof(saddr));
	if ( ret < 0 ) // bind ���ԥ����å�
	{
		debug_log_output("bind() error. ret=%d\n", ret);
		perror("bind");
		return;
	}

	/* Check our send buffer size */
	sock_opt_val=0;
	i=sizeof(sock_opt_val);
	if(getsockopt(listen_socket, SOL_SOCKET, SO_SNDBUF, (char *)&sock_opt_val, &i)==0) {
		debug_log_output("default SO_SNDBUF=%d\n",sock_opt_val);
		if(global_param.stream_sndbuf && (sock_opt_val != global_param.stream_sndbuf)) {
			/* If a buffer size was requested, set it */
			sock_opt_val = global_param.stream_sndbuf;
			setsockopt(listen_socket, SOL_SOCKET, SO_SNDBUF, (char *)&sock_opt_val, sizeof(sock_opt_val));
			i = sizeof(sock_opt_val);
			if(getsockopt(listen_socket, SOL_SOCKET, SO_SNDBUF, (char *)&sock_opt_val, &i)==0) {
				debug_log_output("Changed SO_SNDBUF to %d\n",sock_opt_val);
			}
		}
	}

	/* Check our recv buffer size */
	sock_opt_val=0;
	i=sizeof(sock_opt_val);
	if(getsockopt(listen_socket, SOL_SOCKET, SO_RCVBUF, (char *)&sock_opt_val, &i)==0) {
		debug_log_output("default SO_RCVBUF=%d\n",sock_opt_val);
		if(global_param.stream_rcvbuf && (sock_opt_val != global_param.stream_rcvbuf)) {
			/* If a buffer size was requested, set it */
			sock_opt_val = global_param.stream_rcvbuf;
			setsockopt(listen_socket, SOL_SOCKET, SO_RCVBUF, (char *)&sock_opt_val, sizeof(sock_opt_val));
			i = sizeof(sock_opt_val);
			if(getsockopt(listen_socket, SOL_SOCKET, SO_RCVBUF, (char *)&sock_opt_val, &i)==0) {
				debug_log_output("Changed SO_RCVBUF to %d\n",sock_opt_val);
			}
		}
	}

	// =============================
	// listen�¹�
	// =============================
	ret = listen(listen_socket, LISTEN_BACKLOG);
	if ( ret < 0 )  // listen���ԥ����å�
	{
		debug_log_output("listen() error. ret=%d\n", ret);
		perror("listen");
		return;
	}

	// =====================
	// �ᥤ��롼��
	// =====================
	while ( 1 )
	{
		// ====================
		// Accept�Ԥ�.
		// ====================
		if (MAX_CHILD_COUNT > 0 && child_count >= MAX_CHILD_COUNT) {
			debug_log_output("Waiting for a child_count is going down... "
				"%d / %d\n", child_count, MAX_CHILD_COUNT);
			while (MAX_CHILD_COUNT > 0 && child_count >= MAX_CHILD_COUNT) {
				sleep(1);
			}
		}
		debug_log_output("Waiting for a new client...");
		accept_socket = accept(listen_socket, (struct sockaddr *)&caddr, &caddr_len);
		if ( accept_socket < 0 ) // accept���ԥ����å�
		{
			debug_log_output("accept() error. ret=%d\n", accept_socket);
			continue;		// �ǽ����롣
		}
		debug_log_output("\n\n=============================================================\n");
		debug_log_output("Socket Accept!!(accept_socket=%d)\n", accept_socket);

		child_count ++;
		rand();

		// ========================
		// fork�¹�
		// ========================
		pid = fork();
		if ( pid < 0 ) // fork���ԥ����å�
		{
			printf("fork() error. ret=%d\n", pid);
			close(accept_socket);	// ��ǰ�����������åȤ��Ĥ��롣
			continue;				// �ǽ����롣
		}

		if (pid == 0)
		{
			// �ʲ����ҥץ�����
			debug_log_output("fork success!!(pid=%d)\n", pid);

			// listen_socket�Ĥ��롣
			close(listen_socket);


			// caddr ����ɽ��
			debug_log_output("client addr = %s\n", inet_ntoa(caddr.sin_addr) );
			debug_log_output("client port = %d\n", ntohs(caddr.sin_port) );


			// ==============================
			// �������������å�
			// ==============================
			access_check_ok = FALSE;

			// -------------------------------------------------------------------------
			// Access Allow�����å�
			//  �ꥹ�Ȥ����������饤����ȥ��ɥ쥹�����ꥹ�Ȥ˰��פ�����ϣˤȤ��롣
			// -------------------------------------------------------------------------
			if ( access_allow_list[0].flag == FALSE ) // ���������ꥹ�Ȥ�����
			{
				// �����å��ϣˤȤ��롣
				debug_log_output("No Access Allow List. No Check.\n");
				access_check_ok = TRUE;
			}
			else
			{
				debug_log_output("Access Check.\n");


				// ���饤����ȥ��ɥ쥹
				strncpy(client_addr_str, inet_ntoa(caddr.sin_addr), sizeof(client_addr_str));

				// client_addr_str��char[4]���Ѵ�
				strncat(client_addr_str, ".", sizeof(client_addr_str) - strlen(client_addr_str));
				for (i=0; i<4; i++ )
				{
					sentence_split(client_addr_str, '.', work1, work2);
					client_address[i] = (unsigned char)atoi(work1);
					strncpy(client_addr_str, work2, sizeof(client_addr_str));
				}

				// �ꥹ�Ȥ�¸�ߤ���������롼��
				for ( i=0; i<ACCESS_ALLOW_LIST_MAX; i++ )
				{
					if ( access_allow_list[i].flag == FALSE ) // �ꥹ�Ƚ�λ
						break;

					// masked_client_address ����
					masked_client_address[0] = client_address[0] & access_allow_list[i].netmask[0];
					masked_client_address[1] = client_address[1] & access_allow_list[i].netmask[1];
					masked_client_address[2] = client_address[2] & access_allow_list[i].netmask[2];
					masked_client_address[3] = client_address[3] & access_allow_list[i].netmask[3];

					// ��Ӽ¹�
					if ( 	(masked_client_address[0] == access_allow_list[i].address[0]) &&
							(masked_client_address[1] == access_allow_list[i].address[1]) &&
							(masked_client_address[2] == access_allow_list[i].address[2]) &&
							(masked_client_address[3] == access_allow_list[i].address[3]) 	)
					{
						debug_log_output("[%d.%d.%d.%d] == [%d.%d.%d.%d] accord!!",
										masked_client_address[0], masked_client_address[1], masked_client_address[2], masked_client_address[3],
										access_allow_list[i].address[0],access_allow_list[i].address[1],access_allow_list[i].address[2], access_allow_list[i].address[3] );

						access_check_ok = TRUE;
						break;
					}
					else
					{
						debug_log_output("[%d.%d.%d.%d] == [%d.%d.%d.%d] discord!!",
										masked_client_address[0], masked_client_address[1], masked_client_address[2], masked_client_address[3],
										access_allow_list[i].address[0],access_allow_list[i].address[1],access_allow_list[i].address[2], access_allow_list[i].address[3] );
					}
				}
			}


			if ( access_check_ok == FALSE )
			{
				debug_log_output("Access Denied.\n");

				close(accept_socket);	// Socket������
				exit( 0 ); // �ҥץ�����λ��

			}


			// HTTP���Ȥ��ơ��Ż��¹�
			server_http_process(accept_socket);

			debug_log_output("HTTP process end.\n");
			debug_log_output("=============================================================\n");

			close(accept_socket);	// Socket������
			exit( 0 ); // �ҥץ������ｪλ��
		}

		// �ƥץ�����accept_socket���Ĥ���
		close(accept_socket);
	}


	return;
}

