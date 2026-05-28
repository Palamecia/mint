#include <gtest/gtest.h>
#include "mint/ast/module.h"
#include "mint/ast/symbol.h"
#include "mint/memory/class_tools.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"

TEST(object, number_boolean_and_package_formats) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	const auto integer = mint::create_number(7357);
	const auto decimal = mint::create_number(73.57);
	const auto boolean = mint::create_boolean(true);
	const auto package = mint::make_reference<mint::Package>(mint::Reference::default_flags,
	    scheduler.ast().global_data());

	EXPECT_EQ(mint::Data::Format::number, integer.data().format());
	EXPECT_TRUE(mint::is_integer(integer));
	EXPECT_FALSE(mint::is_integer(decimal));
	EXPECT_EQ(7357, integer.data<mint::Number>().value);
	EXPECT_EQ(mint::Data::Format::boolean, boolean.data().format());
	EXPECT_TRUE(boolean.data<mint::Boolean>().value);
	EXPECT_EQ(mint::Data::Format::package, package.data().format());
	EXPECT_EQ(&scheduler.ast().global_data(), &package.data<mint::Package>().data);
}

TEST(object, object_constructs_slots_from_class_defaults) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	mint::Class& type = mint::create_class(scheduler.ast(), "__object_api_test__",
	    {
	        {mint::Symbol("member"), mint::create_number(42)},
	    });

	auto ref = mint::make_reference<mint::Object>(mint::Reference::default_flags, type);
	auto& object = ref.data<mint::Object>();
	object.construct();

	ASSERT_NE(nullptr, object.data);
	ASSERT_EQ(nullptr, type.find_global("member"));

	auto* member = type.find_member("member");
	ASSERT_NE(nullptr, member);
	const mint::Reference& value = mint::Class::MemberInfo::get(*member, object);
	EXPECT_EQ(mint::Data::Format::number, value.data().format());
	EXPECT_EQ(42, value.data<mint::Number>().value);
}

TEST(object, default_function_has_function_format_and_empty_mapping) {

	auto function = mint::make_reference<mint::Function>(mint::Reference::default_flags);

	ASSERT_EQ(mint::Data::Format::function, function.data().format());
	EXPECT_TRUE(function.data<mint::Function>().mapping.empty());
	EXPECT_EQ(function.data<mint::Function>().mapping.end(), function.data<mint::Function>().mapping.find(0));
	EXPECT_FALSE(mint::is_stateful_function(function));
}
