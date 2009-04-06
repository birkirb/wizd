// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_main.c
//											$Revision: 1.8 $
//											$Date: 2004/12/18 15:59:58 $
//
//	���٤Ƽ�����Ǥ�Ǥ��ʤ������ޤġ�
//  ���Υ��եȤˤĤ���VertexLink���䤤��碌�ʤ��Ǥ���������
// ==========================================================================
#include 	<stdio.h>
#include 	<string.h>
#include 	<stdlib.h>
#include	<unistd.h>
#include 	<signal.h>
#include	<pwd.h>
#include	<grp.h>

#include	<sys/types.h>
#include 	<sys/wait.h>

#include "wizd.h"


static void print_help(void);
static void daemon_init(void);
static void set_user_id(unsigned char *user, unsigned char *group);


static void setup_SIGCHLD(void);
static void catch_SIGCHLD(int signo);

extern int child_count;

// **************************************************************************
// * Main Program
// **************************************************************************
int main(int argc, char *argv[])
{
	int i;
	pid_t	pid;
	char flag_daemon = FALSE;

	fprintf(stderr, "%s  start.\n", SERVER_NAME);

	// =============================================
	// �Ƽ�����
	// =============================================
	global_param_init();


	// =============================================
	// ���ץ��������å�
	// =============================================
	for (i=1; i<argc; i++)
	{

		// ----------------------------------------------------
		// -h, --help, -v, --version:  �إ�ץ�å�����
		// ----------------------------------------------------
		if ( (strcmp(argv[i], "-h") 		== 0) ||
			 (strcmp(argv[i], "--help")		== 0) ||
			 (strcmp(argv[i], "-v") 		== 0) ||
			 (strcmp(argv[i], "--version")	== 0) 		)
		{
			print_help();
			exit( 0 );
		}
		if (strcmp(argv[i], "-d") 		== 0)
		{
			flag_daemon = TRUE;
		}
	}



	// =============================================
	// ����ե����ե������ɤ�(wizd.conf)
	// =============================================
	config_file_read();

	if (global_param.wizd_chdir[0]) {
		chdir(global_param.wizd_chdir);
	}

	// �ǡ����⡼�ɤˤĤ��Ƥϡ����ޥ�ɥ饤��ѥ�᡼����ͥ��
	if (flag_daemon)
	{
		global_param.flag_daemon = TRUE;
	}


	// ======================
	// = SetUID �¹�
	// ======================
	set_user_id(global_param.exec_user, global_param.exec_group);

	if (global_param.wizd_chdir[0]) {
		// after setuid, do chdir again.
		chdir(global_param.wizd_chdir);
	}

	// =======================
	// Debug Log ���ϳ���
	// =======================
	if ( global_param.flag_debug_log_output == TRUE )
	{
		fprintf(stderr, "debug log output start..\n");
		debug_log_initialize(global_param.debug_log_filename);
		debug_log_output("\n%s boot up.", SERVER_NAME );
		debug_log_output("debug log output start..\n");
	}

	config_sanity_check();

	// =================
	// daemon�����롣
	// =================
	if ( global_param.flag_daemon == TRUE )
	{
		printf("Daemoning....\n");
		daemon_init();
	}



	// ==========================================
	// �ҥץ��������ʥ����������fork��ɬ��
	// ==========================================
	setup_SIGCHLD();


	// ==================================
	// Server��ư���Ф�Ȥ��Ф�����
	// Server Detect����Fork���Ƽ¹�
	// ==================================
	if ( global_param.flag_auto_detect == TRUE )
	{
		pid = fork();
		if ( pid < 0 ) // fork���ԥ����å�
		{
			perror("fork");
			exit( 1 );
		}

		if (pid == 0)
		{
			// �ʲ��ҥץ�����
			server_detect();
			exit ( 0 );
		}
	}


	// =======================
	// HTTP Server�Ż�����
	// =======================
	server_listen();

	printf("%s  end.\n", SERVER_NAME);

	exit( 0 );
}





// **************************************************************************
// Help��å���������
// **************************************************************************
static void print_help(void)
{
	printf("%s -- %s\n\n", SERVER_NAME, SERVER_DETAIL);
	printf("Usase: wizd [options]\n");


	printf("Options:\n");
	printf(" -h, --help\tprint this message.\n");

	printf("\n");
}




// **************************************************************************
// �ǡ���󲽤��롣
// **************************************************************************
static void daemon_init(void)
{
	int	pid;

	// ɸ�������ϡ�ɸ�२�顼���Ϥ��Ĥ���
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);


	pid = fork() ;
	if ( pid != 0 )
	{
		exit ( 0 );
	}


	setsid();
	signal(SIGHUP, SIG_IGN);

	pid = fork();
	if ( pid != 0 )
	{
		exit ( 0 );
	}


	return;
}



// **************************************************************************
// ���ꤵ�줿UID/GID���ѹ����롣root��ư���줿�Ȥ��Τ�ͭ��
// **************************************************************************
static void set_user_id(unsigned char *user, unsigned char *group)
{
	struct passwd 	*user_passwd;
	struct group 	*user_group;


	// root�������å�
	if ( getuid() != 0 )
		return;

	// user�����ꤵ��Ƥ��뤫�����å�
	if ( strlen(user) <= 0 )
		return;


	// userID��Get.
	user_passwd = getpwnam( user );
	if ( user_passwd == NULL )
	{
		return;
	}

	// setuid�¹�
	setuid ( user_passwd->pw_uid );

	// group�ϥ��ץ���󡣻��꤬��������ꤹ�롣
	if ( strlen(group) > 0 )
	{
		user_group = getgrnam( group );
		if ( user_group == NULL )
			return;

		setgid( user_group->gr_gid );
	}

	return;
}



// **************************************************************************
// �ҥץ�������λ�����Ȥ��˥����ʥ������������
// **************************************************************************
static void setup_SIGCHLD(void)
{
	struct sigaction act;
	memset(&act, 0, sizeof(act));

	// SIGCHLDȯ������catch_SIGCHLD()��¹�
	act.sa_handler = catch_SIGCHLD;

	// catch_SIGCHLD()����ɲå����ʥ�ޥ����ʤ�
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction(SIGCHLD, &act, NULL);

	return;
}


// **************************************************************************
// �����ʥ��ư����SIGCHLD��������ȸƤФ�롣
// **************************************************************************
static void catch_SIGCHLD(int signo)
{
	pid_t child_pid = 0;
	int child_ret;

	debug_log_output("catch SIGCHLD!!(signo=%d)\n", signo);


	// ���٤Ƥν�λ���Ƥ���ҥץ������Ф���waitpid()��Ƥ�
	while ( 1 )
	{
		child_pid = waitpid(-1, &child_ret, WNOHANG);
		if (child_pid <= 0 )	// ����waitpid�������ȡ�-1�����
		{
			break;
		}
		if (child_count > 0) child_count--;
		debug_log_output("catch_SIGCHILD waitpid()=%d, child_count = %d\n"
			, child_pid, child_count);
	}
	debug_log_output("catch SIGCHLD end.\n");


	return;
}



