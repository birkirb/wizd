#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <sys/cygwin.h>

char *my_basename(char *str)
{
	char *p;
	do {
		p = strrchr(str, '/');
		if (p == NULL) {
			return NULL;
		}
		if (p[1] == '\0') {
			*p = '\0';
		}
	} while (*p == '\0');
	return ++p;
}

int main(int argc, char *argv[])
{
	int i;
	char buf[2048];
	char buf2[2048];
	char *target = NULL;
	char *p;

	strncpy(buf2, argv[0], sizeof(buf2));
	p = strrchr(buf2, '/');
	if (p == NULL) {
		fprintf(stderr, "fatal\n");
		return -1;
	}
	*++p = '\0';

	if (argc < 2) {
		fprintf(stderr, "error. need at least 1 argument.\n");
		return -1;
	}
	cygwin_conv_to_full_posix_path(argv[1], buf);
	if (argc == 2) {
		target = my_basename(buf);
	} else target = argv[2];

	strncat(buf2, target, sizeof(buf2) - strlen(buf2) - 1);
	printf("symlink('%s', '%s');\n", buf, buf2);
	printf("return %d\n", symlink(buf, buf2));
	printf("err: %s\n", strerror(errno));
//	sleep(10);

	return 0;
}
