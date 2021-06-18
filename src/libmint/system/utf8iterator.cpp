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

string::size_type mint::utf8_byte_index_to_pos(const string &str, string::difference_type index) {
	return utf8_byte_index_to_pos(str, static_cast<string::size_type>(index));
}

string::size_type mint::utf8_byte_index_to_pos(const string &str, string::size_type index) {

	string::size_type pos = 0;

	if (index == 0) {
		return pos;
	}

	for (const_utf8iterator i = str.begin(); i != str.end(); ++i) {
		size_t len = utf8char_length(static_cast<byte>((*i).front()));
		if (index < len) {
			return string::npos;
		}
		pos++;
		if ((index -= len) == 0) {
			return pos;
		}
	}

	return string::npos;
}

size_t mint::utf8_pos_to_byte_index(const string &str, string::size_type pos) {

	size_t index = 0;

	if (pos == 0) {
		return index;
	}

	for (const_utf8iterator i = str.begin(); i != str.end(); ++i) {
		index += utf8char_length(static_cast<byte>((*i).front()));
		if (--pos == 0) {
			return index;
		}
	}

	return string::npos;
}
