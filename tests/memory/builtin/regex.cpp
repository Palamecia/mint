#include <gtest/gtest.h>
#include "mint/memory/builtin/regex.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"

TEST(regex, default_regex_has_regex_format_and_empty_pattern) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	auto regex = mint::make_reference<mint::Regex>(mint::Reference::default_flags, scheduler.ast());

	ASSERT_EQ(mint::Data::Format::object, regex.data().format());
	EXPECT_EQ(mint::Class::Metatype::regex, regex.data<mint::Object>().metadata.metatype());
	EXPECT_TRUE(regex.data<mint::Regex>().pattern.empty());
}
