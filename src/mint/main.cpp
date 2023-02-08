#include "mint/scheduler/scheduler.h"

using namespace mint;

#ifdef OS_WINDOWS

#include <Windows.h>

int wmain(int argc, wchar_t **argv) {
	char **utf8_argv = static_cast<char **>(malloc(argc * sizeof(char *)));
	for (int i = 0; i < argc; ++i) {
		int length = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
		utf8_argv[i] = static_cast<char *>(malloc(length * sizeof(char)));
		WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, utf8_argv[i], length, nullptr, nullptr);
	}
	Scheduler scheduler(argc, utf8_argv);
	int status = scheduler.run();
	for (int i = 0; i < argc; ++i) {
		free(utf8_argv[i]);
	}
	free(utf8_argv);
	return status;
}
#else
int main(int argc, char **argv) {
	Scheduler scheduler(argc, argv);
	return scheduler.run();
}
#endif
