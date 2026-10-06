#include <functional>
#include <gtest/gtest.h>
#include "mint/compiler/descriptions.h"
#include "mint/program/program.h"
#include "mint/memory/global_data.h"
#include "mint/memory/data.h"
#include "mint/memory/garbage_collector.h"
#include "mint/scheduler/scheduler.h"

TEST(global_data, default_package_identity_and_static_refs) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	const auto& global = scheduler.program().global_data();

	EXPECT_EQ("", global.get_full_name());
	EXPECT_EQ("(default)", global.get_name().str());
	EXPECT_EQ(mint::Data::Format::none, mint::GlobalData::none_ref().data().format());
	EXPECT_EQ(mint::Data::Format::null, mint::GlobalData::null_ref().data().format());
	EXPECT_EQ(&mint::GarbageCollector::instance().none_ref(), &mint::GlobalData::none_ref());
	EXPECT_EQ(&mint::GarbageCollector::instance().null_ref(), &mint::GlobalData::null_ref());
}

TEST(global_data, packages_are_created_once_and_locatable) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto& global = scheduler.program().global_data_description();

	const auto& child = global.get_or_create_package(scheduler.program(), "child");
	const auto& again = global.get_or_create_package(scheduler.program(), "child");

	EXPECT_EQ(&child, &again);
	EXPECT_EQ(&child, global.find_package("child"));
	EXPECT_EQ(nullptr, global.find_package("missing"));
	EXPECT_EQ("child", child.get_name().str());
	EXPECT_EQ("child", child.get_path().to_string());
	EXPECT_EQ(&child, &std::get<std::reference_wrapper<const mint::PackageDescription>>(global.locate("child")).get());
}
