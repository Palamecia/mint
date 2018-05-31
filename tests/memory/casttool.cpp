#include <gtest/gtest.h>
#include <memory/casttool.h>
#include <memory/memorytool.h>
#include <memory/functiontool.h>
#include <memory/builtin/string.h>
#include <memory/builtin/array.h>
#include <memory/builtin/hash.h>
#include <memory/builtin/iterator.h>

using namespace mint;

TEST(casttool, to_number) {

	EXPECT_EQ(7357, to_number(nullptr, *create_number(7357)));

	EXPECT_EQ(1, to_number(nullptr, *create_boolean(true)));
	EXPECT_EQ(0, to_number(nullptr, *create_boolean(false)));

	EXPECT_EQ(7357, to_number(nullptr, *create_string("7357")));
	EXPECT_EQ(0x7E57, to_number(nullptr, *create_string("0x7E57")));
	EXPECT_EQ(07357, to_number(nullptr, *create_string("0o7357")));
	EXPECT_EQ(0b1010, to_number(nullptr, *create_string("0b1010")));
	EXPECT_EQ(0, to_number(nullptr, *create_string("test")));

	SharedReference it = SharedReference::unique(Reference::create<Iterator>());
	iterator_insert(it->data<Iterator>(), create_number(7357));
	iterator_insert(it->data<Iterator>(), create_number(7356));
	EXPECT_EQ(7357, to_number(nullptr, *it));
	EXPECT_EQ(7356, to_number(nullptr, *it));
}

TEST(casttool, to_boolean) {

	EXPECT_EQ(true, to_boolean(nullptr, *create_number(7357)));
	EXPECT_EQ(false, to_boolean(nullptr, *create_number(0)));

	EXPECT_EQ(true, to_boolean(nullptr, *create_boolean(true)));
	EXPECT_EQ(false, to_boolean(nullptr, *create_boolean(false)));


	SharedReference it;
	it = SharedReference::unique(Reference::create<Iterator>());
	iterator_insert(it->data<Iterator>(), SharedReference::unique(Reference::create<None>()));
	EXPECT_EQ(true, to_boolean(nullptr, *it));
	it = SharedReference::unique(Reference::create<Iterator>());
	EXPECT_EQ(false, to_boolean(nullptr, *it));
}

TEST(casttool, to_char) {

	EXPECT_EQ("", to_char(*SharedReference::unique(Reference::create<None>())));
	EXPECT_EQ("", to_char(*SharedReference::unique(Reference::create<Null>())));

	EXPECT_EQ("\x37", to_char(*create_number(0x37)));

	EXPECT_EQ("n", to_char(*create_boolean(false)));
	EXPECT_EQ("y", to_char(*create_boolean(true)));

	EXPECT_EQ("t", to_char(*create_string("test")));
}

TEST(casttool, to_string) {

	EXPECT_EQ("(none)", to_string(*SharedReference::unique(Reference::create<None>())));
	EXPECT_EQ("(null)", to_string(*SharedReference::unique(Reference::create<Null>())));
	EXPECT_EQ("(function)", to_string(*SharedReference::unique(Reference::create<Function>())));

	EXPECT_EQ("7357", to_string(*create_number(7357)));
	EXPECT_EQ("73.570000", to_string(*create_number(73.57)));

	EXPECT_EQ("false", to_string(*create_boolean(false)));
	EXPECT_EQ("true", to_string(*create_boolean(true)));

	EXPECT_EQ("test", to_string(*create_string("test")));

	EXPECT_EQ("[test1, test2]", to_string(*create_array({create_string("test1"), create_string("test2")})));

	EXPECT_EQ("{key1 : value1}", to_string(*create_hash({{create_string("key1"), create_string("value1")}})));

	SharedReference it = SharedReference::unique(Reference::create<Iterator>());
	iterator_insert(it->data<Iterator>(), create_string("test1"));
	iterator_insert(it->data<Iterator>(), create_string("test2"));
	EXPECT_EQ("test1", to_string(*it));
	EXPECT_EQ("test2", to_string(*it));
}

TEST(casttool, to_regex) {
	/// \todo
}

TEST(casttool, to_array) {
	/// \todo
}

TEST(casttool, to_hash) {
	/// \todo
}
