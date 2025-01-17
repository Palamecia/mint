#include <gtest/gtest.h>
#include <mint/memory/builtin/string.h>
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/symbol.h"
#include "mint/memory/class.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"

using namespace mint;

TEST(string, subscript) {

	Scheduler scheduler(0, nullptr);
	Process *thread = scheduler.enable_testing();
	WeakReference string = create_string("tëst");

	WeakReference result = scheduler.invoke(string, Class::SUBSCRIPT_OPERATOR, create_number(2));
	ASSERT_EQ(Data::FMT_OBJECT, result.data()->format);
	ASSERT_EQ(Class::STRING, result.data<Object>()->metadata->metatype());
	EXPECT_EQ("s", result.data<String>()->str);

	result = scheduler.invoke(string, Class::SUBSCRIPT_OPERATOR, create_iterator(create_number(1), create_number(2)));
	ASSERT_EQ(Data::FMT_OBJECT, result.data()->format);
	ASSERT_EQ(Class::STRING, result.data<Object>()->metadata->metatype());
	EXPECT_EQ("ës", result.data<String>()->str);

	scheduler.disable_testing(thread);
}

TEST(string, contains) {

	Scheduler scheduler(0, nullptr);
	Process *thread = scheduler.enable_testing();
	WeakReference string = create_string("test");
	
	WeakReference result = scheduler.invoke(string, Symbol("contains"), create_string("es"));
	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_EQ(true, result.data<Boolean>()->value);

	result = scheduler.invoke(string, Symbol("contains"), create_string("se"));
	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_EQ(false, result.data<Boolean>()->value);

	scheduler.disable_testing(thread);
}

TEST(string, starts_with) {

	Scheduler scheduler(0, nullptr);
	Process *thread = scheduler.enable_testing();
	WeakReference string = create_string("test");

	WeakReference result = scheduler.invoke(string, Symbol("startsWith"), create_string("te"));
	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_EQ(true, result.data<Boolean>()->value);

	result = scheduler.invoke(string, Symbol("startsWith"), create_string("et"));
	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_EQ(false, result.data<Boolean>()->value);

	scheduler.disable_testing(thread);
}

TEST(string, ends_with) {

	Scheduler scheduler(0, nullptr);
	Process *thread = scheduler.enable_testing();
	WeakReference string = create_string("test");

	WeakReference result = scheduler.invoke(string, Symbol("endsWith"), create_string("st"));
	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_EQ(true, result.data<Boolean>()->value);


	result = scheduler.invoke(string, Symbol("endsWith"), create_string("ts"));
	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_EQ(false, result.data<Boolean>()->value);

	result = scheduler.invoke(string, Symbol("endsWith"), create_string("test+"));
	ASSERT_EQ(Data::FMT_BOOLEAN, result.data()->format);
	EXPECT_EQ(false, result.data<Boolean>()->value);

	scheduler.disable_testing(thread);
}

TEST(string, split) {

	Scheduler scheduler(0, nullptr);
	Process *thread = scheduler.enable_testing();

	WeakReference string = create_string("a, b, c");
	WeakReference result = scheduler.invoke(string, Symbol("split"), create_string(", "));

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

	string = create_string("tëst");
	result = scheduler.invoke(string, Symbol("split"), create_string(""));

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

	scheduler.disable_testing(thread);
}
