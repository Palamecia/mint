#include <gtest/gtest.h>
#include "mint/memory/function_tools.h"
#include "mint/memory/builtin/string.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/memory/data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/object.h"
#include "mint/scheduler/scheduler.h"

TEST(function_tools, pop_parameter) {
	/// \todo
}

TEST(function_tools, return_value) {
	/// \todo
}

TEST(function_tools, create_number) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	const auto ref = mint::create_number(7357);

	ASSERT_EQ(mint::Data::Format::number, ref.data().format());
	EXPECT_EQ(7357, ref.data<mint::Number>().value);
}

TEST(function_tools, create_boolean) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	const auto ref = mint::create_boolean(true);

	ASSERT_EQ(mint::Data::Format::boolean, ref.data().format());
	EXPECT_EQ(true, ref.data<mint::Boolean>().value);
}

TEST(function_tools, create_string) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	const auto ref = mint::create_string(scheduler.ast(), "test");

	ASSERT_EQ(mint::Data::Format::object, ref.data().format());
	ASSERT_EQ(mint::Class::Metatype::string, ref.data<mint::Object>().metadata.metatype());
	EXPECT_EQ("test", ref.data<mint::String>().str);
	EXPECT_TRUE(is_object(ref.data<mint::Object>()));
}

TEST(function_tools, create_array) {
	/// \todo
}

TEST(function_tools, create_hash) {
	/// \todo
}

TEST(function_tools, create_c_object) {
	/// \todo
}
