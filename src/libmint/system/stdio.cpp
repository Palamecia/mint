#include "system/stdio.h"

#ifdef OS_WINDOWS
ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
	return getdelim(lineptr, n, '\n', stream);
}

ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream) {

	if (lineptr == nullptr) {
		return -1;
	}

	if (stream == nullptr) {
		return -1;
	}

	if (n == nullptr) {
		return -1;
	}

	char *bufptr = *lineptr;
	size_t size = *n;

	int c = fgetc(stream);

	if (c == EOF) {
		return -1;
	}
	if (bufptr == nullptr) {

		bufptr = static_cast<char *>(malloc(128));

		if (bufptr == nullptr) {
			return -1;
		}

		size = 128;
	}

	char *cptr = bufptr;

	while (c != EOF) {
		if ((cptr - bufptr) > (size - 1)) {
			size = size + 128;
			bufptr = static_cast<char *>(realloc(bufptr, size));
			if (bufptr == nullptr) {
				return -1;
			}
		}
		*cptr++ = c;
		if (c == delim) {
			break;
		}
		c = fgetc(stream);
	}

	*cptr++ = '\0';
	*lineptr = bufptr;
	*n = size;

	return cptr - bufptr - 1;
}
#endif
