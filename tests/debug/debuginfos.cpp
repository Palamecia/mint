#include <gtest/gtest.h>
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/classregister.h"
#include "mint/ast/module.h"
#include "mint/ast/node.h"
#include "mint/compiler/compiler.h"
#include "mint/debug/debuginfo.h"
#include "mint/system/bufferstream.h"

class TestModule : public mint::Module {
public:
	TestModule() = default;
	using Module::push_node;
};

TEST(debuginfos, new_line) {

	mint::DebugInfo infos;
	TestModule module;

	infos.new_line(&module, 1);
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	EXPECT_EQ(1, infos.line_number(0));

	infos.new_line(&module, 5);
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	EXPECT_EQ(1, infos.line_number(0));
	EXPECT_EQ(5, infos.line_number(1));
}

TEST(debuginfos, line_number) {

	mint::DebugInfo infos;
	TestModule module;

	infos.new_line(&module, 1);
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));

	infos.new_line(&module, 2);
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));
	module.push_node(mint::Node(mint::Node::Command::exit_module));

	infos.new_line(&module, 3);

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

TEST(debuginfos, new_line_from_source) {

	mint::AbstractSyntaxTree ast;
	mint::DebugInfo infos;
	TestModule module;
	mint::Compiler compiler(ast);

	mint::BufferStream stream(R"""(/* comment */

load module

if defined symbol {
	func()
}
)""");

	ASSERT_TRUE(compiler.build(stream, mint::Module::Info {
	                                       .id = mint::Module::invalid_id,
	                                       .module = &module,
	                                       .debug_info = &infos,
	                                   }));
	EXPECT_EQ(3, infos.line_number(0));
	EXPECT_EQ(3, infos.line_number(1));
	EXPECT_EQ(5, infos.line_number(2));
	EXPECT_EQ(5, infos.line_number(3));
}
