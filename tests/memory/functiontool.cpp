#include <gtest/gtest.h>
#include <mint/memory/functiontool.h>
#include <mint/memory/builtin/string.h>
#include <mint/ast/abstractsyntaxtree.h>

using namespace mint;

TEST(functiontool, popParameter) {
	/// \todo
}

TEST(functiontool, returnValue) {
	/// \todo
}

TEST(functiontool, create_number) {

	WeakReference ref = create_number(7357);

	ASSERT_EQ(Data::fmt_number, ref.data()->format);
	EXPECT_EQ(7357, ref.data<Number>()->value);
}

TEST(functiontool, create_boolean) {

	WeakReference ref = create_boolean(true);

	ASSERT_EQ(Data::fmt_boolean, ref.data()->format);
	EXPECT_EQ(true, ref.data<Boolean>()->value);
}

TEST(functiontool, create_string) {

	AbstractSyntaxTree ast;
	WeakReference ref = create_string("test");

	ASSERT_EQ(Data::fmt_object, ref.data()->format);
	ASSERT_EQ(Class::string, ref.data<Object>()->metadata->metatype());
	EXPECT_EQ("test", ref.data<String>()->str);
	EXPECT_TRUE(is_object(ref.data<Object>()));
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
