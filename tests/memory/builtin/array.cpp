#include <gtest/gtest.h>
#include "mint/program/program.h"
#include "mint/program/symbol.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/garbage_collector.h"
#include "mint/scheduler/scheduler.h"

TEST(array, join) {

	auto scheduler = mint::Scheduler({});
	const auto thread = scheduler.enable_testing();

	const auto array = mint::create_array(scheduler.program(), {
	                                                               mint::create_string(scheduler.program(), "a"),
	                                                               mint::create_string(scheduler.program(), "b"),
	                                                               mint::create_string(scheduler.program(), "c"),
	                                                           });

	const auto result = scheduler.invoke(array, mint::Symbol("join"), mint::create_string(scheduler.program(), ", "));
	ASSERT_EQ(mint::Data::Format::object, result.data().format());
	ASSERT_EQ(mint::Class::Metatype::string, result.data<mint::Object>().metadata.metatype());
	EXPECT_EQ("a, b, c", result.data<mint::String>().str);
}
