#include <gtest/gtest.h>
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/class_register.h"
#include "mint/ast/module.h"
#include "mint/ast/node.h"
#include "mint/compiler/compiler.h"
#include "mint/debug/debug_info.h"
#include "mint/system/buffer_stream.h"

class TestModule : public mint::Module {
public:
	TestModule() = default;
	using Module::push_node;
};

TEST(debug_infos, new_line) {

	auto infos = mint::DebugInfo();
	auto module = TestModule();

	infos.new_line(module, 1);
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	EXPECT_EQ(1, infos.line_number(0));

	infos.new_line(module, 5);
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	EXPECT_EQ(1, infos.line_number(0));
	EXPECT_EQ(5, infos.line_number(1));
}

TEST(debug_infos, line_number) {

	auto infos = mint::DebugInfo();
	auto module = TestModule();

	infos.new_line(module, 1);
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));

	infos.new_line(module, 2);
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));

	infos.new_line(module, 3);

	EXPECT_EQ(1, infos.line_number(0));
	EXPECT_EQ(1, infos.line_number(1));
	EXPECT_EQ(1, infos.line_number(2));
	EXPECT_EQ(1, infos.line_number(3));
	EXPECT_EQ(1, infos.line_number(4));
	EXPECT_EQ(2, infos.line_number(5));
	EXPECT_EQ(2, infos.line_number(6));
	EXPECT_EQ(2, infos.line_number(7));
	EXPECT_EQ(2, infos.line_number(8));
	EXPECT_EQ(2, infos.line_number(9));
	EXPECT_EQ(3, infos.line_number(10));
	EXPECT_EQ(3, infos.line_number(11));
}

TEST(debug_infos, new_line_from_source) {

	auto ast = mint::AbstractSyntaxTree();
	auto module = mint::ModuleInfo();
	auto compiler = mint::Compiler(ast);

	auto stream = mint::BufferStream(R"""(/* comment */

load module

if defined symbol {
	func()
}
)""");

	ASSERT_TRUE(compiler.build(stream, module));
	EXPECT_EQ(3, module.debug_info.line_number(0));
	EXPECT_EQ(3, module.debug_info.line_number(1));
	EXPECT_EQ(5, module.debug_info.line_number(2));
	EXPECT_EQ(5, module.debug_info.line_number(3));
}
