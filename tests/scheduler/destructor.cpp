#include <gtest/gtest.h>
#include "mint/scheduler/destructor.h"
#include "mint/ast/module.h"
#include "mint/ast/symbol.h"
#include "mint/memory/data.h"
#include "mint/scheduler/scheduler.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/memory/class.h"
#include "mint/memory/class_tools.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"

#include <memory>
#include <string>
#include <vector>

TEST(destructor, is_destructor) {

	mint::Scheduler scheduler({});
	mint::AbstractSyntaxTree& ast = scheduler.ast();
	auto module = ast.create_module(mint::Module::State::ready);

	auto thread = scheduler.enable_testing();
	ASSERT_NE(nullptr, thread);
	EXPECT_FALSE(is_destructor(*thread));

	mint::Class& test_class = mint::create_class(ast, "__test_class__",
	    {
	        {mint::builtin_symbols::delete_method, mint::create_function(ast, module, 1, R"(
																			def (self) {}
																		)")},
	    });

	const auto object = scheduler.invoke(test_class);
	ASSERT_EQ(mint::Data::Format::object, object.data().format());

	mint::Class::MemberInfo* member = object.data<mint::Object>().metadata.find_operator(mint::Class::delete_operator);
	ASSERT_NE(nullptr, member);

	const auto& member_ref = mint::Class::MemberInfo::get(*member, object.data<mint::Object>().data);
	ASSERT_EQ(mint::Data::Format::function, member_ref.data().format());

	auto destructor = mint::Destructor(&object.data<mint::Object>(), member_ref, member->owner, *thread);
	EXPECT_TRUE(is_destructor(destructor));
}
