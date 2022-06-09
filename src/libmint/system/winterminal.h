#ifndef WINTERMINAL_H
#define WINTERMINAL_H

#include <Windows.h>

namespace mint {

int WriteMultiByteToConsoleW(HANDLE hConsoleOutput, const char *str, int cbMultiByte = -1);
int WriteCharsToConsoleW(HANDLE hConsoleOutput, wchar_t wc, int cbRepeat = 1);

bool vt100_enabled_for_console(HANDLE hTerminal);
const char *handle_vt100_sequence(HANDLE hTerminal, const char *cptr);
int handle_format_flags(HANDLE hTerminal, const char **format, va_list *argptr);

};

#endif // WINTERMINAL_H
