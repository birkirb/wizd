#include <stdio.h>
#include <stdlib.h>

#define MAX_LINE 20
#define LINE_BUF 1000

char buf[MAX_LINE][LINE_BUF];
int num[MAX_LINE];
int max;

int search(int ch) {
	int i;

	for (i=0; i<max; i++) {
		if (strchr(buf[i], ch) != NULL) {
			return num[i];
		}
	}
	return 0;
}

main()
{
	char tmp[LINE_BUF];
	int i;

	for (i=0; i<MAX_LINE && fgets(tmp, sizeof(tmp), stdin) != NULL; i++) {
		sscanf(tmp, "%2d: %s", &num[i], buf[i]);
	}

	max = i;
	for (i=0; i<128; i++) {
		printf("%c", search(i) + 'a');
	}

}
