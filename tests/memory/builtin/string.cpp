#include <gtest/gtest.h>
#include <memory/builtin/string.h>
#include <memory/operatortool.h>
#include <memory/functiontool.h>
#include <scheduler/processor.h>
#include <ast/abstractsyntaxtree.h>
#include <ast/cursor.h>

#include <memory>

using namespace std;
using namespace mint;

#define wait_for_result(cursor) while (1u < cursor->stack().size()) { ASSERT_TRUE(run_step(cursor)); }

TEST(string, subscript) {

	Cursor *cursor = AbstractSyntaxTree::instance().createCursor();

	cursor->stack().emplace_back(create_string("tëst"));
	cursor->stack().emplace_back(create_number(2));

	ASSERT_TRUE(call_overload(cursor, "[]", 1));
	wait_for_result(cursor);

	WeakReference result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_object, result.data()->format);
	ASSERT_EQ(Class::string, result.data<Object>()->metadata->metatype());
	EXPECT_EQ("s", result.data<String>()->str);

	cursor->stack().emplace_back(create_string("tëst"));
	WeakReference it = WeakReference::create<Iterator>();
	iterator_insert(it.data<Iterator>(), create_number(1));
	iterator_insert(it.data<Iterator>(), create_number(2));
	cursor->stack().emplace_back(forward<Reference>(it));

	ASSERT_TRUE(call_overload(cursor, "[]", 1));
	wait_for_result(cursor);

	result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_object, result.data()->format);
	ASSERT_EQ(Class::string, result.data<Object>()->metadata->metatype());
	EXPECT_EQ("ës", result.data<String>()->str);

	delete cursor;
}

TEST(string, contains) {

	Cursor *cursor = AbstractSyntaxTree::instance().createCursor();

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("es"));

	ASSERT_TRUE(call_overload(cursor, "contains", 1));
	wait_for_result(cursor);

	WeakReference result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result.data()->format);
	EXPECT_TRUE(result.data<Boolean>()->value);

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("se"));

	ASSERT_TRUE(call_overload(cursor, "contains", 1));
	wait_for_result(cursor);

	result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result.data()->format);
	EXPECT_FALSE(result.data<Boolean>()->value);

	delete cursor;
}

TEST(string, startsWith) {

	Cursor *cursor = AbstractSyntaxTree::instance().createCursor();

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("te"));

	ASSERT_TRUE(call_overload(cursor, "startsWith", 1));
	wait_for_result(cursor);

	WeakReference result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result.data()->format);
	EXPECT_TRUE(result.data<Boolean>()->value);

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("et"));

	ASSERT_TRUE(call_overload(cursor, "startsWith", 1));
	wait_for_result(cursor);

	result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result.data()->format);
	EXPECT_FALSE(result.data<Boolean>()->value);

	delete cursor;
}

TEST(string, endsWith) {

	Cursor *cursor = AbstractSyntaxTree::instance().createCursor();

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("st"));

	ASSERT_TRUE(call_overload(cursor, "endsWith", 1));
	wait_for_result(cursor);

	WeakReference result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result.data()->format);
	EXPECT_TRUE(result.data<Boolean>()->value);

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("ts"));

	ASSERT_TRUE(call_overload(cursor, "endsWith", 1));
	wait_for_result(cursor);

	result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result.data()->format);
	EXPECT_FALSE(result.data<Boolean>()->value);

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("test+"));

	ASSERT_TRUE(call_overload(cursor, "endsWith", 1));
	wait_for_result(cursor);

	result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_boolean, result.data()->format);
	EXPECT_FALSE(result.data<Boolean>()->value);

	delete cursor;
}

TEST(string, split) {

	Cursor *cursor = AbstractSyntaxTree::instance().createCursor();

	cursor->stack().emplace_back(create_string("a, b, c"));
	cursor->stack().emplace_back(create_string(", "));

	ASSERT_TRUE(call_overload(cursor, "split", 1));
	wait_for_result(cursor);

	WeakReference result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_object, result.data()->format);
	ASSERT_EQ(Class::array, result.data<Object>()->metadata->metatype());
	ASSERT_EQ(3u, result.data<Array>()->values.size());

	ASSERT_EQ(Data::fmt_object, array_get_item(result.data<Array>(), 0).data()->format);
	ASSERT_EQ(Class::string, array_get_item(result.data<Array>(), 0).data<Object>()->metadata->metatype());
	EXPECT_EQ("a", array_get_item(result.data<Array>(), 0).data<String>()->str);

	ASSERT_EQ(Data::fmt_object, array_get_item(result.data<Array>(), 1).data()->format);
	ASSERT_EQ(Class::string, array_get_item(result.data<Array>(), 1).data<Object>()->metadata->metatype());
	EXPECT_EQ("b", array_get_item(result.data<Array>(), 1).data<String>()->str);

	ASSERT_EQ(Data::fmt_object, array_get_item(result.data<Array>(), 2).data()->format);
	ASSERT_EQ(Class::string, array_get_item(result.data<Array>(), 2).data<Object>()->metadata->metatype());
	EXPECT_EQ("c", array_get_item(result.data<Array>(), 2).data<String>()->str);

	cursor->stack().emplace_back(create_string("tëst"));
	cursor->stack().emplace_back(create_string(""));

	ASSERT_TRUE(call_overload(cursor, "split", 1));
	wait_for_result(cursor);

	result = move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_object, result.data()->format);
	ASSERT_EQ(Class::array, result.data<Object>()->metadata->metatype());
	ASSERT_EQ(4u, result.data<Array>()->values.size());

	ASSERT_EQ(Data::fmt_object, array_get_item(result.data<Array>(), 0).data()->format);
	ASSERT_EQ(Class::string, array_get_item(result.data<Array>(), 0).data<Object>()->metadata->metatype());
	EXPECT_EQ("t", array_get_item(result.data<Array>(), 0).data<String>()->str);

	ASSERT_EQ(Data::fmt_object, array_get_item(result.data<Array>(), 1).data()->format);
	ASSERT_EQ(Class::string, array_get_item(result.data<Array>(), 1).data<Object>()->metadata->metatype());
	EXPECT_EQ("ë", array_get_item(result.data<Array>(), 1).data<String>()->str);

	ASSERT_EQ(Data::fmt_object, array_get_item(result.data<Array>(), 2).data()->format);
	ASSERT_EQ(Class::string, array_get_item(result.data<Array>(), 2).data<Object>()->metadata->metatype());
	EXPECT_EQ("s", array_get_item(result.data<Array>(), 2).data<String>()->str);

	ASSERT_EQ(Data::fmt_object, array_get_item(result.data<Array>(), 3).data()->format);
	ASSERT_EQ(Class::string, array_get_item(result.data<Array>(), 3).data<Object>()->metadata->metatype());
	EXPECT_EQ("t", array_get_item(result.data<Array>(), 3).data<String>()->str);

	delete cursor;
}
