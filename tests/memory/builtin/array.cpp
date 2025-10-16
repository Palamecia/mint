#include <gtest/gtest.h>
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/symbol.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/garbagecollector.h"
#include "mint/scheduler/scheduler.h"

TEST(array, join) {

	mint::Scheduler scheduler({});
	auto thread = scheduler.enable_testing();

	const auto array = mint::create_array(scheduler.ast(), {
	                                                           mint::create_string(scheduler.ast(), "a"),
	                                                           mint::create_string(scheduler.ast(), "b"),
	                                                           mint::create_string(scheduler.ast(), "c"),
	                                                       });

	const auto result = scheduler.invoke(array, mint::Symbol("join"), mint::create_string(scheduler.ast(), ", "));
	ASSERT_EQ(mint::Data::object_format, result.data().format());
	ASSERT_EQ(mint::Class::string, result.data<mint::Object>().metadata.metatype());
	EXPECT_EQ("a, b, c", result.data<mint::String>().str);
}
