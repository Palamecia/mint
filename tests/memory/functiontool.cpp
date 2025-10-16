#include <gtest/gtest.h>
#include "mint/memory/functiontool.h"
#include "mint/memory/builtin/string.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/memory/data.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/scheduler/scheduler.h"

TEST(functiontool, pop_parameter) {
	/// \todo
}

TEST(functiontool, return_value) {
	/// \todo
}

TEST(functiontool, create_number) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	const auto ref = mint::create_number(7357);

	ASSERT_EQ(mint::Data::number_format, ref.data().format());
	EXPECT_EQ(7357, ref.data<mint::Number>().value);
}

TEST(functiontool, create_boolean) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	const auto ref = mint::create_boolean(true);

	ASSERT_EQ(mint::Data::boolean_format, ref.data().format());
	EXPECT_EQ(true, ref.data<mint::Boolean>().value);
}

TEST(functiontool, create_string) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	const auto ref = mint::create_string(scheduler.ast(), "test");

	ASSERT_EQ(mint::Data::object_format, ref.data().format());
	ASSERT_EQ(mint::Class::string, ref.data<mint::Object>().metadata.metatype());
	EXPECT_EQ("test", ref.data<mint::String>().str);
	EXPECT_TRUE(is_object(ref.data<mint::Object>()));
}

TEST(functiontool, create_array) {
	/// \todo
}

TEST(functiontool, create_hash) {
	/// \todo
}

TEST(functiontool, create_c_object) {
	/// \todo
}
