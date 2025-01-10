#include <gtest/gtest.h>
#include <mint/debug/debuginfo.h>
#include <mint/compiler/compiler.h>
#include <mint/system/bufferstream.h>
#include <mint/ast/module.h>

using namespace mint;

class TestModule : public Module {
public:
	TestModule() = default;
	using Module::push_node;
};

TEST(debuginfos, newLine) {
	
	DebugInfo infos;
	TestModule module;

	infos.new_line(&module, 1);
	module.push_node(Node(Node::EXIT_MODULE));
	EXPECT_EQ(1, infos.line_number(0));

	infos.new_line(&module, 5);
	module.push_node(Node(Node::EXIT_MODULE));
	EXPECT_EQ(1, infos.line_number(0));
	EXPECT_EQ(5, infos.line_number(1));
}

TEST(debuginfos, lineNumber) {
	
	DebugInfo infos;
	TestModule module;

	infos.new_line(&module, 1);
	module.push_node(Node(Node::EXIT_MODULE));
	module.push_node(Node(Node::EXIT_MODULE));
	module.push_node(Node(Node::EXIT_MODULE));
	module.push_node(Node(Node::EXIT_MODULE));
	module.push_node(Node(Node::EXIT_MODULE));

	infos.new_line(&module, 2);
	module.push_node(Node(Node::EXIT_MODULE));
	module.push_node(Node(Node::EXIT_MODULE));
	module.push_node(Node(Node::EXIT_MODULE));
	module.push_node(Node(Node::EXIT_MODULE));
	module.push_node(Node(Node::EXIT_MODULE));

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

TEST(debuginfos, newLineFromSource) {
	
	DebugInfo infos;
	TestModule module;
	Compiler compiler;

	BufferStream stream(R"""(/* comment */

load module

if defined symbol {
	func()
}
)""");

	ASSERT_TRUE(compiler.build(&stream, { Module::INVALID_ID, &module, &infos }));
	EXPECT_EQ(3, infos.line_number(0));
	EXPECT_EQ(3, infos.line_number(1));
	EXPECT_EQ(5, infos.line_number(2));
	EXPECT_EQ(5, infos.line_number(3));
}
