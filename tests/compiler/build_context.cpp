#include "mint/compiler/class_builder.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/program/module.h"
#include "mint/program/symbol.h"
#include "mint/compiler/build_context.h"
#include "mint/compiler/compiler.h"
#include "mint/memory/data.h"
#include "mint/memory/garbage_collector.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/buffer_stream.h"
#include "mint/system/mint_runtime_error.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

#define EXPECT_THROW_WHAT(statement, expected_exception, msg) \
	try { \
		statement; \
		FAIL() << "Expected exception: " #expected_exception "\n  Actual: no exception thrown"; \
	} \
	catch (const expected_exception& e) { \
		EXPECT_STREQ(msg, e.what()); \
	} \
	catch (...) { \
		FAIL() << "Expected exception: " #expected_exception "\n  Actual: different exception type thrown"; \
	}

TEST(build_context, resolve_class_description) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();

	auto stream = mint::BufferStream("");
	auto& module = scheduler.program().create_module({"test"}, mint::Module::State::ready);
	auto compiler = mint::Compiler(scheduler.program(), module);
	auto context = mint::BuildContext(stream, compiler);

	context.start_class_description("A",
	    mint::Reference::global | mint::Reference::const_address | mint::Reference::const_value);
	context.create_attribute("mbr", {}, mint::Reference::default_flags, compiler.make_data<mint::None>());
	context.resolve_class_description();

	const auto* a_desc = scheduler.program().global_data_description().find_class("A");
	ASSERT_NE(nullptr, a_desc);
	EXPECT_NO_THROW(a_desc->generate(mint::ClassBuilder(scheduler.program())));

	context.start_class_description("B",
	    mint::Reference::global | mint::Reference::const_address | mint::Reference::const_value);
	context.create_attribute("mbr", {}, mint::Reference::default_flags, compiler.make_data<mint::None>());
	context.resolve_class_description();

	const auto* b_desc = scheduler.program().global_data_description().find_class("B");
	ASSERT_NE(nullptr, b_desc);
	EXPECT_NO_THROW(b_desc->generate(mint::ClassBuilder(scheduler.program())));

	context.start_class_description("C",
	    mint::Reference::global | mint::Reference::const_address | mint::Reference::const_value);
	context.append_symbol_to_base_class_path("A");
	context.save_base_class_path();
	context.append_symbol_to_base_class_path("B");
	context.save_base_class_path();
	context.create_attribute("mbr", {}, mint::Reference::default_flags, compiler.make_data<mint::None>());
	context.resolve_class_description();

	const auto* c_desc = scheduler.program().global_data_description().find_class("C");
	ASSERT_NE(nullptr, c_desc);
	EXPECT_NO_THROW(c_desc->generate(mint::ClassBuilder(scheduler.program())));

	context.start_class_description("D",
	    mint::Reference::global | mint::Reference::const_address | mint::Reference::const_value);
	context.append_symbol_to_base_class_path("A");
	context.save_base_class_path();
	context.append_symbol_to_base_class_path("B");
	context.save_base_class_path();
	context.resolve_class_description();

	const auto* d_desc = scheduler.program().global_data_description().find_class("D");
	ASSERT_NE(nullptr, d_desc);
	EXPECT_THROW_WHAT(d_desc->generate(mint::ClassBuilder(scheduler.program())), mint::MintRuntimeError,
	    "member 'mbr' is ambiguous for class 'D'");
}
