#include <gtest/gtest.h>
#include "mint/memory/class_tools.h"
#include "mint/ast/symbol.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/object.h"
#include "mint/scheduler/scheduler.h"

TEST(class_tools, create_class_with_builtin_member) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	mint::Class& type = mint::create_class(scheduler.ast(), "__class_api_test__",
	    {
	        {mint::Symbol("member"), mint::create_number(42)},
	    });

	auto* member = type.find_member("member");
	ASSERT_NE(nullptr, member);
	EXPECT_EQ(mint::Data::Format::number, member->value.data().format());
	EXPECT_EQ(42, member->value.data<mint::Number>().value);
}
