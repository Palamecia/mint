#include <cstdint>
#include <gtest/gtest.h>
#include "mint/ast/symbol.h"
#include "mint/memory/class_tools.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"

TEST(class, operator_symbols_round_trip) {

	for (std::uint8_t i = 0; i < mint::Class::operator_count; ++i) {
		const auto op = static_cast<mint::Class::Operator>(i);
		const auto symbol = mint::get_operator_symbol(op);

		EXPECT_FALSE(symbol.str().empty());
		ASSERT_TRUE(mint::get_symbol_operator(symbol).has_value());
		EXPECT_EQ(op, *mint::get_symbol_operator(symbol));
	}

	EXPECT_FALSE(mint::get_symbol_operator(mint::Symbol("__not_an_operator__")).has_value());
}

TEST(class, created_class_exposes_members_slots_and_metadata) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	mint::Class& type = mint::create_class(scheduler.ast(), "__class_api_test__",
	    {
	        {mint::Symbol("slot"), mint::Reference(mint::Reference::default_flags)},
	        {mint::Symbol("constant"), mint::create_number(7357)},
	    });

	EXPECT_EQ(mint::Class::Metatype::object, type.metatype());
	EXPECT_EQ("__class_api_test__", type.full_name());
	EXPECT_EQ("__class_api_test__", type.name().str());
	EXPECT_EQ(&scheduler.ast().global_data(), &type.get_package());
	EXPECT_EQ(1, type.size());
	EXPECT_TRUE(type.is_trivially_copyable());

	auto* slot = type.find_member("slot");
	ASSERT_NE(nullptr, slot);
	EXPECT_EQ(0, slot->offset);
	EXPECT_EQ(&type, &slot->owner.get());

	auto* constant = type.find_member("constant");
	ASSERT_NE(nullptr, constant);
	EXPECT_EQ(mint::Class::MemberInfo::invalid_offset, constant->offset);
	EXPECT_EQ(mint::Data::Format::number, constant->value.data().format());
	EXPECT_EQ(7357, constant->value.data<mint::Number>().value);

	EXPECT_EQ(nullptr, type.find_member("missing"));

	type.disable_trivial_copy();
	EXPECT_FALSE(type.is_trivially_copyable());
}

TEST(class, same_class_relationships_do_not_require_bases) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	mint::Class& first = mint::create_class(scheduler.ast(), "ClassApiFirst", {});
	mint::Class& second = mint::create_class(scheduler.ast(), "ClassApiSecond", {});

	EXPECT_TRUE(first.is_same(first));
	EXPECT_FALSE(first.is_same(second));
	EXPECT_TRUE(first.bases().empty());
	EXPECT_TRUE(first.is_base_or_same(first));
	EXPECT_TRUE(first.is_direct_base_or_same(first));
	EXPECT_FALSE(first.is_base_of(second));
	EXPECT_FALSE(first.is_base_or_same(second));
	EXPECT_FALSE(first.is_direct_base_or_same(second));
}
