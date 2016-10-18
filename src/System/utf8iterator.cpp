#include "System/utf8iterator.h"

using namespace std;

size_t utf8length(const string &str) {

	size_t length = 0;

	for (const_utf8iterator it = str.begin(); it != str.end(); ++it) {
		++length;
	}

	return length;
}
