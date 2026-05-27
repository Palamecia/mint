#include <gtest/gtest.h>
#include "mint/memory/builtin/string.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/symbol.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"

TEST(string, subscript) {

	mint::Scheduler scheduler({});
	auto thread = scheduler.enable_testing();
	const auto string = mint::create_string(scheduler.ast(), "tëst");

	mint::Reference result = scheduler.invoke(string, mint::Class::subscript_operator, mint::create_signed_number(2));
	ASSERT_EQ(mint::Data::Format::object, result.data().format());
	ASSERT_EQ(mint::Class::Metatype::string, result.data<mint::Object>().metadata.metatype());
	EXPECT_EQ("s", result.data<mint::String>().str);

	result = scheduler.invoke(string, mint::Class::subscript_operator,
	    create_iterator_from(thread->cursor(), mint::create_signed_number(1), mint::create_signed_number(2)));
	ASSERT_EQ(mint::Data::Format::object, result.data().format());
	ASSERT_EQ(mint::Class::Metatype::string, result.data<mint::Object>().metadata.metatype());
	EXPECT_EQ("ës", result.data<mint::String>().str);
}

TEST(string, contains) {

	mint::Scheduler scheduler({});
	auto thread = scheduler.enable_testing();
	const auto string = mint::create_string(scheduler.ast(), "test");

	mint::Reference result = scheduler.invoke(string, mint::Symbol("contains"),
	    mint::create_string(scheduler.ast(), "es"));
	ASSERT_EQ(mint::Data::Format::boolean, result.data().format());
	EXPECT_EQ(true, result.data<mint::Boolean>().value);

	result = scheduler.invoke(string, mint::Symbol("contains"), mint::create_string(scheduler.ast(), "se"));
	ASSERT_EQ(mint::Data::Format::boolean, result.data().format());
	EXPECT_EQ(false, result.data<mint::Boolean>().value);
}

TEST(string, starts_with) {

	mint::Scheduler scheduler({});
	auto thread = scheduler.enable_testing();
	const auto string = mint::create_string(scheduler.ast(), "test");

	mint::Reference result = scheduler.invoke(string, mint::Symbol("startsWith"),
	    mint::create_string(scheduler.ast(), "te"));
	ASSERT_EQ(mint::Data::Format::boolean, result.data().format());
	EXPECT_EQ(true, result.data<mint::Boolean>().value);

	result = scheduler.invoke(string, mint::Symbol("startsWith"), mint::create_string(scheduler.ast(), "et"));
	ASSERT_EQ(mint::Data::Format::boolean, result.data().format());
	EXPECT_EQ(false, result.data<mint::Boolean>().value);
}

TEST(string, ends_with) {

	mint::Scheduler scheduler({});
	auto thread = scheduler.enable_testing();
	const auto string = mint::create_string(scheduler.ast(), "test");

	mint::Reference result = scheduler.invoke(string, mint::Symbol("endsWith"),
	    mint::create_string(scheduler.ast(), "st"));
	ASSERT_EQ(mint::Data::Format::boolean, result.data().format());
	EXPECT_EQ(true, result.data<mint::Boolean>().value);

	result = scheduler.invoke(string, mint::Symbol("endsWith"), mint::create_string(scheduler.ast(), "ts"));
	ASSERT_EQ(mint::Data::Format::boolean, result.data().format());
	EXPECT_EQ(false, result.data<mint::Boolean>().value);

	result = scheduler.invoke(string, mint::Symbol("endsWith"), mint::create_string(scheduler.ast(), "test+"));
	ASSERT_EQ(mint::Data::Format::boolean, result.data().format());
	EXPECT_EQ(false, result.data<mint::Boolean>().value);
}

