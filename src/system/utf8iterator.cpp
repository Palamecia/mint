#include "system/utf8iterator.h"

using namespace std;

bool utf8char_valid(unsigned char c) {
	return !((c & 0x80) && !(c & 0x40));
}

size_t utf8char_length(unsigned char c) {

	if ((c & 0x80) && (c & 0x40)) {
		if (c & 0x20) {
			if (c & 0x10) {
				return 4;
			}

			return 3;
		}

		return 2;
	}

	return 1;
}

size_t utf8length(const string &str) {

	size_t length = 0;

	for (const_utf8iterator it = str.begin(); it != str.end(); ++it) {
		++length;
	}

	return length;
}
