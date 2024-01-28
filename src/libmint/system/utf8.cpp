/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "mint/system/utf8.h"
#include "mint/system/assert.h"

#ifdef MINT_UTF8_WITH_ICU
#ifdef OS_WINDOWS
#include <icu.h>
#else
#include <unicode/uchar.h>
#endif
#else
#include <cctype>
#endif

#include <algorithm>

using namespace std;
using namespace mint;

static const constexpr byte_t first_byte_mark[] = {
	0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};

static const constexpr byte_t trailing_bytes_for_utf8[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static const constexpr uint32_t offsets_from_utf8[] = {
	0x00000000UL, 0x00003080UL, 0x000E2080UL,
	0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static constexpr const byte_t utf8_replacement_char[] = { 0xEF, 0xBF, 0xBD, 0x00 };

static constexpr const uint32_t utf32_max_legal = 0x0010FFFF;
static constexpr const uint32_t utf32_sur_high_start = 0xD800;
static constexpr const uint32_t utf32_sur_low_end = 0xDFFF;
static constexpr const uint32_t utf32_replacement_char = 0x0000FFFD;

bool mint::utf8_begin_code_point(byte_t b) {
	return !((b & 0x80) && !(b & 0x40));
}

string_view::size_type mint::utf8_code_point_length(byte_t b) {
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

string_view::size_type mint::utf8_code_point_count(const string_view &str) {
	size_t code_point_count = 0;
	for (const_utf8view_iterator it = str.begin(); it != str.end(); ++it) {
		++code_point_count;
	}
	return code_point_count;
}

string_view::size_type mint::utf8_byte_index_to_code_point_index(const string_view &str, string_view::difference_type byte_index) {
	return utf8_byte_index_to_code_point_index(str, static_cast<string_view::size_type>(byte_index));
}

string_view::size_type mint::utf8_byte_index_to_code_point_index(const string_view &str, string_view::size_type byte_index) {

	string_view::size_type code_point_index = 0;

	if (byte_index == 0) {
		return code_point_index;
	}

	for (const_utf8view_iterator i = str.begin(); i != str.end(); ++i) {
		size_t len = utf8_code_point_length(static_cast<byte_t>((*i).front()));
		if (byte_index < len) {
			return string_view::npos;
		}
		code_point_index++;
		if ((byte_index -= len) == 0) {
			return code_point_index;
		}
	}

	return string_view::npos;
}

string_view::size_type mint::utf8_previous_code_point_byte_index(const string_view &str, string_view::size_type byte_index) {
	if (byte_index) {
		do {
			byte_index--;
		}
		while (!utf8_begin_code_point(static_cast<byte_t>(str[byte_index])));
		return byte_index;
	}
	return string_view::npos;
}

string_view::size_type mint::utf8_next_code_point_byte_index(const string_view &str, string_view::size_type byte_index) {
	return byte_index + utf8_code_point_length(static_cast<byte_t>(str[byte_index]));
}

string_view::size_type mint::utf8_code_point_index_to_byte_index(const string_view &str, string_view::size_type code_point_index) {

	size_t byte_index = 0;

	if (code_point_index == 0) {
		return byte_index;
	}

	for (const_utf8view_iterator i = str.begin(); i != str.end(); ++i) {
		byte_index += utf8_code_point_length(static_cast<byte_t>((*i).front()));
		if (--code_point_index == 0) {
			return byte_index;
		}
	}

	return string_view::npos;
}

string_view::size_type mint::utf8_substring_byte_count(const string_view &str, string_view::size_type code_point_index, string_view::size_type code_point_count) {
	size_t byte_count = 0, i = 0;
	for (const_utf8view_iterator it = str.begin(); i < code_point_index + code_point_count && it != str.end(); ++it) {
		if (i++ >= code_point_index) {
			byte_count += it->length();
		}
	}
	return byte_count;
}

struct interval {
	int32_t first;
	int32_t last;
};

/* auxiliary function for binary search in interval table */
static bool bisearch(int32_t ucs, const struct interval *table, int max) {
	int min = 0;
	if (ucs < table[0].first || ucs > table[max].last) {
		return false;
	}
	while (max >= min) {
		int mid = (min + max) / 2;
		if (ucs > table[mid].last) {
			min = mid + 1;
		}
		else if (ucs < table[mid].first) {
			max = mid - 1;
		}
		else {
			return true;
		}
	}
	return false;
}

static bool mk_is_wide_char(uint32_t ucs) {
	static const interval wide[] = {
		{0x1100, 0x115f}, {0x231a, 0x231b}, {0x2329, 0x232a},
		{0x23e9, 0x23ec}, {0x23f0, 0x23f0}, {0x23f3, 0x23f3},
		{0x25fd, 0x25fe}, {0x2614, 0x2615}, {0x2648, 0x2653},
		{0x267f, 0x267f}, {0x2693, 0x2693}, {0x26a1, 0x26a1},
		{0x26aa, 0x26ab}, {0x26bd, 0x26be}, {0x26c4, 0x26c5},
		{0x26ce, 0x26ce}, {0x26d4, 0x26d4}, {0x26ea, 0x26ea},
		{0x26f2, 0x26f3}, {0x26f5, 0x26f5}, {0x26fa, 0x26fa},
		{0x26fd, 0x26fd}, {0x2705, 0x2705}, {0x270a, 0x270b},
		{0x2728, 0x2728}, {0x274c, 0x274c}, {0x274e, 0x274e},
		{0x2753, 0x2755}, {0x2757, 0x2757}, {0x2795, 0x2797},
		{0x27b0, 0x27b0}, {0x27bf, 0x27bf}, {0x2b1b, 0x2b1c},
		{0x2b50, 0x2b50}, {0x2b55, 0x2b55}, {0x2e80, 0x2fdf},
		{0x2ff0, 0x303e}, {0x3040, 0x3247}, {0x3250, 0x4dbf},
		{0x4e00, 0xa4cf}, {0xa960, 0xa97f}, {0xac00, 0xd7a3},
		{0xf900, 0xfaff}, {0xfe10, 0xfe19}, {0xfe30, 0xfe6f},
		{0xff01, 0xff60}, {0xffe0, 0xffe6}, {0x16fe0, 0x16fe1},
		{0x17000, 0x18aff}, {0x1b000, 0x1b12f}, {0x1b170, 0x1b2ff},
		{0x1f004, 0x1f004}, {0x1f0cf, 0x1f0cf}, {0x1f18e, 0x1f18e},
		{0x1f191, 0x1f19a}, {0x1f200, 0x1f202}, {0x1f210, 0x1f23b},
		{0x1f240, 0x1f248}, {0x1f250, 0x1f251}, {0x1f260, 0x1f265},
		{0x1f300, 0x1f320}, {0x1f32d, 0x1f335}, {0x1f337, 0x1f37c},
		{0x1f37e, 0x1f393}, {0x1f3a0, 0x1f3ca}, {0x1f3cf, 0x1f3d3},
		{0x1f3e0, 0x1f3f0}, {0x1f3f4, 0x1f3f4}, {0x1f3f8, 0x1f43e},
		{0x1f440, 0x1f440}, {0x1f442, 0x1f4fc}, {0x1f4ff, 0x1f53d},
		{0x1f54b, 0x1f54e}, {0x1f550, 0x1f567}, {0x1f57a, 0x1f57a},
		{0x1f595, 0x1f596}, {0x1f5a4, 0x1f5a4}, {0x1f5fb, 0x1f64f},
		{0x1f680, 0x1f6c5}, {0x1f6cc, 0x1f6cc}, {0x1f6d0, 0x1f6d2},
		{0x1f6eb, 0x1f6ec}, {0x1f6f4, 0x1f6f8}, {0x1f910, 0x1f93e},
		{0x1f940, 0x1f94c}, {0x1f950, 0x1f96b}, {0x1f980, 0x1f997},
		{0x1f9c0, 0x1f9c0}, {0x1f9d0, 0x1f9e6}, {0x20000, 0x2fffd},
		{0x30000, 0x3fffd},
	};

	return bisearch(ucs, wide, extent<decltype(wide)>::value - 1);
}

static int mk_wcwidth(uint32_t ucs) {
	/* sorted list of non-overlapping intervals of non-spacing characters */
	/* generated by "uniset +cat=Me +cat=Mn +cat=Cf -00AD +1160-11FF +200B c" */
	static const interval combining[] = {
		{0x00ad, 0x00ad}, {0x0300, 0x036f}, {0x0483, 0x0489},
		{0x0591, 0x05bd}, {0x05bf, 0x05bf}, {0x05c1, 0x05c2},
		{0x05c4, 0x05c5}, {0x05c7, 0x05c7}, {0x0610, 0x061a},
		{0x061c, 0x061c}, {0x064b, 0x065f}, {0x0670, 0x0670},
		{0x06d6, 0x06dc}, {0x06df, 0x06e4}, {0x06e7, 0x06e8},
		{0x06ea, 0x06ed}, {0x0711, 0x0711}, {0x0730, 0x074a},
		{0x07a6, 0x07b0}, {0x07eb, 0x07f3}, {0x0816, 0x0819},
		{0x081b, 0x0823}, {0x0825, 0x0827}, {0x0829, 0x082d},
		{0x0859, 0x085b}, {0x08d4, 0x08e1}, {0x08e3, 0x0902},
		{0x093a, 0x093a}, {0x093c, 0x093c}, {0x0941, 0x0948},
		{0x094d, 0x094d}, {0x0951, 0x0957}, {0x0962, 0x0963},
		{0x0981, 0x0981}, {0x09bc, 0x09bc}, {0x09c1, 0x09c4},
		{0x09cd, 0x09cd}, {0x09e2, 0x09e3}, {0x0a01, 0x0a02},
		{0x0a3c, 0x0a3c}, {0x0a41, 0x0a42}, {0x0a47, 0x0a48},
		{0x0a4b, 0x0a4d}, {0x0a51, 0x0a51}, {0x0a70, 0x0a71},
		{0x0a75, 0x0a75}, {0x0a81, 0x0a82}, {0x0abc, 0x0abc},
		{0x0ac1, 0x0ac5}, {0x0ac7, 0x0ac8}, {0x0acd, 0x0acd},
		{0x0ae2, 0x0ae3}, {0x0afa, 0x0aff}, {0x0b01, 0x0b01},
		{0x0b3c, 0x0b3c}, {0x0b3f, 0x0b3f}, {0x0b41, 0x0b44},
		{0x0b4d, 0x0b4d}, {0x0b56, 0x0b56}, {0x0b62, 0x0b63},
		{0x0b82, 0x0b82}, {0x0bc0, 0x0bc0}, {0x0bcd, 0x0bcd},
		{0x0c00, 0x0c00}, {0x0c3e, 0x0c40}, {0x0c46, 0x0c48},
		{0x0c4a, 0x0c4d}, {0x0c55, 0x0c56}, {0x0c62, 0x0c63},
		{0x0c81, 0x0c81}, {0x0cbc, 0x0cbc}, {0x0cbf, 0x0cbf},
		{0x0cc6, 0x0cc6}, {0x0ccc, 0x0ccd}, {0x0ce2, 0x0ce3},
		{0x0d00, 0x0d01}, {0x0d3b, 0x0d3c}, {0x0d41, 0x0d44},
		{0x0d4d, 0x0d4d}, {0x0d62, 0x0d63}, {0x0dca, 0x0dca},
		{0x0dd2, 0x0dd4}, {0x0dd6, 0x0dd6}, {0x0e31, 0x0e31},
		{0x0e34, 0x0e3a}, {0x0e47, 0x0e4e}, {0x0eb1, 0x0eb1},
		{0x0eb4, 0x0eb9}, {0x0ebb, 0x0ebc}, {0x0ec8, 0x0ecd},
		{0x0f18, 0x0f19}, {0x0f35, 0x0f35}, {0x0f37, 0x0f37},
		{0x0f39, 0x0f39}, {0x0f71, 0x0f7e}, {0x0f80, 0x0f84},
		{0x0f86, 0x0f87}, {0x0f8d, 0x0f97}, {0x0f99, 0x0fbc},
		{0x0fc6, 0x0fc6}, {0x102d, 0x1030}, {0x1032, 0x1037},
		{0x1039, 0x103a}, {0x103d, 0x103e}, {0x1058, 0x1059},
		{0x105e, 0x1060}, {0x1071, 0x1074}, {0x1082, 0x1082},
		{0x1085, 0x1086}, {0x108d, 0x108d}, {0x109d, 0x109d},
		{0x1160, 0x11ff}, {0x135d, 0x135f}, {0x1712, 0x1714},
		{0x1732, 0x1734}, {0x1752, 0x1753}, {0x1772, 0x1773},
		{0x17b4, 0x17b5}, {0x17b7, 0x17bd}, {0x17c6, 0x17c6},
		{0x17c9, 0x17d3}, {0x17dd, 0x17dd}, {0x180b, 0x180e},
		{0x1885, 0x1886}, {0x18a9, 0x18a9}, {0x1920, 0x1922},
		{0x1927, 0x1928}, {0x1932, 0x1932}, {0x1939, 0x193b},
		{0x1a17, 0x1a18}, {0x1a1b, 0x1a1b}, {0x1a56, 0x1a56},
		{0x1a58, 0x1a5e}, {0x1a60, 0x1a60}, {0x1a62, 0x1a62},
		{0x1a65, 0x1a6c}, {0x1a73, 0x1a7c}, {0x1a7f, 0x1a7f},
		{0x1ab0, 0x1abe}, {0x1b00, 0x1b03}, {0x1b34, 0x1b34},
		{0x1b36, 0x1b3a}, {0x1b3c, 0x1b3c}, {0x1b42, 0x1b42},
		{0x1b6b, 0x1b73}, {0x1b80, 0x1b81}, {0x1ba2, 0x1ba5},
		{0x1ba8, 0x1ba9}, {0x1bab, 0x1bad}, {0x1be6, 0x1be6},
		{0x1be8, 0x1be9}, {0x1bed, 0x1bed}, {0x1bef, 0x1bf1},
		{0x1c2c, 0x1c33}, {0x1c36, 0x1c37}, {0x1cd0, 0x1cd2},
		{0x1cd4, 0x1ce0}, {0x1ce2, 0x1ce8}, {0x1ced, 0x1ced},
		{0x1cf4, 0x1cf4}, {0x1cf8, 0x1cf9}, {0x1dc0, 0x1df9},
		{0x1dfb, 0x1dff}, {0x200b, 0x200f}, {0x202a, 0x202e},
		{0x2060, 0x2064}, {0x2066, 0x206f}, {0x20d0, 0x20f0},
		{0x2cef, 0x2cf1}, {0x2d7f, 0x2d7f}, {0x2de0, 0x2dff},
		{0x302a, 0x302d}, {0x3099, 0x309a}, {0xa66f, 0xa672},
		{0xa674, 0xa67d}, {0xa69e, 0xa69f}, {0xa6f0, 0xa6f1},
		{0xa802, 0xa802}, {0xa806, 0xa806}, {0xa80b, 0xa80b},
		{0xa825, 0xa826}, {0xa8c4, 0xa8c5}, {0xa8e0, 0xa8f1},
		{0xa926, 0xa92d}, {0xa947, 0xa951}, {0xa980, 0xa982},
		{0xa9b3, 0xa9b3}, {0xa9b6, 0xa9b9}, {0xa9bc, 0xa9bc},
		{0xa9e5, 0xa9e5}, {0xaa29, 0xaa2e}, {0xaa31, 0xaa32},
		{0xaa35, 0xaa36}, {0xaa43, 0xaa43}, {0xaa4c, 0xaa4c},
		{0xaa7c, 0xaa7c}, {0xaab0, 0xaab0}, {0xaab2, 0xaab4},
		{0xaab7, 0xaab8}, {0xaabe, 0xaabf}, {0xaac1, 0xaac1},
		{0xaaec, 0xaaed}, {0xaaf6, 0xaaf6}, {0xabe5, 0xabe5},
		{0xabe8, 0xabe8}, {0xabed, 0xabed}, {0xfb1e, 0xfb1e},
		{0xfe00, 0xfe0f}, {0xfe20, 0xfe2f}, {0xfeff, 0xfeff},
		{0xfff9, 0xfffb}, {0x101fd, 0x101fd}, {0x102e0, 0x102e0},
		{0x10376, 0x1037a}, {0x10a01, 0x10a03}, {0x10a05, 0x10a06},
		{0x10a0c, 0x10a0f}, {0x10a38, 0x10a3a}, {0x10a3f, 0x10a3f},
		{0x10ae5, 0x10ae6}, {0x11001, 0x11001}, {0x11038, 0x11046},
		{0x1107f, 0x11081}, {0x110b3, 0x110b6}, {0x110b9, 0x110ba},
		{0x11100, 0x11102}, {0x11127, 0x1112b}, {0x1112d, 0x11134},
		{0x11173, 0x11173}, {0x11180, 0x11181}, {0x111b6, 0x111be},
		{0x111ca, 0x111cc}, {0x1122f, 0x11231}, {0x11234, 0x11234},
		{0x11236, 0x11237}, {0x1123e, 0x1123e}, {0x112df, 0x112df},
		{0x112e3, 0x112ea}, {0x11300, 0x11301}, {0x1133c, 0x1133c},
		{0x11340, 0x11340}, {0x11366, 0x1136c}, {0x11370, 0x11374},
		{0x11438, 0x1143f}, {0x11442, 0x11444}, {0x11446, 0x11446},
		{0x114b3, 0x114b8}, {0x114ba, 0x114ba}, {0x114bf, 0x114c0},
		{0x114c2, 0x114c3}, {0x115b2, 0x115b5}, {0x115bc, 0x115bd},
		{0x115bf, 0x115c0}, {0x115dc, 0x115dd}, {0x11633, 0x1163a},
		{0x1163d, 0x1163d}, {0x1163f, 0x11640}, {0x116ab, 0x116ab},
		{0x116ad, 0x116ad}, {0x116b0, 0x116b5}, {0x116b7, 0x116b7},
		{0x1171d, 0x1171f}, {0x11722, 0x11725}, {0x11727, 0x1172b},
		{0x11a01, 0x11a06}, {0x11a09, 0x11a0a}, {0x11a33, 0x11a38},
		{0x11a3b, 0x11a3e}, {0x11a47, 0x11a47}, {0x11a51, 0x11a56},
		{0x11a59, 0x11a5b}, {0x11a8a, 0x11a96}, {0x11a98, 0x11a99},
		{0x11c30, 0x11c36}, {0x11c38, 0x11c3d}, {0x11c3f, 0x11c3f},
		{0x11c92, 0x11ca7}, {0x11caa, 0x11cb0}, {0x11cb2, 0x11cb3},
		{0x11cb5, 0x11cb6}, {0x11d31, 0x11d36}, {0x11d3a, 0x11d3a},
		{0x11d3c, 0x11d3d}, {0x11d3f, 0x11d45}, {0x11d47, 0x11d47},
		{0x16af0, 0x16af4}, {0x16b30, 0x16b36}, {0x16f8f, 0x16f92},
		{0x1bc9d, 0x1bc9e}, {0x1bca0, 0x1bca3}, {0x1d167, 0x1d169},
		{0x1d173, 0x1d182}, {0x1d185, 0x1d18b}, {0x1d1aa, 0x1d1ad},
		{0x1d242, 0x1d244}, {0x1da00, 0x1da36}, {0x1da3b, 0x1da6c},
		{0x1da75, 0x1da75}, {0x1da84, 0x1da84}, {0x1da9b, 0x1da9f},
		{0x1daa1, 0x1daaf}, {0x1e000, 0x1e006}, {0x1e008, 0x1e018},
		{0x1e01b, 0x1e021}, {0x1e023, 0x1e024}, {0x1e026, 0x1e02a},
		{0x1e8d0, 0x1e8d6}, {0x1e944, 0x1e94a}, {0xe0001, 0xe0001},
		{0xe0020, 0xe007f}, {0xe0100, 0xe01ef},
	};

	/* test for 8-bit control characters */
	if (ucs == 0) {
		return 0;
	}
	if ((ucs < 32) || ((ucs >= 0x7f) && (ucs < 0xa0))) {
		return -1;
	}

	/* binary search in table of non-spacing characters */
	if (bisearch(ucs, combining, extent<decltype(combining)>::value - 1)) {
		return 0;
	}

	/* if we arrive here, ucs is not a combining or C0/C1 control character */
	return mk_is_wide_char(ucs) ? 2 : 1;
}

static uint32_t utf8_to_utf32(const string_view& code_point) {
	const byte_t *source = reinterpret_cast<const byte_t *>(code_point.data());
	if (!utf8_begin_code_point(*source)) {
		return utf32_replacement_char;
	}
	const byte_t extra_bytes_to_read = trailing_bytes_for_utf8[*source];
	if (extra_bytes_to_read > code_point.size()) {
		return utf32_replacement_char;
	}
	uint32_t ch = 0;
	switch (extra_bytes_to_read) {
	case 5:
		ch += *source++;
		ch <<= 6;
		[[ fallthrough ]];
	case 4:
		ch += *source++;
		ch <<= 6;
		[[ fallthrough ]];
	case 3:
		ch += *source++;
		ch <<= 6;
		[[ fallthrough ]];
	case 2:
		ch += *source++;
		ch <<= 6;
		[[ fallthrough ]];
	case 1:
		ch += *source++;
		ch <<= 6;
		[[ fallthrough ]];
	case 0:
		ch += *source++;
	}
	ch -= offsets_from_utf8[extra_bytes_to_read];
	if (ch <= utf32_max_legal) {
		/*
		 * UTF-16 surrogate values are illegal in UTF-32, and anything
		 * over Plane 17 (> 0x10FFFF) is illegal.
		 */
		if (ch >= utf32_sur_high_start && ch <= utf32_sur_low_end) {
			return utf32_replacement_char;
		}
		return ch;
	}
	return utf32_replacement_char;
}

static string utf8_from_utf32(uint32_t code_point) {
	string::size_type code_point_length = 0;
	/*
	 * Figure out how many bytes the result will require. Turn any
	 * illegally large UTF32 things (> Plane 17) into replacement chars.
	 */
	if (code_point < 0x80) {
		code_point_length = 1;
	}
	else if (code_point < 0x800) {
		code_point_length = 2;
	}
	else if (code_point < 0x10000) {
		code_point_length = 3;
	}
	else if (code_point <= utf32_max_legal) {
		code_point_length = 4;
	}
	else {
		return reinterpret_cast<const char *>(utf8_replacement_char);
	}

	string ch(code_point_length, ' ');
	byte_t *target = reinterpret_cast<byte_t *>(ch.data());
	target += code_point_length;

	static constexpr const uint32_t byte_mask = 0xBF;
	static constexpr const uint32_t byte_mark = 0x80;

	switch (code_point_length) {
	case 4:
		*--target = ((code_point | byte_mark) & byte_mask);
		code_point >>= 6;
		[[ fallthrough ]];
	case 3:
		*--target = ((code_point | byte_mark) & byte_mask);
		code_point >>= 6;
		[[ fallthrough ]];
	case 2:
		*--target = ((code_point | byte_mark) & byte_mask);
		code_point >>= 6;
		[[ fallthrough ]];
	case 1:
		*--target = (code_point | first_byte_mark[code_point_length]);
	}

	return ch;
}

// column width of a utf8 single character sequence.
string_view::size_type mint::utf8_grapheme_code_point_count(const string_view &str) {
	byte_t b = static_cast<byte_t>(str[0]);
	if (b < ' ') {
		return 0;
	}
	if (b <= 0x7F) {
		return 1;
	}
	if (b <= 0xC1) { // invalid continuation byte or invalid 0xC0, 0xC1 (check is strictly not necessary as we don't validate..)
		return 1;
	}
	if (b <= 0xDF) { // b >= 0xC2  // 2 bytes
		if (str.length() < 2) {
			// failed
			return 1;
		}
		int32_t ucs = (((b & 0x1F) << 6) | (str[1] & 0x3F));
		assert(ucs < 0xD800 || ucs > 0xDFFF);
		return mk_wcwidth(ucs);
	}
	if (b <= 0xEF) { // b >= 0xE0  // 3 bytes
		if (str.length() < 3) {
			// failed
			return 1;
		}
		int32_t ucs = (((b & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F));
		return mk_wcwidth(ucs);
	}
	if (b <= 0xF4) { // b >= 0xF0  // 4 bytes
		if (str.length() < 4) {
			// failed
			return 1;
		}
		int32_t ucs = (((b & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F));
		return mk_wcwidth(ucs);
	}
	// failed
	return 1;
}

int mint::utf8_compare(const string_view &s1, const string_view &s2) {
	return s1.compare(s2);
}

int mint::utf8_compare_substring(const string_view &s1, const string_view &s2, string_view::size_type code_point_count) {
	return s1.compare(0, utf8_substring_byte_count(s1, 0, code_point_count), s2, 0, utf8_substring_byte_count(s2, 0, code_point_count));
}

int mint::utf8_compare_case_insensitive(const string_view &s1, const string_view &s2) {
	const_utf8view_iterator s1_begin = s1.begin(), s1_end = s1.end();
	const_utf8view_iterator s2_begin = s2.begin(), s2_end = s2.end();
	for (auto it1 = s1_begin, it2 = s2_begin; it1 != s1_end && it2 != s2_end; ++it1, ++it2) {
		if (int diff = utf8_to_lower(*it1).compare(utf8_to_lower(*it2))) {
			return diff;
		}
	}
	return 0;
}

int mint::utf8_compare_substring_case_insensitive(const string_view &s1, const string_view &s2, string_view::size_type code_point_count) {
	return utf8_compare_case_insensitive(s1.substr(0, utf8_substring_byte_count(s1, 0, code_point_count)),
										 s2.substr(0, utf8_substring_byte_count(s2, 0, code_point_count)));
}

bool mint::utf8_is_alnum(const string_view &str) {
#ifdef MINT_UTF8_WITH_ICU
	const_utf8view_iterator begin = str.begin(), end = str.end();
	return std::all_of(begin, end, [](const string_view &code_point) {
		return u_isalnum(utf8_to_utf32(code_point));
	});
#else
	return std::all_of(str.begin(), str.end(), std::isalnum);
#endif
}

bool mint::utf8_is_alpha(const string_view &str) {
#ifdef MINT_UTF8_WITH_ICU
	const_utf8view_iterator begin = str.begin(), end = str.end();
	return std::all_of(begin, end, [](const string_view &code_point) {
		return u_isalpha(utf8_to_utf32(code_point));
	});
#else
	return std::all_of(str.begin(), str.end(), std::isalpha);
#endif
}

bool mint::utf8_is_digit(const string_view &str) {
#ifdef MINT_UTF8_WITH_ICU
	const_utf8view_iterator begin = str.begin(), end = str.end();
	return std::all_of(begin, end, [](const string_view &code_point) {
		return u_isdigit(utf8_to_utf32(code_point));
	});
#else
	return std::all_of(str.begin(), str.end(), std::isdigit);
#endif
}

bool mint::utf8_is_blank(const string_view &str) {
#ifdef MINT_UTF8_WITH_ICU
	const_utf8view_iterator begin = str.begin(), end = str.end();
	return std::all_of(begin, end, [](const string_view &code_point) {
		return u_isblank(utf8_to_utf32(code_point));
	});
#else
	return std::all_of(str.begin(), str.end(), std::isblank);
#endif
}

bool mint::utf8_is_space(const string_view &str) {
#ifdef MINT_UTF8_WITH_ICU
	const_utf8view_iterator begin = str.begin(), end = str.end();
	return std::all_of(begin, end, [](const string_view &code_point) {
		return u_isspace(utf8_to_utf32(code_point));
	});
#else
	return std::all_of(str.begin(), str.end(), std::isspace);
#endif
}

bool mint::utf8_is_cntrl(const string_view &str) {
#ifdef MINT_UTF8_WITH_ICU
	const_utf8view_iterator begin = str.begin(), end = str.end();
	return std::all_of(begin, end, [](const string_view &code_point) {
		return u_iscntrl(utf8_to_utf32(code_point));
	});
#else
	return std::all_of(str.begin(), str.end(), std::iscntrl);
#endif
}

bool mint::utf8_is_graph(const string_view &str) {
#ifdef MINT_UTF8_WITH_ICU
	const_utf8view_iterator begin = str.begin(), end = str.end();
	return std::all_of(begin, end, [](const string_view &code_point) {
		return u_isgraph(utf8_to_utf32(code_point));
	});
#else
	return std::all_of(str.begin(), str.end(), std::isgraph);
#endif
}

bool mint::utf8_is_print(const string_view &str) {
#ifdef MINT_UTF8_WITH_ICU
	const_utf8view_iterator begin = str.begin(), end = str.end();
	return std::all_of(begin, end, [](const string_view &code_point) {
		return u_isprint(utf8_to_utf32(code_point));
	});
#else
	return std::all_of(str.begin(), str.end(), std::isprint);
#endif
}

bool mint::utf8_is_punct(const string_view &str) {
#ifdef MINT_UTF8_WITH_ICU
	const_utf8view_iterator begin = str.begin(), end = str.end();
	return std::all_of(begin, end, [](const string_view &code_point) {
		return u_ispunct(utf8_to_utf32(code_point));
	});
#else
	return std::all_of(str.begin(), str.end(), std::ispunct);
#endif
}

bool mint::utf8_is_lower(const string_view &str) {
#ifdef MINT_UTF8_WITH_ICU
	const_utf8view_iterator begin = str.begin(), end = str.end();
	return std::all_of(begin, end, [](const string_view &code_point) {
		return u_islower(utf8_to_utf32(code_point));
	});
#else
	return std::all_of(str.begin(), str.end(), std::islower);
#endif
}

bool mint::utf8_is_upper(const string_view &str) {
#ifdef MINT_UTF8_WITH_ICU
	const_utf8view_iterator begin = str.begin(), end = str.end();
	return std::all_of(begin, end, [](const string_view &code_point) {
		return u_isupper(utf8_to_utf32(code_point));
	});
#else
	return std::all_of(str.begin(), str.end(), std::isupper);
#endif
}

string mint::utf8_to_lower(const string_view &str) {
	string lower;
#ifdef MINT_UTF8_WITH_ICU
	for (const_utf8view_iterator it = str.begin(); it != str.end(); ++it) {
		lower.append(utf8_from_utf32(u_tolower(utf8_to_utf32(*it))));
	}
#else
	std::transform(str.begin(), str.end(), std::back_inserter(lower), std::tolower);
#endif
	return lower;
}

string mint::utf8_to_upper(const string_view &str) {
	string upper;
#ifdef MINT_UTF8_WITH_ICU
	for (const_utf8view_iterator it = str.begin(); it != str.end(); ++it) {
		upper.append(utf8_from_utf32(u_toupper(utf8_to_utf32(*it))));
	}
#else
	std::transform(str.begin(), str.end(), std::back_inserter(upper), std::toupper);
#endif
	return upper;
}