TEST(string, split) {

	mint::Scheduler scheduler({});
	auto thread = scheduler.enable_testing();

	mint::Reference string = mint::create_string(scheduler.ast(), "a, b, c");
	mint::Reference result = scheduler.invoke(string, mint::Symbol("split"), mint::create_string(scheduler.ast(), ", "));

	ASSERT_EQ(mint::Data::Format::object, result.data().format());
	ASSERT_EQ(mint::Class::Metatype::array, result.data<mint::Object>().metadata.metatype());
	ASSERT_EQ(3uz, result.data<mint::Array>().values.size());

	ASSERT_EQ(mint::Data::Format::object, array_get_item(result.data<mint::Array>(), 0).data().format());
	ASSERT_EQ(mint::Class::Metatype::string,
	    array_get_item(result.data<mint::Array>(), 0).data<mint::Object>().metadata.metatype());
	EXPECT_EQ("a", array_get_item(result.data<mint::Array>(), 0).data<mint::String>().str);

	ASSERT_EQ(mint::Data::Format::object, mint::array_get_item(result.data<mint::Array>(), 1).data().format());
	ASSERT_EQ(mint::Class::Metatype::string,
	    array_get_item(result.data<mint::Array>(), 1).data<mint::Object>().metadata.metatype());
	EXPECT_EQ("b", array_get_item(result.data<mint::Array>(), 1).data<mint::String>().str);

	ASSERT_EQ(mint::Data::Format::object, array_get_item(result.data<mint::Array>(), 2).data().format());
	ASSERT_EQ(mint::Class::Metatype::string,
	    array_get_item(result.data<mint::Array>(), 2).data<mint::Object>().metadata.metatype());
	EXPECT_EQ("c", array_get_item(result.data<mint::Array>(), 2).data<mint::String>().str);

	string = mint::create_string(scheduler.ast(), "tëst");
	result = scheduler.invoke(string, mint::Symbol("split"), mint::create_string(scheduler.ast(), ""));

	ASSERT_EQ(mint::Data::Format::object, result.data().format());
	ASSERT_EQ(mint::Class::Metatype::array, result.data<mint::Object>().metadata.metatype());
	ASSERT_EQ(4uz, result.data<mint::Array>().values.size());

	ASSERT_EQ(mint::Data::Format::object, array_get_item(result.data<mint::Array>(), 0).data().format());
	ASSERT_EQ(mint::Class::Metatype::string,
	    array_get_item(result.data<mint::Array>(), 0).data<mint::Object>().metadata.metatype());
	EXPECT_EQ("t", array_get_item(result.data<mint::Array>(), 0).data<mint::String>().str);

	ASSERT_EQ(mint::Data::Format::object, array_get_item(result.data<mint::Array>(), 1).data().format());
	ASSERT_EQ(mint::Class::Metatype::string,
	    array_get_item(result.data<mint::Array>(), 1).data<mint::Object>().metadata.metatype());
	EXPECT_EQ("ë", array_get_item(result.data<mint::Array>(), 1).data<mint::String>().str);

	ASSERT_EQ(mint::Data::Format::object, array_get_item(result.data<mint::Array>(), 2).data().format());
	ASSERT_EQ(mint::Class::Metatype::string,
	    array_get_item(result.data<mint::Array>(), 2).data<mint::Object>().metadata.metatype());
	EXPECT_EQ("s", array_get_item(result.data<mint::Array>(), 2).data<mint::String>().str);

	ASSERT_EQ(mint::Data::Format::object, array_get_item(result.data<mint::Array>(), 3).data().format());
	ASSERT_EQ(mint::Class::Metatype::string,
	    array_get_item(result.data<mint::Array>(), 3).data<mint::Object>().metadata.metatype());
	EXPECT_EQ("t", array_get_item(result.data<mint::Array>(), 3).data<mint::String>().str);
}

TEST(string, format) {

	mint::Scheduler scheduler({});
	auto thread = scheduler.enable_testing();

	mint::Reference result = scheduler.invoke(mint::create_string(scheduler.ast(), "{}, {:04}, {:.2f}, {{ok}}"),
	    mint::Symbol("format"), mint::create_string(scheduler.ast(), "value"), mint::create_signed_number(7),
	    mint::create_number(3.5));
	ASSERT_EQ(mint::Data::Format::object, result.data().format());
	ASSERT_EQ(mint::Class::Metatype::string, result.data<mint::Object>().metadata.metatype());
	EXPECT_EQ("value, 0007, 3.50, {ok}", result.data<mint::String>().str);

	result = scheduler.invoke(mint::create_string(scheduler.ast(), "{1} then {0}"), mint::Symbol("format"),
	    mint::create_string(scheduler.ast(), "left"), mint::create_string(scheduler.ast(), "right"));
	ASSERT_EQ(mint::Data::Format::object, result.data().format());
	ASSERT_EQ(mint::Class::Metatype::string, result.data<mint::Object>().metadata.metatype());
	EXPECT_EQ("right then left", result.data<mint::String>().str);
}
