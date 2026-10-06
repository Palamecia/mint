#include <gtest/gtest.h>
#include "mint/compiler/descriptions.h"
#include "mint/program/descriptions.h"
#include "mint/program/program.h"
#include "mint/program/symbol_scope.h"
#include "mint/program/module.h"
#include "mint/program/node.h"
#include "mint/compiler/compiler.h"
#include "mint/debug/debug_info.h"
#include "mint/system/buffer_stream.h"

namespace {

class TestModule : public mint::Module {
public:
	explicit TestModule(mint::Program& program) :
	    Module(program) {}

	using Module::push_node;
};

}

TEST(debug_infos, new_line) {

	auto program = mint::Program();
	auto infos = mint::DebugInfo();
	auto module = TestModule(program);

	infos.new_line(module, 1);
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	EXPECT_EQ(1, infos.line_number(0));

	infos.new_line(module, 5);
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	EXPECT_EQ(1, infos.line_number(0));
	EXPECT_EQ(5, infos.line_number(1));
}

TEST(debug_infos, line_number) {

	auto program = mint::Program();
	auto infos = mint::DebugInfo();
	auto module = TestModule(program);

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

	auto program = mint::Program();
	auto module = mint::ModuleInfo {
	    .bytecode = mint::Module(),
	    .description = mint::ModuleDescription("test"),
	};
	auto compiler = mint::Compiler(program, module);

	auto stream = mint::BufferStream(R"""(/* comment */

load module

if defined symbol {
	func()
}
)""");

	ASSERT_TRUE(compiler.build(stream));
	EXPECT_EQ(3, module.debug_info.line_number(0));
	EXPECT_EQ(3, module.debug_info.line_number(1));
	EXPECT_EQ(5, module.debug_info.line_number(2));
	EXPECT_EQ(5, module.debug_info.line_number(3));
}
