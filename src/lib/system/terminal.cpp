#include "memory/functiontool.h"
#include "system/utf8iterator.h"
#include "system/terminal.h"

#include <unistd.h>
#include <termios.h>

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_terminal_readchar, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	int fd = static_cast<int>(helper.popParameter()->data<Number>()->value);

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

MINT_FUNCTION(mint_terminal_readline, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	/// \todo find an efficient way to read a line from a file descriptor
}

MINT_FUNCTION(mint_terminal_read, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	/// \todo find an efficient way to read a line from a file descriptor
}
