#include <gtest/gtest.h>
#include "mint/scheduler/scheduler.h"
#include "mint/ast/module.h"
#include "mint/ast/symbol.h"
#include "mint/memory/class.h"
#include "mint/memory/classtool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/reference.h"
#include "mint/memory/object.h"
#include "mint/memory/data.h"

TEST(scheduler, invoke_function) {

	mint::Scheduler scheduler({});
	mint::AbstractSyntaxTree& ast = scheduler.ast();
	mint::Module::Info module = ast.create_module(mint::Module::State::ready);
	auto thread = scheduler.enable_testing();
	ASSERT_NE(nullptr, thread);

	const auto fn = mint::create_function(ast, module, 2, R"(
        def (a, b) {
            return a + b
        }
    )");
	ASSERT_EQ(mint::Data::function_format, fn.data().format());

	const auto result = scheduler.invoke(fn, mint::create_signed_number(2), mint::create_signed_number(2));
	ASSERT_EQ(mint::Data::number_format, result.data().format());
	EXPECT_EQ(4, result.data<mint::Number>().value);
}

TEST(scheduler, invoke_new) {

	mint::Scheduler scheduler({});
	mint::AbstractSyntaxTree& ast = scheduler.ast();
	auto thread = scheduler.enable_testing();
	ASSERT_NE(nullptr, thread);

	mint::Class& test_class = mint::create_class(ast, "__test_class__", {});

	const auto object = scheduler.invoke(test_class);
	ASSERT_EQ(mint::Data::object_format, object.data().format());
}

TEST(scheduler, invoke_method) {

	mint::Scheduler scheduler({});
	mint::AbstractSyntaxTree& ast = scheduler.ast();
	mint::Module::Info module = ast.create_module(mint::Module::State::ready);
	auto thread = scheduler.enable_testing();
	ASSERT_NE(nullptr, thread);

	mint::Class& test_class = mint::create_class(ast, "__test_class__",
	    {
	        {mint::builtin_symbols::new_method, mint::create_function(ast, module, 2, R"(
															def (self, value) {
																self.value = value
																return self
															}
														)")},
	        {"getSelf", mint::create_function(ast, module, 1, R"(
															def (self) {
																return self
															}
														)")},
	        {"getValue", mint::create_function(ast, module, 1, R"(
															def (self) {
																return self.value
															}
														)")},
	        {"value", mint::create_none()},
	    });

	const auto object = scheduler.invoke(test_class, mint::create_signed_number(42));
	ASSERT_EQ(mint::Data::object_format, object.data().format());

	{
		const auto result = scheduler.invoke(object, mint::Symbol("getSelf"));
		ASSERT_EQ(mint::Data::object_format, result.data().format());
		EXPECT_EQ(&object.data(), &result.data());
	}

	{
		const auto result = scheduler.invoke(object, mint::Symbol("getValue"));
		ASSERT_EQ(mint::Data::number_format, result.data().format());
		EXPECT_EQ(42, result.data<mint::Number>().value);
	}
}

TEST(scheduler, invoke_operator) {

	mint::Scheduler scheduler({});
	mint::AbstractSyntaxTree& ast = scheduler.ast();
	mint::Module::Info module = ast.create_module(mint::Module::State::ready);
	auto thread = scheduler.enable_testing();
	ASSERT_NE(nullptr, thread);

	mint::Class& test_class = mint::create_class(ast, "__test_class__",
	    {
	        {mint::builtin_symbols::new_method, mint::create_function(ast, module, 2, R"(
										def (self, value) {
											self.value = value
											return self
										}
									)")},
	        {mint::builtin_symbols::add_operator, mint::create_function(ast, module, 2, R"(
										def (self, value) {
											return self.value + value
										}
									)")},
	        {"value", mint::create_none()},
	    });

	const auto object = scheduler.invoke(test_class, mint::create_number(2));
	ASSERT_EQ(mint::Data::object_format, object.data().format());

	{
		const auto result = scheduler.invoke(object, mint::Class::add_operator, mint::create_number(2));
		ASSERT_EQ(mint::Data::number_format, result.data().format());
		EXPECT_EQ(4, result.data<mint::Number>().value);
	}
}
