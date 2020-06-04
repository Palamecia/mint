#include <gtest/gtest.h>
#include <memory/operatortool.h>
#include <memory/functiontool.h>
#include <memory/builtin/string.h>
#include <ast/abstractsyntaxtree.h>
#include <ast/cursor.h>

using namespace std;
using namespace mint;

TEST(operatortool, call_overload) {

	GarbageCollector::instance();
	unique_ptr<Cursor> cursor(AbstractSyntaxTree::instance().createCursor());

	cursor->stack().emplace_back(create_string("foo"));
	cursor->stack().emplace_back(create_string("bar"));
	EXPECT_TRUE(call_overload(cursor.get(), "+", 1));
	EXPECT_EQ(1u, cursor->stack().size());
	EXPECT_EQ(Data::fmt_object, cursor->stack().back()->data()->format);
	EXPECT_EQ(Class::string, cursor->stack().back()->data<Object>()->metadata->metatype());
	EXPECT_STREQ("foobar", cursor->stack().back()->data<String>()->str.c_str());
	cursor->stack().clear();

	cursor->stack().emplace_back(create_string("foo"));
	cursor->stack().emplace_back(create_string("bar"));
	EXPECT_FALSE(call_overload(cursor.get(), "#", 1));
	cursor->stack().clear();

	cursor->stack().emplace_back(SharedReference::unique(StrongReference::create<String>()));
	cursor->stack().emplace_back(create_string("bar"));
	ASSERT_DEATH(call_overload(cursor.get(), "+", 1), "invalid use of class in an operation");
	cursor->stack().clear();
}
