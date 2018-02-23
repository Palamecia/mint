#include <gtest/gtest.h>
#include <memory/functiontool.h>
#include <memory/builtin/string.h>

using namespace mint;

TEST(functiontool, popParameter) {
	/// \todo
}

TEST(functiontool, returnValue) {
	/// \todo
}

TEST(functiontool, create_number) {

	SharedReference ref = create_number(7357);

	ASSERT_EQ(Data::fmt_number, ref->data()->format);
	EXPECT_EQ(7357, ref->data<Number>()->value);
	EXPECT_TRUE(ref.isUnique());
}

TEST(functiontool, create_boolean) {

	SharedReference ref = create_boolean(true);

	ASSERT_EQ(Data::fmt_boolean, ref->data()->format);
	EXPECT_EQ(true, ref->data<Boolean>()->value);
	EXPECT_TRUE(ref.isUnique());
}

TEST(functiontool, create_string) {

	SharedReference ref = create_string("test");

	ASSERT_EQ(Data::fmt_object, ref->data()->format);
	ASSERT_EQ(Class::string, ref->data<Object>()->metadata->metatype());
	EXPECT_EQ("test", ref->data<String>()->str);
	EXPECT_TRUE(is_object(ref->data<Object>()));
	EXPECT_TRUE(ref.isUnique());
}

TEST(functiontool, create_array) {
	/// \todo
}

TEST(functiontool, create_hash) {
	/// \todo
}

TEST(functiontool, create_object) {
	/// \todo
}
