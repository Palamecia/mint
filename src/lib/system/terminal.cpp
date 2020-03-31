#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "system/utf8iterator.h"
#include "system/stdio.h"
#include "system/terminal.h"
#include "system/fileprinter.h"
#include "ast/cursor.h"

#ifdef OS_UNIX
#include <unistd.h>
#include <termios.h>
#else
#include <Windows.h>
#include <io.h>
#endif

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_terminal_readchar, 0, cursor) {

	FunctionHelper helper(cursor, 0);

	int fd = fileno(stdin);

	char buffer[5];

	if (read(fd, buffer, 1) > 0) {

		size_t length = utf8char_length(static_cast<byte>(*buffer));

		if (length > 1) {
			if (read(fd, buffer + 1, length - 1) > 0) {
				helper.returnValue(create_string(string(buffer, length)));
			}
		}
		else {
			helper.returnValue(create_string(string(buffer, 1)));
		}
	}
}

MINT_FUNCTION(mint_terminal_readline, 0, cursor) {

	FunctionHelper helper(cursor, 0);

	size_t size = 0;
	char *buffer = nullptr;

	if (getline(&buffer, &size, stdin) != -1) {
		helper.returnValue(create_string(buffer));
		free(buffer);
	}
}

MINT_FUNCTION(mint_terminal_read, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	size_t size = 0;
	char *buffer = nullptr;
	string delim = to_string(helper.popParameter());

	if (getdelim(&buffer, &size, delim.front(), stdin) != -1) {
		helper.returnValue(create_string(buffer));
		free(buffer);
	}
}

MINT_FUNCTION(mint_terminal_write, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	term_cprint(stdout, to_string(helper.popParameter()).c_str());
}

MINT_FUNCTION(mint_terminal_change_attribute, 1, cursor) {

	string attr = to_string(cursor->stack().back());
	FILE *stream = stdout;

	cursor->stack().back() = SharedReference::unique(StrongReference::create<None>());
	cursor->exitCall();

	if (FilePrinter *printer = dynamic_cast<FilePrinter *>(cursor->printer())) {
		if (isatty(fileno(printer->file()))) {
			stream = printer->file();
		}
	}

	term_cprint(stream, attr.c_str());
}
