#ifndef WIN32_TERMINAL_H
#define WIN32_TERMINAL_H

#include <Windows.h>

namespace mint {

int WriteMultiByteToFile(HANDLE hFileOutput, const char* str, int cbMultiByte = -1);
int WriteCharsToFile(HANDLE hFileOutput, char ch, int cbRepeat = 1);

};

#endif // WIN32_TERMINAL_H
