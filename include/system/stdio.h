#ifndef STDIO_H
#define STDIO_H

#include <config.h>
#include <cstdio>

#ifdef OS_WINDOWS
#include <Windows.h>

using ssize_t = SSIZE_T;

MINT_EXPORT ssize_t getline(char **lineptr, size_t *n, FILE *stream);
MINT_EXPORT ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);
#endif

#endif // STDIO_H
