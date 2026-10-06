#include "mint/system/string.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>

TEST(format_integer, no_width) {
	EXPECT_EQ("42", mint::format_integer(42));
	EXPECT_EQ("-42", mint::format_integer(-42));
	EXPECT_EQ("0", mint::format_integer(0));
	EXPECT_EQ("005", mint::format_integer(5, mint::decimal_base, mint::unknown_size, 3));
}

TEST(format_integer, width_and_alignment) {
	EXPECT_EQ("   42", mint::format_integer(42, mint::decimal_base, 5));
	EXPECT_EQ("42   ", mint::format_integer(42, mint::decimal_base, 5, mint::unknown_precision, mint::string_left));
	EXPECT_EQ("00042", mint::format_integer(42, mint::decimal_base, 5, mint::unknown_precision, mint::string_zeropad));
	EXPECT_EQ("-0042", mint::format_integer(-42, mint::decimal_base, 5, mint::unknown_precision,
	                       mint::string_sign | mint::string_zeropad));
	EXPECT_EQ("  +42", mint::format_integer(42, mint::decimal_base, 5, mint::unknown_precision,
	                       mint::string_sign | mint::string_plus));
	EXPECT_EQ("  -42", mint::format_integer(-42, mint::decimal_base, 5));
}

TEST(format_integer, prefixes) {
	EXPECT_EQ("0x0000ff", mint::format_integer(255, mint::hexadecimal_base, 8, mint::unknown_precision,
	                          mint::string_special | mint::string_zeropad));
	EXPECT_EQ("0XFF", mint::format_integer(255, mint::hexadecimal_base, mint::unknown_size, mint::unknown_precision,
	                      mint::string_special | mint::string_large));
	EXPECT_EQ("0b101",
	    mint::format_integer(5, mint::binary_base, mint::unknown_size, mint::unknown_precision, mint::string_special));
	EXPECT_EQ("0o17",
	    mint::format_integer(15, mint::octal_base, mint::unknown_size, mint::unknown_precision, mint::string_special));
}

// Regression: the width was decremented (sign, prefix, digits) with an unsigned type, so a
// width smaller than the content wrapped around and the function tried to build a string
// of about SIZE_MAX characters.
TEST(format_integer, width_smaller_than_content) {
	EXPECT_EQ("123", mint::format_integer(123, mint::decimal_base, 2));
	EXPECT_EQ("-5", mint::format_integer(-5, mint::decimal_base, 1));
	EXPECT_EQ("-5", mint::format_integer(-5, mint::decimal_base, 0));
	EXPECT_EQ("0xff",
	    mint::format_integer(255, mint::hexadecimal_base, 1, mint::unknown_precision, mint::string_special));
}

// Regression: the "unknown width" marker was decremented before being tested, so it was no
// longer recognised as unknown as soon as a sign or a prefix was written.
TEST(format_integer, unknown_width_with_sign_or_prefix) {
	EXPECT_EQ("-5", mint::format_integer(-5, mint::decimal_base, mint::unknown_size));
	EXPECT_EQ("+5", mint::format_integer(5, mint::decimal_base, mint::unknown_size, mint::unknown_precision,
	                    mint::string_sign | mint::string_plus));
	EXPECT_EQ("0xff", mint::format_integer(255, mint::hexadecimal_base, mint::unknown_size, mint::unknown_precision,
	                      mint::string_special));
}

TEST(format_integer, limits_and_twos_complement) {
	EXPECT_EQ("-2147483648", mint::format_integer(std::numeric_limits<int>::min()));
	EXPECT_EQ("-128", mint::format_integer(std::numeric_limits<std::int8_t>::min()));
	EXPECT_EQ("-9223372036854775808", mint::format_integer(std::numeric_limits<std::int64_t>::min()));
	EXPECT_EQ("18446744073709551615", mint::format_integer(std::numeric_limits<std::uint64_t>::max()));
	EXPECT_EQ("ff", mint::format_integer(static_cast<std::uint8_t>(255), mint::hexadecimal_base));
	// without the sign flag, a negative number is written in two's complement (as printf does)
	EXPECT_EQ("ffffffff",
	    mint::format_integer(-1, mint::hexadecimal_base, mint::unknown_size, mint::unknown_precision, 0));
}

TEST(format_float, no_width) {
	EXPECT_EQ("3.14",
	    mint::format_float(3.14159, mint::decimal_base, mint::DigitsFormat::decimal, mint::unknown_size, 2));
	EXPECT_EQ("-1.5", mint::format_float(-1.5, mint::decimal_base, mint::DigitsFormat::decimal, mint::unknown_size, 1));
	EXPECT_EQ("3.", mint::format_float(3.0, mint::decimal_base, mint::DigitsFormat::decimal, mint::unknown_size, 0,
	                    mint::string_special));
}

TEST(format_float, width_and_alignment) {
	EXPECT_EQ("    3.14", mint::format_float(3.14159, mint::decimal_base, mint::DigitsFormat::decimal, 8, 2));
	EXPECT_EQ("3.14    ", mint::format_float(3.14159, mint::decimal_base, mint::DigitsFormat::decimal, 8, 2,
	                          mint::string_sign | mint::string_left));
	EXPECT_EQ("00003.14", mint::format_float(3.14159, mint::decimal_base, mint::DigitsFormat::decimal, 8, 2,
	                          mint::string_sign | mint::string_zeropad));
	EXPECT_EQ("-0003.14", mint::format_float(-3.14159, mint::decimal_base, mint::DigitsFormat::decimal, 8, 2,
	                          mint::string_sign | mint::string_zeropad));
	EXPECT_EQ("   +3.14", mint::format_float(3.14159, mint::decimal_base, mint::DigitsFormat::decimal, 8, 2,
	                          mint::string_sign | mint::string_plus));
	EXPECT_EQ("   -3.14", mint::format_float(-3.14159, mint::decimal_base, mint::DigitsFormat::decimal, 8, 2));
	EXPECT_EQ("-3.14   ", mint::format_float(-3.14159, mint::decimal_base, mint::DigitsFormat::decimal, 8, 2,
	                          mint::string_sign | mint::string_left));
}

// Regression: the width was decremented (sign, then content) with an unsigned type, so a width
// smaller than the content wrapped around and the function tried to build a string of about
// SIZE_MAX characters.
TEST(format_float, width_smaller_than_content) {
	EXPECT_EQ("12345.7",
	    mint::format_float(12345.678, mint::decimal_base, mint::DigitsFormat::decimal, mint::unknown_size, 1));
	EXPECT_EQ("-1.5", mint::format_float(-1.5, mint::decimal_base, mint::DigitsFormat::decimal, mint::unknown_size, 1));
	EXPECT_EQ("-1.5", mint::format_float(-1.5, mint::decimal_base, mint::DigitsFormat::decimal, mint::unknown_size, 1));
}

// Regression: the "unknown width" marker was decremented before being tested, so it was no longer
// recognised as unknown as soon as a sign was written.
TEST(format_float, unknown_width_with_sign) {
	EXPECT_EQ("-1.5", mint::format_float(-1.5, mint::decimal_base, mint::DigitsFormat::decimal, mint::unknown_size, 1));
	EXPECT_EQ("+1.5", mint::format_float(1.5, mint::decimal_base, mint::DigitsFormat::decimal, mint::unknown_size, 1,
	                      mint::string_sign | mint::string_plus));
}
