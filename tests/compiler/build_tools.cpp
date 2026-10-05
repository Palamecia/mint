#include "mint/program/class_register.h"
#include "mint/program/module.h"
#include "mint/program/symbol.h"
#include "mint/compiler/abstract_syntax_tree.h"
#include "mint/compiler/build_tools.h"
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

TEST(build_tools, resolve_class_description) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();

	mint::BufferStream stream("");
	mint::Compiler compiler(scheduler.program());
	mint::BytecodeBuildContext context(stream, compiler, scheduler.program().create_module(mint::Module::State::ready));

	context.start_class_description("A",
	    mint::Reference::global | mint::Reference::const_address | mint::Reference::const_value);
	context.create_member(mint::Reference::default_flags, mint::Symbol("mbr"),
	    mint::GarbageCollector::instance().alloc<mint::None>());
	context.resolve_class_description();

	mint::ClassDescription* a_desc = scheduler.program().global_data().find_class_description("A");
	ASSERT_NE(nullptr, a_desc);
	EXPECT_NO_THROW(a_desc->generate());

	context.start_class_description("B",
	    mint::Reference::global | mint::Reference::const_address | mint::Reference::const_value);
	context.create_member(mint::Reference::default_flags, mint::Symbol("mbr"),
	    mint::GarbageCollector::instance().alloc<mint::None>());
	context.resolve_class_description();

	mint::ClassDescription* b_desc = scheduler.program().global_data().find_class_description("B");
	ASSERT_NE(nullptr, b_desc);
	EXPECT_NO_THROW(b_desc->generate());

	context.start_class_description("C",
	    mint::Reference::global | mint::Reference::const_address | mint::Reference::const_value);
	context.append_symbol_to_base_class_path("A");
	context.save_base_class_path();
	context.append_symbol_to_base_class_path("B");
	context.save_base_class_path();
	context.create_member(mint::Reference::default_flags, mint::Symbol("mbr"),
	    mint::GarbageCollector::instance().alloc<mint::None>());
	context.resolve_class_description();

	mint::ClassDescription* c_desc = scheduler.program().global_data().find_class_description("C");
	ASSERT_NE(nullptr, c_desc);
	EXPECT_NO_THROW(c_desc->generate());

	context.start_class_description("D",
	    mint::Reference::global | mint::Reference::const_address | mint::Reference::const_value);
	context.append_symbol_to_base_class_path("A");
	context.save_base_class_path();
	context.append_symbol_to_base_class_path("B");
	context.save_base_class_path();
	context.resolve_class_description();

	mint::ClassDescription* d_desc = scheduler.program().global_data().find_class_description("D");
	ASSERT_NE(nullptr, d_desc);
	EXPECT_THROW_WHAT(d_desc->generate(), mint::MintRuntimeError, "member 'mbr' is ambiguous for class 'D'");
}

TEST(build_tools, abstract_syntax_tree_builds_owned_expression_nodes) {
	mint::BufferStream stream("");
	mint::AbstractSyntaxTree tree;
	mint::AbstractSyntaxTreeBuildContext context(stream, tree);

	context.reduce_identifier("left");
	context.reduce_number_literal("42");
	context.reduce_addition_expression();
	context.reduce_expression_statement();
	context.reduce_module_stmt_list();

	ASSERT_NE(nullptr, tree.root());
	ASSERT_EQ(1, tree.root()->statements.size());
	const auto* statement = dynamic_cast<const mint::ExpressionStatement*>(tree.root()->statements.front().get());
	ASSERT_NE(nullptr, statement);
	const auto* expression = dynamic_cast<const mint::BinaryExpression*>(statement->expression.get());
	ASSERT_NE(nullptr, expression);
	EXPECT_EQ("+", expression->op);
	const auto* left = dynamic_cast<const mint::Identifier*>(expression->left.get());
	ASSERT_NE(nullptr, left);
	EXPECT_EQ("left", left->name);
	const auto* right = dynamic_cast<const mint::Literal*>(expression->right.get());
	ASSERT_NE(nullptr, right);
	EXPECT_EQ(mint::Literal::Kind::number, right->kind);
	EXPECT_EQ("42", right->value);
}
