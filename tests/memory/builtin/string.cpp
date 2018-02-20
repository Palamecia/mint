#include <gtest/gtest.h>
#include <memory/builtin/string.h>
#include <memory/operatortool.h>
#include <memory/functiontool.h>
#include <ast/abstractsyntaxtree.h>
#include <ast/cursor.h>

#include <memory>

using namespace std;
using namespace mint;

TEST(string, contains) {

	AbstractSyntaxTree ast;
	unique_ptr<Cursor> cursor(ast.createCursor());

	cursor->stack().push_back(create_string("test"));
	cursor->stack().push_back(create_string("es"));

	ASSERT_TRUE(call_overload(cursor.get(), "contains", 1));
	EXPECT_EQ(1u, cursor->stack().size());

	SharedReference result = cursor->stack().back();
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result->data()->format);
	EXPECT_TRUE(result->data<Boolean>()->value);

	cursor->stack().push_back(create_string("test"));
	cursor->stack().push_back(create_string("se"));

	ASSERT_TRUE(call_overload(cursor.get(), "contains", 1));
	EXPECT_EQ(1u, cursor->stack().size());

	result = cursor->stack().back();
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result->data()->format);
	EXPECT_FALSE(result->data<Boolean>()->value);
}

TEST(string, startsWith) {

	AbstractSyntaxTree ast;
	unique_ptr<Cursor> cursor(ast.createCursor());

	cursor->stack().push_back(create_string("test"));
	cursor->stack().push_back(create_string("te"));

	ASSERT_TRUE(call_overload(cursor.get(), "startsWith", 1));
	EXPECT_EQ(1u, cursor->stack().size());

	SharedReference result = cursor->stack().back();
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result->data()->format);
	EXPECT_TRUE(result->data<Boolean>()->value);

	cursor->stack().push_back(create_string("test"));
	cursor->stack().push_back(create_string("et"));

	ASSERT_TRUE(call_overload(cursor.get(), "startsWith", 1));
	EXPECT_EQ(1u, cursor->stack().size());

	result = cursor->stack().back();
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result->data()->format);
	EXPECT_FALSE(result->data<Boolean>()->value);
}

TEST(string, endsWith) {

	AbstractSyntaxTree ast;
	unique_ptr<Cursor> cursor(ast.createCursor());

	cursor->stack().push_back(create_string("test"));
	cursor->stack().push_back(create_string("st"));

	ASSERT_TRUE(call_overload(cursor.get(), "endsWith", 1));
	EXPECT_EQ(1u, cursor->stack().size());

	SharedReference result = cursor->stack().back();
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result->data()->format);
	EXPECT_TRUE(result->data<Boolean>()->value);

	cursor->stack().push_back(create_string("test"));
	cursor->stack().push_back(create_string("ts"));

	ASSERT_TRUE(call_overload(cursor.get(), "endsWith", 1));
	EXPECT_EQ(1u, cursor->stack().size());

	result = cursor->stack().back();
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result->data()->format);
	EXPECT_FALSE(result->data<Boolean>()->value);

	cursor->stack().push_back(create_string("test"));
	cursor->stack().push_back(create_string("test+"));

	ASSERT_TRUE(call_overload(cursor.get(), "endsWith", 1));
	EXPECT_EQ(1u, cursor->stack().size());

	result = cursor->stack().back();
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result->data()->format);
	EXPECT_FALSE(result->data<Boolean>()->value);
}

TEST(string, split) {

	AbstractSyntaxTree ast;
	unique_ptr<Cursor> cursor(ast.createCursor());

	cursor->stack().push_back(create_string("a, b, c"));
	cursor->stack().push_back(create_string(", "));

	ASSERT_TRUE(call_overload(cursor.get(), "split", 1));
	EXPECT_EQ(1u, cursor->stack().size());

	SharedReference result = cursor->stack().back();
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_object, result->data()->format);
	ASSERT_EQ(Class::array, result->data<Object>()->metadata->metatype());
	ASSERT_EQ(3u, result->data<Array>()->values.size());

	ASSERT_EQ(Data::fmt_object, array_get_item(result->data<Array>(), 0)->data()->format);
	ASSERT_EQ(Class::array, array_get_item(result->data<Array>(), 0)->data<Object>()->metadata->metatype());
	EXPECT_EQ("a", array_get_item(result->data<Array>(), 0)->data<String>()->str);

	ASSERT_EQ(Data::fmt_object, array_get_item(result->data<Array>(), 1)->data()->format);
	ASSERT_EQ(Class::array, array_get_item(result->data<Array>(), 1)->data<Object>()->metadata->metatype());
	EXPECT_EQ("b", array_get_item(result->data<Array>(), 1)->data<String>()->str);

	ASSERT_EQ(Data::fmt_object, array_get_item(result->data<Array>(), 2)->data()->format);
	ASSERT_EQ(Class::array, array_get_item(result->data<Array>(), 2)->data<Object>()->metadata->metatype());
	EXPECT_EQ("c", array_get_item(result->data<Array>(), 2)->data<String>()->str);
}
