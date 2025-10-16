#include "mint/ast/classregister.h"
#include "mint/ast/module.h"
#include "mint/ast/symbol.h"
#include "mint/compiler/buildtool.h"
#include "mint/compiler/compiler.h"
#include "mint/memory/data.h"
#include "mint/memory/garbagecollector.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/bufferstream.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(buildtool, resolve_class_description) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	mint::BufferStream stream("");
	mint::Compiler compiler(scheduler.ast());
	mint::BuildContext context(stream, compiler, scheduler.ast().create_module(mint::Module::State::ready));

	context.start_class_description("A");
	context.create_member(mint::Reference::default_flags, mint::Symbol("mbr"),
	    mint::GarbageCollector::instance().alloc<mint::None>());
	context.resolve_class_description();

	mint::ClassDescription* a_desc = scheduler.ast().global_data().find_class_description("A");
	ASSERT_NE(nullptr, a_desc);
	EXPECT_NO_THROW(a_desc->generate());

	context.start_class_description("B");
	context.create_member(mint::Reference::default_flags, mint::Symbol("mbr"),
	    mint::GarbageCollector::instance().alloc<mint::None>());
	context.resolve_class_description();

	mint::ClassDescription* b_desc = scheduler.ast().global_data().find_class_description("B");
	ASSERT_NE(nullptr, b_desc);
	EXPECT_NO_THROW(b_desc->generate());

	context.start_class_description("C");
	context.append_symbol_to_base_class_path("A");
	context.save_base_class_path();
	context.append_symbol_to_base_class_path("B");
	context.save_base_class_path();
	context.create_member(mint::Reference::default_flags, mint::Symbol("mbr"),
	    mint::GarbageCollector::instance().alloc<mint::None>());
	context.resolve_class_description();

	mint::ClassDescription* c_desc = scheduler.ast().global_data().find_class_description("C");
	ASSERT_NE(nullptr, c_desc);
	EXPECT_NO_THROW(c_desc->generate());

	context.start_class_description("D");
	context.append_symbol_to_base_class_path("A");
	context.save_base_class_path();
	context.append_symbol_to_base_class_path("B");
	context.save_base_class_path();
	context.resolve_class_description();

	mint::ClassDescription* d_desc = scheduler.ast().global_data().find_class_description("D");
	ASSERT_NE(nullptr, d_desc);
	ASSERT_DEATH(d_desc->generate(), "member 'mbr' is ambiguous for class 'D'");
}
