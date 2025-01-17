#include <gtest/gtest.h>
#include <mint/scheduler/destructor.h>
#include "mint/scheduler/processor.h"
#include "mint/scheduler/scheduler.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/memory/class.h"
#include "mint/memory/classtool.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"

#include <memory>

TEST(destructor, is_destructor) {

	mint::Scheduler scheduler(0, nullptr);
	mint::AbstractSyntaxTree *ast = scheduler.ast();
	mint::Module::Info module = ast->create_module(mint::Module::READY);

	mint::Process *thread = scheduler.enable_testing();
	ASSERT_NE(nullptr, thread);
	EXPECT_FALSE(is_destructor(thread));

	mint::Class *test_class = mint::create_class("__test_class__", {
																	   {mint::builtin_symbols::DELETE_METHOD,
																		mint::create_function(module, 2, R"(
																			def (self) {}
																		)")},
																   });
	ASSERT_NE(nullptr, test_class);

	mint::WeakReference object = scheduler.invoke(test_class);
	ASSERT_EQ(mint::Data::FMT_OBJECT, object.data()->format);

	mint::Class::MemberInfo *member = object.data<mint::Object>()->metadata->find_operator(mint::Class::DELETE_OPERATOR);
	ASSERT_NE(nullptr, member);

	mint::Reference &member_ref = mint::Class::MemberInfo::get(member, object.data<mint::Object>()->data);
	ASSERT_EQ(mint::Data::FMT_FUNCTION, member_ref.data()->format);

	auto destructor = std::make_unique<mint::Destructor>(object.data<mint::Object>(), std::move(member_ref), member->owner, thread);
	EXPECT_TRUE(is_destructor(destructor.get()));

	mint::unlock_processor();
	destructor.reset();
	mint::lock_processor();

	EXPECT_TRUE(scheduler.disable_testing(thread));
}
