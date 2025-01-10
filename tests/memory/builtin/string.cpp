#include <gtest/gtest.h>
#include <mint/memory/builtin/string.h>
#include <mint/memory/operatortool.h>
#include <mint/memory/functiontool.h>
#include <mint/scheduler/processor.h>
#include <mint/ast/abstractsyntaxtree.h>
#include <mint/ast/cursor.h>

#include <memory>

using namespace mint;

#define wait_for_result(cursor) while (1u < cursor->stack().size()) { ASSERT_TRUE(run_step(cursor)); }

TEST(string, subscript) {

	AbstractSyntaxTree ast;
	Cursor *cursor = ast.create_cursor();

	cursor->stack().emplace_back(create_string("tëst"));
	cursor->stack().emplace_back(create_number(2));

	ASSERT_TRUE(call_overload(cursor, "[]", 1));
	wait_for_result(cursor);

	WeakReference result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_OBJECT, result.data()->format);
	ASSERT_EQ(Class::STRING, result.data<Object>()->metadata->metatype());
	EXPECT_EQ("s", result.data<String>()->str);

	cursor->stack().emplace_back(create_string("tëst"));
	WeakReference it = WeakReference::create<Iterator>();
	iterator_insert(it.data<Iterator>(), create_number(1));
	iterator_insert(it.data<Iterator>(), create_number(2));
	cursor->stack().emplace_back(std::forward<Reference>(it));

	ASSERT_TRUE(call_overload(cursor, "[]", 1));
	wait_for_result(cursor);

	result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_OBJECT, result.data()->format);
	ASSERT_EQ(Class::STRING, result.data<Object>()->metadata->metatype());
	EXPECT_EQ("ës", result.data<String>()->str);

	delete cursor;
}

TEST(string, contains) {

	AbstractSyntaxTree ast;
	Cursor *cursor = ast.create_cursor();

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("es"));

	ASSERT_TRUE(call_overload(cursor, "contains", 1));
	wait_for_result(cursor);

	WeakReference result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_TRUE(result.data<Boolean>()->value);

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("se"));

	ASSERT_TRUE(call_overload(cursor, "contains", 1));
	wait_for_result(cursor);

	result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_FALSE(result.data<Boolean>()->value);

	delete cursor;
}

TEST(string, startsWith) {

	AbstractSyntaxTree ast;
	Cursor *cursor = ast.create_cursor();

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("te"));

	ASSERT_TRUE(call_overload(cursor, "startsWith", 1));
	wait_for_result(cursor);

	WeakReference result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_TRUE(result.data<Boolean>()->value);

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("et"));

	ASSERT_TRUE(call_overload(cursor, "startsWith", 1));
	wait_for_result(cursor);

	result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_FALSE(result.data<Boolean>()->value);

	delete cursor;
}

TEST(string, endsWith) {

	AbstractSyntaxTree ast;
	Cursor *cursor = ast.create_cursor();

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("st"));

	ASSERT_TRUE(call_overload(cursor, "endsWith", 1));
	wait_for_result(cursor);

	WeakReference result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_TRUE(result.data<Boolean>()->value);

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("ts"));

	ASSERT_TRUE(call_overload(cursor, "endsWith", 1));
	wait_for_result(cursor);

	result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_FALSE(result.data<Boolean>()->value);

	cursor->stack().emplace_back(create_string("test"));
	cursor->stack().emplace_back(create_string("test+"));

	ASSERT_TRUE(call_overload(cursor, "endsWith", 1));
	wait_for_result(cursor);

	result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_FALSE(result.data<Boolean>()->value);

	delete cursor;
}

TEST(string, split) {

	AbstractSyntaxTree ast;
	Cursor *cursor = ast.create_cursor();

	cursor->stack().emplace_back(create_string("a, b, c"));
	cursor->stack().emplace_back(create_string(", "));

	ASSERT_TRUE(call_overload(cursor, "split", 1));
	wait_for_result(cursor);

	WeakReference result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_OBJECT, result.data()->format);
	ASSERT_EQ(Class::ARRAY, result.data<Object>()->metadata->metatype());
	ASSERT_EQ(3u, result.data<Array>()->values.size());

	ASSERT_EQ(Data::FMT_OBJECT, array_get_item(result.data<Array>(), 0).data()->format);
	ASSERT_EQ(Class::STRING, array_get_item(result.data<Array>(), 0).data<Object>()->metadata->metatype());
	EXPECT_EQ("a", array_get_item(result.data<Array>(), 0).data<String>()->str);

	ASSERT_EQ(Data::FMT_OBJECT, array_get_item(result.data<Array>(), 1).data()->format);
	ASSERT_EQ(Class::STRING, array_get_item(result.data<Array>(), 1).data<Object>()->metadata->metatype());
	EXPECT_EQ("b", array_get_item(result.data<Array>(), 1).data<String>()->str);

	ASSERT_EQ(Data::FMT_OBJECT, array_get_item(result.data<Array>(), 2).data()->format);
	ASSERT_EQ(Class::STRING, array_get_item(result.data<Array>(), 2).data<Object>()->metadata->metatype());
	EXPECT_EQ("c", array_get_item(result.data<Array>(), 2).data<String>()->str);

	cursor->stack().emplace_back(create_string("tëst"));
	cursor->stack().emplace_back(create_string(""));

	ASSERT_TRUE(call_overload(cursor, "split", 1));
	wait_for_result(cursor);

	result = std::move(cursor->stack().back());
	cursor->stack().pop_back();

	ASSERT_EQ(Data::FMT_OBJECT, result.data()->format);
	ASSERT_EQ(Class::ARRAY, result.data<Object>()->metadata->metatype());
	ASSERT_EQ(4u, result.data<Array>()->values.size());

	ASSERT_EQ(Data::FMT_OBJECT, array_get_item(result.data<Array>(), 0).data()->format);
	ASSERT_EQ(Class::STRING, array_get_item(result.data<Array>(), 0).data<Object>()->metadata->metatype());
	EXPECT_EQ("t", array_get_item(result.data<Array>(), 0).data<String>()->str);

	ASSERT_EQ(Data::FMT_OBJECT, array_get_item(result.data<Array>(), 1).data()->format);
	ASSERT_EQ(Class::STRING, array_get_item(result.data<Array>(), 1).data<Object>()->metadata->metatype());
	EXPECT_EQ("ë", array_get_item(result.data<Array>(), 1).data<String>()->str);

	ASSERT_EQ(Data::FMT_OBJECT, array_get_item(result.data<Array>(), 2).data()->format);
	ASSERT_EQ(Class::STRING, array_get_item(result.data<Array>(), 2).data<Object>()->metadata->metatype());
	EXPECT_EQ("s", array_get_item(result.data<Array>(), 2).data<String>()->str);

	ASSERT_EQ(Data::FMT_OBJECT, array_get_item(result.data<Array>(), 3).data()->format);
	ASSERT_EQ(Class::STRING, array_get_item(result.data<Array>(), 3).data<Object>()->metadata->metatype());
	EXPECT_EQ("t", array_get_item(result.data<Array>(), 3).data<String>()->str);

	delete cursor;
}
