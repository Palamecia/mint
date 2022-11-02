#include "debugger.h"

#ifdef OS_WINDOWS

#include <Windows.h>

int wmain(int argc, wchar_t **argv) {
	char **utf8_argv = static_cast<char **>(alloca(argc * sizeof(char *)));
	for (int i = 0; i < argc; ++i) {
		int length = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
		utf8_argv[i] = static_cast<char *>(alloca(length * sizeof(char)));
		WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, utf8_argv[i], length, nullptr, nullptr);
	}
	Debugger debuger(argc, utf8_argv);
	return debuger.run();
}
#else
int main(int argc, char **argv) {
	Debugger debuger(argc, argv);
	return debuger.run();
}
#endif
