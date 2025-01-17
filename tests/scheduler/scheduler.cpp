#include <gtest/gtest.h>
#include <mint/scheduler/scheduler.h>
#include "mint/ast/symbol.h"
#include "mint/memory/class.h"
#include "mint/memory/classtool.h"
#include "mint/scheduler/process.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/reference.h"
#include "mint/memory/object.h"
#include "mint/memory/data.h"

TEST(scheduler, invoke_function) {

	mint::Scheduler scheduler(0, nullptr);
	mint::AbstractSyntaxTree *ast = scheduler.ast();
	mint::Module::Info module = ast->create_module(mint::Module::READY);

	mint::Process *thread = scheduler.enable_testing();
	ASSERT_NE(nullptr, thread);

	mint::WeakReference fn = mint::create_function(module, 2, R"(
        def (a, b) {
            return a + b
        }
    )");
	ASSERT_EQ(mint::Data::FMT_FUNCTION, fn.data()->format);

	mint::WeakReference result = scheduler.invoke(fn, mint::create_number(2), mint::create_number(2));
	ASSERT_EQ(mint::Data::FMT_NUMBER, result.data()->format);
	EXPECT_EQ(4, result.data<mint::Number>()->value);

	EXPECT_TRUE(scheduler.disable_testing(thread));
}

TEST(scheduler, invoke_new) {

	mint::Scheduler scheduler(0, nullptr);

	mint::Process *thread = scheduler.enable_testing();
	ASSERT_NE(nullptr, thread);

	mint::Class *test_class = mint::create_class("__test_class__", {});
	ASSERT_NE(nullptr, test_class);

	mint::WeakReference object = scheduler.invoke(test_class);
	ASSERT_EQ(mint::Data::FMT_OBJECT, object.data()->format);

	EXPECT_TRUE(scheduler.disable_testing(thread));
}

TEST(scheduler, invoke_method) {

	mint::Scheduler scheduler(0, nullptr);
	mint::AbstractSyntaxTree *ast = scheduler.ast();
	mint::Module::Info module = ast->create_module(mint::Module::READY);

	mint::Process *thread = scheduler.enable_testing();
	ASSERT_NE(nullptr, thread);

	mint::Class *test_class = mint::create_class("__test_class__",
												 {
													 {mint::builtin_symbols::NEW_METHOD,
													  mint::create_function(module, 2, R"(
															def (self, value) {
																self.value = value
																return self
															}
														)")},
													 {"getSelf", mint::create_function(module, 1, R"(
															def (self) {
																return self
															}
														)")},
													 {"getValue", mint::create_function(module, 1, R"(
															def (self) {
																return self.value
															}
														)")},
													 {"value", mint::WeakReference::create<mint::None>()},
												 });
	ASSERT_NE(nullptr, test_class);

	mint::WeakReference object = scheduler.invoke(test_class, mint::create_number(42));
	ASSERT_EQ(mint::Data::FMT_OBJECT, object.data()->format);

	{
		mint::WeakReference result = scheduler.invoke(object, mint::Symbol("getSelf"));
		ASSERT_EQ(mint::Data::FMT_OBJECT, result.data()->format);
		EXPECT_EQ(object.data(), result.data());
	}

	{
		mint::WeakReference result = scheduler.invoke(object, mint::Symbol("getValue"));
		ASSERT_EQ(mint::Data::FMT_NUMBER, result.data()->format);
		EXPECT_EQ(42, result.data<mint::Number>()->value);
	}

	EXPECT_TRUE(scheduler.disable_testing(thread));
}

TEST(scheduler, invoke_operator) {

	mint::Scheduler scheduler(0, nullptr);
	mint::AbstractSyntaxTree *ast = scheduler.ast();
	mint::Module::Info module = ast->create_module(mint::Module::READY);

	mint::Process *thread = scheduler.enable_testing();
	ASSERT_NE(nullptr, thread);

	mint::Class *test_class =
		mint::create_class("__test_class__",
						   {
							   {mint::builtin_symbols::NEW_METHOD, mint::create_function(module, 2, R"(
										def (self, value) {
											self.value = value
											return self
										}
									)")},
							   {mint::builtin_symbols::ADD_OPERATOR, mint::create_function(module, 2, R"(
										def (self, value) {
											return self.value + value
										}
									)")},
							   {"value", mint::WeakReference::create<mint::None>()},
						   });
	ASSERT_NE(nullptr, test_class);

	mint::WeakReference object = scheduler.invoke(test_class, mint::create_number(2));
	ASSERT_EQ(mint::Data::FMT_OBJECT, object.data()->format);

	{
		mint::WeakReference result = scheduler.invoke(object, mint::Class::ADD_OPERATOR, mint::create_number(2));
		ASSERT_EQ(mint::Data::FMT_NUMBER, result.data()->format);
		EXPECT_EQ(4, result.data<mint::Number>()->value);
	}

	EXPECT_TRUE(scheduler.disable_testing(thread));
}
