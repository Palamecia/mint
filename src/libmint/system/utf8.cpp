/**
 * Copyright (c) 2026 Gauvain CHERY.
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
#include "mint/config.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <unicode/uchar.h>
#include <unicode/umachine.h>
#include <unicode/urename.h>

using namespace mint;

namespace {

const constexpr std::array first_byte_mark = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};

// clang-format off
const constexpr std::array trailing_bytes_for_utf8 = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5,
};
// clang-format on

// clang-format off
const constexpr std::array offsets_from_utf8 = {
	0x00000000UL, 0x00003080UL, 0x000E2080UL,
	0x03C82080UL, 0xFA082080UL, 0x82082080UL,
};
// clang-format on

constexpr const auto utf8_replacement_char = std::to_array<std::uint8_t>({0xEF, 0xBF, 0xBD, 0x00});

constexpr const UChar32 utf32_max_legal = 0x0010FFFF;
constexpr const UChar32 utf32_sur_high_start = 0xD800;
constexpr const UChar32 utf32_sur_low_end = 0xDFFF;
constexpr const UChar32 utf32_replacement_char = 0x0000FFFD;

struct Interval {
	UChar32 first;
	UChar32 last;
};

/* auxiliary function for binary search in interval table */
bool bisearch(UChar32 ucs, std::span<const Interval> table) {
	std::size_t min = 0;
	std::size_t max = table.size() - 1;
	if (ucs < table[min].first || ucs > table[max].last) {
		return false;
	}
	while (max >= min) {
		const auto mid = (min + max) / 2;
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

bool mk_is_wide_char(UChar32 ucs) {
	static const std::array wide {
	    Interval {.first = 0x1100, .last = 0x115f},
	    Interval {.first = 0x231a, .last = 0x231b},
	    Interval {.first = 0x2329, .last = 0x232a},
	    Interval {.first = 0x23e9, .last = 0x23ec},
	    Interval {.first = 0x23f0, .last = 0x23f0},
	    Interval {.first = 0x23f3, .last = 0x23f3},
	    Interval {.first = 0x25fd, .last = 0x25fe},
	    Interval {.first = 0x2614, .last = 0x2615},
	    Interval {.first = 0x2648, .last = 0x2653},
	    Interval {.first = 0x267f, .last = 0x267f},
	    Interval {.first = 0x2693, .last = 0x2693},
	    Interval {.first = 0x26a1, .last = 0x26a1},
	    Interval {.first = 0x26aa, .last = 0x26ab},
	    Interval {.first = 0x26bd, .last = 0x26be},
	    Interval {.first = 0x26c4, .last = 0x26c5},
	    Interval {.first = 0x26ce, .last = 0x26ce},
	    Interval {.first = 0x26d4, .last = 0x26d4},
	    Interval {.first = 0x26ea, .last = 0x26ea},
	    Interval {.first = 0x26f2, .last = 0x26f3},
	    Interval {.first = 0x26f5, .last = 0x26f5},
	    Interval {.first = 0x26fa, .last = 0x26fa},
	    Interval {.first = 0x26fd, .last = 0x26fd},
	    Interval {.first = 0x2705, .last = 0x2705},
	    Interval {.first = 0x270a, .last = 0x270b},
	    Interval {.first = 0x2728, .last = 0x2728},
	    Interval {.first = 0x274c, .last = 0x274c},
	    Interval {.first = 0x274e, .last = 0x274e},
	    Interval {.first = 0x2753, .last = 0x2755},
	    Interval {.first = 0x2757, .last = 0x2757},
	    Interval {.first = 0x2795, .last = 0x2797},
	    Interval {.first = 0x27b0, .last = 0x27b0},
	    Interval {.first = 0x27bf, .last = 0x27bf},
	    Interval {.first = 0x2b1b, .last = 0x2b1c},
	    Interval {.first = 0x2b50, .last = 0x2b50},
	    Interval {.first = 0x2b55, .last = 0x2b55},
	    Interval {.first = 0x2e80, .last = 0x2fdf},
	    Interval {.first = 0x2ff0, .last = 0x303e},
	    Interval {.first = 0x3040, .last = 0x3247},
	    Interval {.first = 0x3250, .last = 0x4dbf},
	    Interval {.first = 0x4e00, .last = 0xa4cf},
	    Interval {.first = 0xa960, .last = 0xa97f},
	    Interval {.first = 0xac00, .last = 0xd7a3},
	    Interval {.first = 0xf900, .last = 0xfaff},
	    Interval {.first = 0xfe10, .last = 0xfe19},
	    Interval {.first = 0xfe30, .last = 0xfe6f},
	    Interval {.first = 0xff01, .last = 0xff60},
	    Interval {.first = 0xffe0, .last = 0xffe6},
	    Interval {.first = 0x16fe0, .last = 0x16fe1},
	    Interval {.first = 0x17000, .last = 0x18aff},
	    Interval {.first = 0x1b000, .last = 0x1b12f},
	    Interval {.first = 0x1b170, .last = 0x1b2ff},
	    Interval {.first = 0x1f004, .last = 0x1f004},
	    Interval {.first = 0x1f0cf, .last = 0x1f0cf},
	    Interval {.first = 0x1f18e, .last = 0x1f18e},
	    Interval {.first = 0x1f191, .last = 0x1f19a},
	    Interval {.first = 0x1f200, .last = 0x1f202},
	    Interval {.first = 0x1f210, .last = 0x1f23b},
	    Interval {.first = 0x1f240, .last = 0x1f248},
	    Interval {.first = 0x1f250, .last = 0x1f251},
	    Interval {.first = 0x1f260, .last = 0x1f265},
	    Interval {.first = 0x1f300, .last = 0x1f320},
	    Interval {.first = 0x1f32d, .last = 0x1f335},
	    Interval {.first = 0x1f337, .last = 0x1f37c},
	    Interval {.first = 0x1f37e, .last = 0x1f393},
	    Interval {.first = 0x1f3a0, .last = 0x1f3ca},
	    Interval {.first = 0x1f3cf, .last = 0x1f3d3},
	    Interval {.first = 0x1f3e0, .last = 0x1f3f0},
	    Interval {.first = 0x1f3f4, .last = 0x1f3f4},
	    Interval {.first = 0x1f3f8, .last = 0x1f43e},
	    Interval {.first = 0x1f440, .last = 0x1f440},
	    Interval {.first = 0x1f442, .last = 0x1f4fc},
	    Interval {.first = 0x1f4ff, .last = 0x1f53d},
	    Interval {.first = 0x1f54b, .last = 0x1f54e},
	    Interval {.first = 0x1f550, .last = 0x1f567},
	    Interval {.first = 0x1f57a, .last = 0x1f57a},
	    Interval {.first = 0x1f595, .last = 0x1f596},
	    Interval {.first = 0x1f5a4, .last = 0x1f5a4},
	    Interval {.first = 0x1f5fb, .last = 0x1f64f},
	    Interval {.first = 0x1f680, .last = 0x1f6c5},
	    Interval {.first = 0x1f6cc, .last = 0x1f6cc},
	    Interval {.first = 0x1f6d0, .last = 0x1f6d2},
	    Interval {.first = 0x1f6eb, .last = 0x1f6ec},
	    Interval {.first = 0x1f6f4, .last = 0x1f6f8},
	    Interval {.first = 0x1f910, .last = 0x1f93e},
	    Interval {.first = 0x1f940, .last = 0x1f94c},
	    Interval {.first = 0x1f950, .last = 0x1f96b},
	    Interval {.first = 0x1f980, .last = 0x1f997},
	    Interval {.first = 0x1f9c0, .last = 0x1f9c0},
	    Interval {.first = 0x1f9d0, .last = 0x1f9e6},
	    Interval {.first = 0x20000, .last = 0x2fffd},
	    Interval {.first = 0x30000, .last = 0x3fffd},
	};

	return bisearch(ucs, wide);
}

int mk_wcwidth(UChar32 ucs) {
	/* sorted list of non-overlapping intervals of non-spacing characters */
	/* generated by "uniset +cat=Me +cat=Mn +cat=Cf -00AD +1160-11FF +200B c" */
	static const std::array combining {
	    Interval {.first = 0x00ad, .last = 0x00ad},
	    Interval {.first = 0x0300, .last = 0x036f},
	    Interval {.first = 0x0483, .last = 0x0489},
	    Interval {.first = 0x0591, .last = 0x05bd},
	    Interval {.first = 0x05bf, .last = 0x05bf},
	    Interval {.first = 0x05c1, .last = 0x05c2},
	    Interval {.first = 0x05c4, .last = 0x05c5},
	    Interval {.first = 0x05c7, .last = 0x05c7},
	    Interval {.first = 0x0610, .last = 0x061a},
	    Interval {.first = 0x061c, .last = 0x061c},
	    Interval {.first = 0x064b, .last = 0x065f},
	    Interval {.first = 0x0670, .last = 0x0670},
	    Interval {.first = 0x06d6, .last = 0x06dc},
	    Interval {.first = 0x06df, .last = 0x06e4},
	    Interval {.first = 0x06e7, .last = 0x06e8},
	    Interval {.first = 0x06ea, .last = 0x06ed},
	    Interval {.first = 0x0711, .last = 0x0711},
	    Interval {.first = 0x0730, .last = 0x074a},
	    Interval {.first = 0x07a6, .last = 0x07b0},
	    Interval {.first = 0x07eb, .last = 0x07f3},
	    Interval {.first = 0x0816, .last = 0x0819},
	    Interval {.first = 0x081b, .last = 0x0823},
	    Interval {.first = 0x0825, .last = 0x0827},
	    Interval {.first = 0x0829, .last = 0x082d},
	    Interval {.first = 0x0859, .last = 0x085b},
	    Interval {.first = 0x08d4, .last = 0x08e1},
	    Interval {.first = 0x08e3, .last = 0x0902},
	    Interval {.first = 0x093a, .last = 0x093a},
	    Interval {.first = 0x093c, .last = 0x093c},
	    Interval {.first = 0x0941, .last = 0x0948},
	    Interval {.first = 0x094d, .last = 0x094d},
	    Interval {.first = 0x0951, .last = 0x0957},
	    Interval {.first = 0x0962, .last = 0x0963},
	    Interval {.first = 0x0981, .last = 0x0981},
	    Interval {.first = 0x09bc, .last = 0x09bc},
	    Interval {.first = 0x09c1, .last = 0x09c4},
	    Interval {.first = 0x09cd, .last = 0x09cd},
	    Interval {.first = 0x09e2, .last = 0x09e3},
	    Interval {.first = 0x0a01, .last = 0x0a02},
	    Interval {.first = 0x0a3c, .last = 0x0a3c},
	    Interval {.first = 0x0a41, .last = 0x0a42},
	    Interval {.first = 0x0a47, .last = 0x0a48},
	    Interval {.first = 0x0a4b, .last = 0x0a4d},
	    Interval {.first = 0x0a51, .last = 0x0a51},
	    Interval {.first = 0x0a70, .last = 0x0a71},
	    Interval {.first = 0x0a75, .last = 0x0a75},
	    Interval {.first = 0x0a81, .last = 0x0a82},
	    Interval {.first = 0x0abc, .last = 0x0abc},
	    Interval {.first = 0x0ac1, .last = 0x0ac5},
	    Interval {.first = 0x0ac7, .last = 0x0ac8},
	    Interval {.first = 0x0acd, .last = 0x0acd},
	    Interval {.first = 0x0ae2, .last = 0x0ae3},
	    Interval {.first = 0x0afa, .last = 0x0aff},
	    Interval {.first = 0x0b01, .last = 0x0b01},
	    Interval {.first = 0x0b3c, .last = 0x0b3c},
	    Interval {.first = 0x0b3f, .last = 0x0b3f},
	    Interval {.first = 0x0b41, .last = 0x0b44},
	    Interval {.first = 0x0b4d, .last = 0x0b4d},
	    Interval {.first = 0x0b56, .last = 0x0b56},
	    Interval {.first = 0x0b62, .last = 0x0b63},
	    Interval {.first = 0x0b82, .last = 0x0b82},
	    Interval {.first = 0x0bc0, .last = 0x0bc0},
	    Interval {.first = 0x0bcd, .last = 0x0bcd},
	    Interval {.first = 0x0c00, .last = 0x0c00},
	    Interval {.first = 0x0c3e, .last = 0x0c40},
	    Interval {.first = 0x0c46, .last = 0x0c48},
	    Interval {.first = 0x0c4a, .last = 0x0c4d},
	    Interval {.first = 0x0c55, .last = 0x0c56},
	    Interval {.first = 0x0c62, .last = 0x0c63},
	    Interval {.first = 0x0c81, .last = 0x0c81},
	    Interval {.first = 0x0cbc, .last = 0x0cbc},
	    Interval {.first = 0x0cbf, .last = 0x0cbf},
	    Interval {.first = 0x0cc6, .last = 0x0cc6},
	    Interval {.first = 0x0ccc, .last = 0x0ccd},
	    Interval {.first = 0x0ce2, .last = 0x0ce3},
	    Interval {.first = 0x0d00, .last = 0x0d01},
	    Interval {.first = 0x0d3b, .last = 0x0d3c},
	    Interval {.first = 0x0d41, .last = 0x0d44},
	    Interval {.first = 0x0d4d, .last = 0x0d4d},
	    Interval {.first = 0x0d62, .last = 0x0d63},
	    Interval {.first = 0x0dca, .last = 0x0dca},
	    Interval {.first = 0x0dd2, .last = 0x0dd4},
	    Interval {.first = 0x0dd6, .last = 0x0dd6},
	    Interval {.first = 0x0e31, .last = 0x0e31},
	    Interval {.first = 0x0e34, .last = 0x0e3a},
	    Interval {.first = 0x0e47, .last = 0x0e4e},
	    Interval {.first = 0x0eb1, .last = 0x0eb1},
	    Interval {.first = 0x0eb4, .last = 0x0eb9},
	    Interval {.first = 0x0ebb, .last = 0x0ebc},
	    Interval {.first = 0x0ec8, .last = 0x0ecd},
	    Interval {.first = 0x0f18, .last = 0x0f19},
	    Interval {.first = 0x0f35, .last = 0x0f35},
	    Interval {.first = 0x0f37, .last = 0x0f37},
	    Interval {.first = 0x0f39, .last = 0x0f39},
	    Interval {.first = 0x0f71, .last = 0x0f7e},
	    Interval {.first = 0x0f80, .last = 0x0f84},
	    Interval {.first = 0x0f86, .last = 0x0f87},
	    Interval {.first = 0x0f8d, .last = 0x0f97},
	    Interval {.first = 0x0f99, .last = 0x0fbc},
	    Interval {.first = 0x0fc6, .last = 0x0fc6},
	    Interval {.first = 0x102d, .last = 0x1030},
	    Interval {.first = 0x1032, .last = 0x1037},
	    Interval {.first = 0x1039, .last = 0x103a},
	    Interval {.first = 0x103d, .last = 0x103e},
	    Interval {.first = 0x1058, .last = 0x1059},
	    Interval {.first = 0x105e, .last = 0x1060},
	    Interval {.first = 0x1071, .last = 0x1074},
	    Interval {.first = 0x1082, .last = 0x1082},
	    Interval {.first = 0x1085, .last = 0x1086},
	    Interval {.first = 0x108d, .last = 0x108d},
	    Interval {.first = 0x109d, .last = 0x109d},
	    Interval {.first = 0x1160, .last = 0x11ff},
	    Interval {.first = 0x135d, .last = 0x135f},
	    Interval {.first = 0x1712, .last = 0x1714},
	    Interval {.first = 0x1732, .last = 0x1734},
	    Interval {.first = 0x1752, .last = 0x1753},
	    Interval {.first = 0x1772, .last = 0x1773},
	    Interval {.first = 0x17b4, .last = 0x17b5},
	    Interval {.first = 0x17b7, .last = 0x17bd},
	    Interval {.first = 0x17c6, .last = 0x17c6},
	    Interval {.first = 0x17c9, .last = 0x17d3},
	    Interval {.first = 0x17dd, .last = 0x17dd},
	    Interval {.first = 0x180b, .last = 0x180e},
	    Interval {.first = 0x1885, .last = 0x1886},
	    Interval {.first = 0x18a9, .last = 0x18a9},
	    Interval {.first = 0x1920, .last = 0x1922},
	    Interval {.first = 0x1927, .last = 0x1928},
	    Interval {.first = 0x1932, .last = 0x1932},
	    Interval {.first = 0x1939, .last = 0x193b},
	    Interval {.first = 0x1a17, .last = 0x1a18},
	    Interval {.first = 0x1a1b, .last = 0x1a1b},
	    Interval {.first = 0x1a56, .last = 0x1a56},
	    Interval {.first = 0x1a58, .last = 0x1a5e},
	    Interval {.first = 0x1a60, .last = 0x1a60},
	    Interval {.first = 0x1a62, .last = 0x1a62},
	    Interval {.first = 0x1a65, .last = 0x1a6c},
	    Interval {.first = 0x1a73, .last = 0x1a7c},
	    Interval {.first = 0x1a7f, .last = 0x1a7f},
	    Interval {.first = 0x1ab0, .last = 0x1abe},
	    Interval {.first = 0x1b00, .last = 0x1b03},
	    Interval {.first = 0x1b34, .last = 0x1b34},
	    Interval {.first = 0x1b36, .last = 0x1b3a},
	    Interval {.first = 0x1b3c, .last = 0x1b3c},
	    Interval {.first = 0x1b42, .last = 0x1b42},
	    Interval {.first = 0x1b6b, .last = 0x1b73},
	    Interval {.first = 0x1b80, .last = 0x1b81},
	    Interval {.first = 0x1ba2, .last = 0x1ba5},
	    Interval {.first = 0x1ba8, .last = 0x1ba9},
	    Interval {.first = 0x1bab, .last = 0x1bad},
	    Interval {.first = 0x1be6, .last = 0x1be6},
	    Interval {.first = 0x1be8, .last = 0x1be9},
	    Interval {.first = 0x1bed, .last = 0x1bed},
	    Interval {.first = 0x1bef, .last = 0x1bf1},
	    Interval {.first = 0x1c2c, .last = 0x1c33},
	    Interval {.first = 0x1c36, .last = 0x1c37},
	    Interval {.first = 0x1cd0, .last = 0x1cd2},
	    Interval {.first = 0x1cd4, .last = 0x1ce0},
	    Interval {.first = 0x1ce2, .last = 0x1ce8},
	    Interval {.first = 0x1ced, .last = 0x1ced},
	    Interval {.first = 0x1cf4, .last = 0x1cf4},
	    Interval {.first = 0x1cf8, .last = 0x1cf9},
	    Interval {.first = 0x1dc0, .last = 0x1df9},
	    Interval {.first = 0x1dfb, .last = 0x1dff},
	    Interval {.first = 0x200b, .last = 0x200f},
	    Interval {.first = 0x202a, .last = 0x202e},
	    Interval {.first = 0x2060, .last = 0x2064},
	    Interval {.first = 0x2066, .last = 0x206f},
	    Interval {.first = 0x20d0, .last = 0x20f0},
	    Interval {.first = 0x2cef, .last = 0x2cf1},
	    Interval {.first = 0x2d7f, .last = 0x2d7f},
	    Interval {.first = 0x2de0, .last = 0x2dff},
	    Interval {.first = 0x302a, .last = 0x302d},
	    Interval {.first = 0x3099, .last = 0x309a},
	    Interval {.first = 0xa66f, .last = 0xa672},
	    Interval {.first = 0xa674, .last = 0xa67d},
	    Interval {.first = 0xa69e, .last = 0xa69f},
	    Interval {.first = 0xa6f0, .last = 0xa6f1},
	    Interval {.first = 0xa802, .last = 0xa802},
	    Interval {.first = 0xa806, .last = 0xa806},
	    Interval {.first = 0xa80b, .last = 0xa80b},
	    Interval {.first = 0xa825, .last = 0xa826},
	    Interval {.first = 0xa8c4, .last = 0xa8c5},
	    Interval {.first = 0xa8e0, .last = 0xa8f1},
	    Interval {.first = 0xa926, .last = 0xa92d},
	    Interval {.first = 0xa947, .last = 0xa951},
	    Interval {.first = 0xa980, .last = 0xa982},
	    Interval {.first = 0xa9b3, .last = 0xa9b3},
	    Interval {.first = 0xa9b6, .last = 0xa9b9},
	    Interval {.first = 0xa9bc, .last = 0xa9bc},
	    Interval {.first = 0xa9e5, .last = 0xa9e5},
	    Interval {.first = 0xaa29, .last = 0xaa2e},
	    Interval {.first = 0xaa31, .last = 0xaa32},
	    Interval {.first = 0xaa35, .last = 0xaa36},
	    Interval {.first = 0xaa43, .last = 0xaa43},
	    Interval {.first = 0xaa4c, .last = 0xaa4c},
	    Interval {.first = 0xaa7c, .last = 0xaa7c},
	    Interval {.first = 0xaab0, .last = 0xaab0},
	    Interval {.first = 0xaab2, .last = 0xaab4},
	    Interval {.first = 0xaab7, .last = 0xaab8},
	    Interval {.first = 0xaabe, .last = 0xaabf},
	    Interval {.first = 0xaac1, .last = 0xaac1},
	    Interval {.first = 0xaaec, .last = 0xaaed},
	    Interval {.first = 0xaaf6, .last = 0xaaf6},
	    Interval {.first = 0xabe5, .last = 0xabe5},
	    Interval {.first = 0xabe8, .last = 0xabe8},
	    Interval {.first = 0xabed, .last = 0xabed},
	    Interval {.first = 0xfb1e, .last = 0xfb1e},
	    Interval {.first = 0xfe00, .last = 0xfe0f},
	    Interval {.first = 0xfe20, .last = 0xfe2f},
	    Interval {.first = 0xfeff, .last = 0xfeff},
	    Interval {.first = 0xfff9, .last = 0xfffb},
	    Interval {.first = 0x101fd, .last = 0x101fd},
	    Interval {.first = 0x102e0, .last = 0x102e0},
	    Interval {.first = 0x10376, .last = 0x1037a},
	    Interval {.first = 0x10a01, .last = 0x10a03},
	    Interval {.first = 0x10a05, .last = 0x10a06},
	    Interval {.first = 0x10a0c, .last = 0x10a0f},
	    Interval {.first = 0x10a38, .last = 0x10a3a},
	    Interval {.first = 0x10a3f, .last = 0x10a3f},
	    Interval {.first = 0x10ae5, .last = 0x10ae6},
	    Interval {.first = 0x11001, .last = 0x11001},
	    Interval {.first = 0x11038, .last = 0x11046},
	    Interval {.first = 0x1107f, .last = 0x11081},
	    Interval {.first = 0x110b3, .last = 0x110b6},
	    Interval {.first = 0x110b9, .last = 0x110ba},
	    Interval {.first = 0x11100, .last = 0x11102},
	    Interval {.first = 0x11127, .last = 0x1112b},
	    Interval {.first = 0x1112d, .last = 0x11134},
	    Interval {.first = 0x11173, .last = 0x11173},
	    Interval {.first = 0x11180, .last = 0x11181},
	    Interval {.first = 0x111b6, .last = 0x111be},
	    Interval {.first = 0x111ca, .last = 0x111cc},
	    Interval {.first = 0x1122f, .last = 0x11231},
	    Interval {.first = 0x11234, .last = 0x11234},
	    Interval {.first = 0x11236, .last = 0x11237},
	    Interval {.first = 0x1123e, .last = 0x1123e},
	    Interval {.first = 0x112df, .last = 0x112df},
	    Interval {.first = 0x112e3, .last = 0x112ea},
	    Interval {.first = 0x11300, .last = 0x11301},
	    Interval {.first = 0x1133c, .last = 0x1133c},
	    Interval {.first = 0x11340, .last = 0x11340},
	    Interval {.first = 0x11366, .last = 0x1136c},
	    Interval {.first = 0x11370, .last = 0x11374},
	    Interval {.first = 0x11438, .last = 0x1143f},
	    Interval {.first = 0x11442, .last = 0x11444},
	    Interval {.first = 0x11446, .last = 0x11446},
	    Interval {.first = 0x114b3, .last = 0x114b8},
	    Interval {.first = 0x114ba, .last = 0x114ba},
	    Interval {.first = 0x114bf, .last = 0x114c0},
	    Interval {.first = 0x114c2, .last = 0x114c3},
	    Interval {.first = 0x115b2, .last = 0x115b5},
	    Interval {.first = 0x115bc, .last = 0x115bd},
	    Interval {.first = 0x115bf, .last = 0x115c0},
	    Interval {.first = 0x115dc, .last = 0x115dd},
	    Interval {.first = 0x11633, .last = 0x1163a},
	    Interval {.first = 0x1163d, .last = 0x1163d},
	    Interval {.first = 0x1163f, .last = 0x11640},
	    Interval {.first = 0x116ab, .last = 0x116ab},
	    Interval {.first = 0x116ad, .last = 0x116ad},
	    Interval {.first = 0x116b0, .last = 0x116b5},
	    Interval {.first = 0x116b7, .last = 0x116b7},
	    Interval {.first = 0x1171d, .last = 0x1171f},
	    Interval {.first = 0x11722, .last = 0x11725},
	    Interval {.first = 0x11727, .last = 0x1172b},
	    Interval {.first = 0x11a01, .last = 0x11a06},
	    Interval {.first = 0x11a09, .last = 0x11a0a},
	    Interval {.first = 0x11a33, .last = 0x11a38},
	    Interval {.first = 0x11a3b, .last = 0x11a3e},
	    Interval {.first = 0x11a47, .last = 0x11a47},
	    Interval {.first = 0x11a51, .last = 0x11a56},
	    Interval {.first = 0x11a59, .last = 0x11a5b},
	    Interval {.first = 0x11a8a, .last = 0x11a96},
	    Interval {.first = 0x11a98, .last = 0x11a99},
	    Interval {.first = 0x11c30, .last = 0x11c36},
	    Interval {.first = 0x11c38, .last = 0x11c3d},
	    Interval {.first = 0x11c3f, .last = 0x11c3f},
	    Interval {.first = 0x11c92, .last = 0x11ca7},
	    Interval {.first = 0x11caa, .last = 0x11cb0},
	    Interval {.first = 0x11cb2, .last = 0x11cb3},
	    Interval {.first = 0x11cb5, .last = 0x11cb6},
	    Interval {.first = 0x11d31, .last = 0x11d36},
	    Interval {.first = 0x11d3a, .last = 0x11d3a},
	    Interval {.first = 0x11d3c, .last = 0x11d3d},
	    Interval {.first = 0x11d3f, .last = 0x11d45},
	    Interval {.first = 0x11d47, .last = 0x11d47},
	    Interval {.first = 0x16af0, .last = 0x16af4},
	    Interval {.first = 0x16b30, .last = 0x16b36},
	    Interval {.first = 0x16f8f, .last = 0x16f92},
	    Interval {.first = 0x1bc9d, .last = 0x1bc9e},
	    Interval {.first = 0x1bca0, .last = 0x1bca3},
	    Interval {.first = 0x1d167, .last = 0x1d169},
	    Interval {.first = 0x1d173, .last = 0x1d182},
	    Interval {.first = 0x1d185, .last = 0x1d18b},
	    Interval {.first = 0x1d1aa, .last = 0x1d1ad},
	    Interval {.first = 0x1d242, .last = 0x1d244},
	    Interval {.first = 0x1da00, .last = 0x1da36},
	    Interval {.first = 0x1da3b, .last = 0x1da6c},
	    Interval {.first = 0x1da75, .last = 0x1da75},
	    Interval {.first = 0x1da84, .last = 0x1da84},
	    Interval {.first = 0x1da9b, .last = 0x1da9f},
	    Interval {.first = 0x1daa1, .last = 0x1daaf},
	    Interval {.first = 0x1e000, .last = 0x1e006},
	    Interval {.first = 0x1e008, .last = 0x1e018},
	    Interval {.first = 0x1e01b, .last = 0x1e021},
	    Interval {.first = 0x1e023, .last = 0x1e024},
	    Interval {.first = 0x1e026, .last = 0x1e02a},
	    Interval {.first = 0x1e8d0, .last = 0x1e8d6},
	    Interval {.first = 0x1e944, .last = 0x1e94a},
	    Interval {.first = 0xe0001, .last = 0xe0001},
	    Interval {.first = 0xe0020, .last = 0xe007f},
	    Interval {.first = 0xe0100, .last = 0xe01ef},
	};

	/* test for 8-bit control characters */
	if (ucs == 0) {
		return 0;
	}
	if ((ucs < 32) || ((ucs >= 0x7f) && (ucs < 0xa0))) {
		return -1;
	}

	/* binary search in table of non-spacing characters */
	if (bisearch(ucs, combining)) {
		return 0;
	}

	/* if we arrive here, ucs is not a combining or C0/C1 control character */
	return mk_is_wide_char(ucs) ? 2 : 1;
}

UChar32 utf8_to_utf32(std::string_view code_point) {
	const auto* source = reinterpret_cast<const std::uint8_t*>(code_point.data());
	if (!utf8_begin_code_point(*source)) {
		return utf32_replacement_char;
	}
	const std::uint8_t extra_bytes_to_read = trailing_bytes_for_utf8[*source];
	if (extra_bytes_to_read > code_point.size()) {
		return utf32_replacement_char;
	}
	UChar32 ch = 0;
	switch (extra_bytes_to_read) {
	case 5:
		ch += *source++;
		ch <<= 6;
		[[fallthrough]];
	case 4:
		ch += *source++;
		ch <<= 6;
		[[fallthrough]];
	case 3:
		ch += *source++;
		ch <<= 6;
		[[fallthrough]];
	case 2:
		ch += *source++;
		ch <<= 6;
		[[fallthrough]];
	case 1:
		ch += *source++;
		ch <<= 6;
		[[fallthrough]];
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

std::string utf8_from_utf32(UChar32 code_point) {
	std::string::size_type code_point_length = 0;
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
		return reinterpret_cast<const char*>(utf8_replacement_char.data());
	}

	std::string ch(code_point_length, ' ');
	auto* target = reinterpret_cast<std::uint8_t*>(ch.data());
	target += code_point_length;

	static constexpr const UChar32 byte_mask = 0xBF;
	static constexpr const UChar32 byte_mark = 0x80;

	switch (code_point_length) {
	case 4:
		*--target = ((code_point | byte_mark) & byte_mask);
		code_point >>= 6;
		[[fallthrough]];
	case 3:
		*--target = ((code_point | byte_mark) & byte_mask);
		code_point >>= 6;
		[[fallthrough]];
	case 2:
		*--target = ((code_point | byte_mark) & byte_mask);
		code_point >>= 6;
		[[fallthrough]];
	case 1:
		*--target = (code_point | first_byte_mark[code_point_length]);
		break;
	default:
		break;
	}

	return ch;
}

}

bool mint::utf8_begin_code_point(std::uint8_t b) {
	return !((b & 0x80) && !(b & 0x40));
}

std::string_view::size_type mint::utf8_code_point_length(std::uint8_t b) {
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

std::string_view::size_type mint::utf8_code_point_count(std::string_view str) {
	std::size_t code_point_count = 0;
	for (const_utf8view_iterator it = str.begin(); it != str.end(); ++it) {
		++code_point_count;
	}
	return code_point_count;
}

std::string_view::size_type mint::utf8_byte_index_to_code_point_index(std::string_view str,
    std::string_view::difference_type byte_index) {
	return utf8_byte_index_to_code_point_index(str, static_cast<std::string_view::size_type>(byte_index));
}

std::string_view::size_type mint::utf8_byte_index_to_code_point_index(std::string_view str,
    std::string_view::size_type byte_index) {

	std::string_view::size_type code_point_index = 0;

	if (byte_index == 0) {
		return code_point_index;
	}

	for (const_utf8view_iterator i = str.begin(); i != str.end(); ++i) {
		const auto len = utf8_code_point_length(static_cast<std::uint8_t>((*i).front()));
		if (byte_index < len) {
			return std::string_view::npos;
		}
		code_point_index++;
		byte_index -= len;
		if (byte_index == 0) {
			return code_point_index;
		}
	}

	return std::string_view::npos;
}

std::string_view::size_type mint::utf8_previous_code_point_byte_index(std::string_view str,
    std::string_view::size_type byte_index) {
	if (byte_index) {
		do {
			byte_index--;
		}
		while (!utf8_begin_code_point(static_cast<std::uint8_t>(str[byte_index])));
		return byte_index;
	}
	return std::string_view::npos;
}

std::string_view::size_type mint::utf8_next_code_point_byte_index(std::string_view str,
    std::string_view::size_type byte_index) {
	return byte_index + utf8_code_point_length(static_cast<std::uint8_t>(str[byte_index]));
}

std::string_view::size_type mint::utf8_code_point_index_to_byte_index(std::string_view str,
    std::string_view::size_type code_point_index) {

	std::size_t byte_index = 0;

	if (code_point_index == 0) {
		return byte_index;
	}

	for (const_utf8view_iterator i = str.begin(); i != str.end(); ++i) {
		byte_index += utf8_code_point_length(static_cast<std::uint8_t>((*i).front()));
		if (--code_point_index == 0) {
			return byte_index;
		}
	}

	return std::string_view::npos;
}

std::string_view::size_type mint::utf8_substring_byte_count(std::string_view str,
    std::string_view::size_type code_point_index, std::string_view::size_type code_point_count) {
	std::size_t byte_count = 0;
	std::size_t i = 0;
	for (const_utf8view_iterator it = str.begin(); i < code_point_index + code_point_count && it != str.end(); ++it) {
		if (i++ >= code_point_index) {
			byte_count += it->length();
		}
	}
	return byte_count;
}

// column width of a utf8 single character sequence.
std::string_view::size_type mint::utf8_grapheme_code_point_count(std::string_view str) {
	auto b = static_cast<std::uint8_t>(str[0]);
	if (b < ' ') {
		return 0;
	}
	if (b <= 0x7F) {
		return 1;
	}
	// invalid continuation byte or invalid 0xC0, 0xC1 (check is strictly not necessary as we don't validate..)
	if (b <= 0xC1) {
		return 1;
	}
	if (b <= 0xDF) { // b >= 0xC2  // 2 bytes
		if (str.length() < 2) {
			// failed
			return 1;
		}
		const UChar32 ucs = (((b & 0x1F) << 6) | (str[1] & 0x3F));
		assert(ucs < 0xD800 || ucs > 0xDFFF);
		return mk_wcwidth(ucs);
	}
	if (b <= 0xEF) { // b >= 0xE0  // 3 bytes
		if (str.length() < 3) {
			// failed
			return 1;
		}
		const UChar32 ucs = (((b & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F));
		return mk_wcwidth(ucs);
	}
	if (b <= 0xF4) { // b >= 0xF0  // 4 bytes
		if (str.length() < 4) {
			// failed
			return 1;
		}
		const UChar32 ucs = (((b & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F));
		return mk_wcwidth(ucs);
	}
	// failed
	return 1;
}

std::strong_ordering mint::utf8_compare(std::string_view s1, std::string_view s2) {
	return s1 <=> s2;
}

std::strong_ordering mint::utf8_compare_substring(std::string_view s1, std::string_view s2,
    std::string_view::size_type code_point_count) {
	return s1.compare(0, utf8_substring_byte_count(s1, 0, code_point_count), //
	           s2, 0, utf8_substring_byte_count(s2, 0, code_point_count))
	       <=> 0;
}

std::strong_ordering mint::utf8_compare_case_insensitive(std::string_view s1, std::string_view s2) {
	if (std::strong_ordering diff = s1.size() <=> s2.size(); diff != std::strong_ordering::equal) {
		return diff;
	}
	for (const auto& [s1_code_point, s2_code_point] : std::views::zip(views::utf8(s1), views::utf8(s2))) {
		if (std::strong_ordering diff = utf8_to_upper(s1_code_point).compare(utf8_to_upper(s2_code_point)) <=> 0;
		    diff != std::strong_ordering::equal) {
			return diff;
		}
	}
	return std::strong_ordering::equal;
}

std::strong_ordering mint::utf8_compare_substring_case_insensitive(std::string_view s1, std::string_view s2,
    std::string_view::size_type code_point_count) {
	return utf8_compare_case_insensitive(s1.substr(0, utf8_substring_byte_count(s1, 0, code_point_count)),
	    s2.substr(0, utf8_substring_byte_count(s2, 0, code_point_count)));
}

bool mint::utf8_is_alnum(std::string_view str) {
	return std::ranges::all_of(views::utf8(str), [](std::string_view code_point) {
		return u_isalnum(utf8_to_utf32(code_point));
	});
}

bool mint::utf8_is_alpha(std::string_view str) {
	return std::ranges::all_of(views::utf8(str), [](std::string_view code_point) {
		return u_isalpha(utf8_to_utf32(code_point));
	});
}

bool mint::utf8_is_digit(std::string_view str) {
	return std::ranges::all_of(views::utf8(str), [](std::string_view code_point) {
		return u_isdigit(utf8_to_utf32(code_point));
	});
}

bool mint::utf8_is_blank(std::string_view str) {
	return std::ranges::all_of(views::utf8(str), [](std::string_view code_point) {
		return u_isblank(utf8_to_utf32(code_point));
	});
}

bool mint::utf8_is_space(std::string_view str) {
	return std::ranges::all_of(views::utf8(str), [](std::string_view code_point) {
		return u_isspace(utf8_to_utf32(code_point));
	});
}

bool mint::utf8_is_cntrl(std::string_view str) {
	return std::ranges::all_of(views::utf8(str), [](std::string_view code_point) {
		return u_iscntrl(utf8_to_utf32(code_point));
	});
}

bool mint::utf8_is_graph(std::string_view str) {
	return std::ranges::all_of(views::utf8(str), [](std::string_view code_point) {
		return u_isgraph(utf8_to_utf32(code_point));
	});
}

bool mint::utf8_is_print(std::string_view str) {
	return std::ranges::all_of(views::utf8(str), [](std::string_view code_point) {
		return u_isprint(utf8_to_utf32(code_point));
	});
}

bool mint::utf8_is_punct(std::string_view str) {
	return std::ranges::all_of(views::utf8(str), [](std::string_view code_point) {
		return u_ispunct(utf8_to_utf32(code_point));
	});
}

bool mint::utf8_is_lower(std::string_view str) {
	return std::ranges::all_of(views::utf8(str), [](std::string_view code_point) {
		return u_islower(utf8_to_utf32(code_point));
	});
}

bool mint::utf8_is_upper(std::string_view str) {
	return std::ranges::all_of(views::utf8(str), [](std::string_view code_point) {
		return u_isupper(utf8_to_utf32(code_point));
	});
}

std::string mint::utf8_to_lower(std::string_view str) {
	std::string lower;
	lower.reserve(str.size());
	for (const auto code_point : views::utf8(str)) {
		lower.append(utf8_from_utf32(u_tolower(utf8_to_utf32(code_point))));
	}
	return lower;
}

std::string mint::utf8_to_upper(std::string_view str) {
	std::string upper;
	upper.reserve(str.size());
	for (const auto code_point : views::utf8(str)) {
		upper.append(utf8_from_utf32(u_toupper(utf8_to_utf32(code_point))));
	}
	return upper;
}
