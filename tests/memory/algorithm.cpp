#include <gtest/gtest.h>
#include <string>
#include "mint/memory/algorithm.h"
#include "mint/config.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/object.h"
#include "mint/scheduler/scheduler.h"

TEST(algorithm, visit_calls_visitor_with_referenced_data) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();

	const auto ref = mint::create_string(scheduler.program(), "hello");

	const auto result = mint::visit<std::string>(mint::Overloaded {
	                                                 [](const mint::None&) {
		                                                 return "none";
	                                                 },
	                                                 [](const mint::Null&) {
		                                                 return "null";
	                                                 },
	                                                 [](const mint::Number& number) {
		                                                 return std::to_string(number.value);
	                                                 },
	                                                 [](const mint::Boolean& boolean) {
		                                                 return boolean.value ? "true" : "false";
	                                                 },
	                                                 [](const mint::String& str) {
		                                                 return str.str;
	                                                 },
	                                                 [](const auto&) {
		                                                 return "other";
	                                                 },
	                                             },
	    ref);

	EXPECT_EQ("hello", result);
}
