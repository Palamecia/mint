#include "system/utf8iterator.h"

using namespace std;
using namespace mint;

bool mint::utf8char_valid(byte b) {
	return !((b & 0x80) && !(b & 0x40));
}

size_t mint::utf8char_length(byte b) {

	if ((b & 0x80) && (b & 0x40)) {
		if (b & 0x20) {
			if (b & 0x10) {
				return 4;
			}

			return 3;
		}

		return 2;
	}

	return 1;
}

size_t mint::utf8length(const string &str) {

	size_t length = 0;

	for (const_utf8iterator it = str.begin(); it != str.end(); ++it) {
		++length;
	}

	return length;
}
