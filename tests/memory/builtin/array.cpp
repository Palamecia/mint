#include <gtest/gtest.h>
#include <memory/builtin/array.h>
#include <memory/builtin/string.h>
#include <memory/functiontool.h>
#include <memory/operatortool.h>
#include <memory/memorytool.h>
#include <ast/abstractsyntaxtree.h>
#include <ast/cursor.h>

using namespace std;
using namespace mint;

TEST(array, join) {

	AbstractSyntaxTree ast;
	unique_ptr<Cursor> cursor(ast.createCursor());
	unique_ptr<Reference> array(Reference::create<Array>());
	array_append(array->data<Array>(), create_string("a"));
	array_append(array->data<Array>(), create_string("b"));
	array_append(array->data<Array>(), create_string("c"));
	array->data<Array>()->construct();

	cursor->stack().push_back(array.get());
	cursor->stack().push_back(create_string(", "));

	ASSERT_TRUE(call_overload(cursor.get(), "join", 1));
	EXPECT_EQ(1u, cursor->stack().size());

	SharedReference result = cursor->stack().back();
	cursor->stack().pop_back();

	ASSERT_EQ(Data::fmt_object, result->data()->format);
	ASSERT_EQ(Class::string, result->data<Object>()->metadata->metatype());
	EXPECT_EQ("a, b, c", result->data<String>()->str);
}
