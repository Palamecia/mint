#include <gtest/gtest.h>
#include <memory/casttool.h>
#include <memory/memorytool.h>
#include <memory/builtin/string.h>
#include <memory/builtin/array.h>
#include <memory/builtin/hash.h>
#include <memory/builtin/iterator.h>

using namespace mint;

TEST(casttool, to_number) {

	Reference *ref = nullptr;

	ref = Reference::create<Number>();
	ref->data<Number>()->value = 7357;
	EXPECT_EQ(7357, to_number(nullptr, *ref));
	delete ref;

	ref = Reference::create<Boolean>();
	ref->data<Boolean>()->value = true;
	EXPECT_EQ(1, to_number(nullptr, *ref));
	delete ref;

	ref = Reference::create<String>();
	ref->data<String>()->str = "7357";
	EXPECT_EQ(7357, to_number(nullptr, *ref));
	delete ref;

	ref = Reference::create<String>();
	ref->data<String>()->str = "0x7E57";
	EXPECT_EQ(0x7E57, to_number(nullptr, *ref));
	delete ref;

	ref = Reference::create<String>();
	ref->data<String>()->str = "0o7357";
	EXPECT_EQ(07357, to_number(nullptr, *ref));
	delete ref;

	ref = Reference::create<String>();
	ref->data<String>()->str = "0b1010";
	EXPECT_EQ(0b1010, to_number(nullptr, *ref));
	delete ref;

	ref = Reference::create<String>();
	ref->data<String>()->str = "test";
	EXPECT_EQ(0, to_number(nullptr, *ref));
	delete ref;

	ref = Reference::create<Iterator>();
	{
		Reference *num = nullptr;
		num = Reference::create<Number>();
		num->data<Number>()->value = 7357;
		iterator_insert(ref->data<Iterator>(), SharedReference::unique(num));
		num = Reference::create<Number>();
		num->data<Number>()->value = 7356;
		iterator_insert(ref->data<Iterator>(), SharedReference::unique(num));
	}
	EXPECT_EQ(7357, to_number(nullptr, *ref));
	EXPECT_EQ(7356, to_number(nullptr, *ref));
	delete ref;
}

TEST(casttool, to_boolean) {

	Reference *ref = nullptr;

	ref = Reference::create<Number>();
	ref->data<Number>()->value = 7357;
	EXPECT_EQ(true, to_boolean(nullptr, *ref));
	delete ref;

	ref = Reference::create<Number>();
	ref->data<Number>()->value = 0;
	EXPECT_EQ(false, to_boolean(nullptr, *ref));
	delete ref;

	ref = Reference::create<Boolean>();
	ref->data<Boolean>()->value = true;
	EXPECT_EQ(true, to_boolean(nullptr, *ref));
	delete ref;

	ref = Reference::create<Boolean>();
	ref->data<Boolean>()->value = false;
	EXPECT_EQ(false, to_boolean(nullptr, *ref));
	delete ref;

	ref = Reference::create<Iterator>();
	iterator_insert(ref->data<Iterator>(), SharedReference::unique(Reference::create<None>()));
	EXPECT_EQ(true, to_boolean(nullptr, *ref));
	delete ref;

	ref = Reference::create<Iterator>();
	EXPECT_EQ(false, to_boolean(nullptr, *ref));
	delete ref;
}

TEST(casttool, to_char) {

	Reference *ref = nullptr;

	ref = Reference::create<None>();
	EXPECT_EQ("", to_char(*ref));
	delete ref;

	ref = Reference::create<Null>();
	EXPECT_EQ("", to_char(*ref));
	delete ref;

	ref = Reference::create<Number>();
	ref->data<Number>()->value = 0x37;
	EXPECT_EQ("\x37", to_char(*ref));
	delete ref;

	ref = Reference::create<Boolean>();
	ref->data<Boolean>()->value = false;
	EXPECT_EQ("n", to_char(*ref));
	delete ref;

	ref = Reference::create<Boolean>();
	ref->data<Boolean>()->value = true;
	EXPECT_EQ("y", to_char(*ref));
	delete ref;

	ref = Reference::create<String>();
	ref->data<String>()->str = "test";
	EXPECT_EQ("t", to_char(*ref));
	delete ref;
}

TEST(casttool, to_string) {

	Reference *ref = nullptr;

	ref = Reference::create<None>();
	EXPECT_EQ("(none)", to_string(*ref));
	delete ref;

	ref = Reference::create<Null>();
	EXPECT_EQ("(null)", to_string(*ref));
	delete ref;

	ref = Reference::create<Number>();
	ref->data<Number>()->value = 7357;
	EXPECT_EQ("7357.000000", to_string(*ref));
	delete ref;

	ref = Reference::create<Boolean>();
	ref->data<Boolean>()->value = false;
	EXPECT_EQ("false", to_string(*ref));
	delete ref;

	ref = Reference::create<Boolean>();
	ref->data<Boolean>()->value = true;
	EXPECT_EQ("true", to_string(*ref));
	delete ref;

	ref = Reference::create<String>();
	ref->data<String>()->str = "test";
	EXPECT_EQ("test", to_string(*ref));
	delete ref;

	ref = Reference::create<Array>();
	{
		Reference *str = nullptr;
		str = Reference::create<String>();
		str->data<String>()->str = "test1";
		array_append(ref->data<Array>(), SharedReference::unique(str));
		str = Reference::create<String>();
		str->data<String>()->str = "test2";
		array_append(ref->data<Array>(), SharedReference::unique(str));
	}
	EXPECT_EQ("[test1, test2]", to_string(*ref));
	delete ref;

	ref = Reference::create<Hash>();
	{
		Reference *key = Reference::create<String>();
		Reference *value = Reference::create<String>();
		key->data<String>()->str = "key";
		value->data<String>()->str = "value";
		hash_insert(ref->data<Hash>(), SharedReference::unique(key), SharedReference::unique(value));
	}
	EXPECT_EQ("{key : value}", to_string(*ref));
	delete ref;

	ref = Reference::create<Iterator>();
	{
		Reference *str = nullptr;
		str = Reference::create<String>();
		str->data<String>()->str = "test1";
		iterator_insert(ref->data<Iterator>(), SharedReference::unique(str));
		str = Reference::create<String>();
		str->data<String>()->str = "test2";
		iterator_insert(ref->data<Iterator>(), SharedReference::unique(str));
	}
	EXPECT_EQ("test1", to_string(*ref));
	EXPECT_EQ("test2", to_string(*ref));
	delete ref;

	ref = Reference::create<Function>();
	EXPECT_EQ("(function)", to_string(*ref));
	delete ref;
}

TEST(casttool, to_array) {
	/// \todo
}

TEST(casttool, to_hash) {
	/// \todo
}
