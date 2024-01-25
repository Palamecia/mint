#ifndef WIN32_TERMINAL_H
#define WIN32_TERMINAL_H

#include "mint/config.h"

#include <Windows.h>

namespace mint {

int WriteMultiByteToFile(HANDLE hFileOutput, const char *str, int cbMultiByte = -1);
int WriteCharsToFile(HANDLE hFileOutput, char ch, int cbRepeat = 1);

int pipe_handle_format_flags(HANDLE hPipe, const char **format, va_list *argptr);

};

#endif // WIN32_TERMINAL_H
