#include <gtest/gtest.h>
#include <mint/compiler/buildtool.h>
#include <mint/system/bufferstream.h>
#include <mint/ast/abstractsyntaxtree.h>

using namespace mint;

TEST(buildtool, resolveClassDescription) {

	AbstractSyntaxTree ast;

	BufferStream stream("");
	BuildContext context(&stream, ast.create_module(Module::READY));

	context.start_class_description("A");
	context.create_member(Reference::DEFAULT, "mbr");
	context.resolve_class_description();

	ClassDescription *a_desc = ast.global_data().find_class_description("A");
	ASSERT_NE(nullptr, a_desc);
	EXPECT_NE(nullptr, a_desc->generate());

	context.start_class_description("B");
	context.create_member(Reference::DEFAULT, "mbr");
	context.resolve_class_description();

	ClassDescription *b_desc = ast.global_data().find_class_description("B");
	ASSERT_NE(nullptr, b_desc);
	EXPECT_NE(nullptr, b_desc->generate());

	context.start_class_description("C");
	context.append_symbol_to_base_class_path("A");
	context.save_base_class_path();
	context.append_symbol_to_base_class_path("B");
	context.save_base_class_path();
	context.create_member(Reference::DEFAULT, "mbr");
	context.resolve_class_description();

	ClassDescription *c_desc = ast.global_data().find_class_description("C");
	ASSERT_NE(nullptr, c_desc);
	EXPECT_NE(nullptr, c_desc->generate());

	context.start_class_description("D");
	context.append_symbol_to_base_class_path("A");
	context.save_base_class_path();
	context.append_symbol_to_base_class_path("B");
	context.save_base_class_path();
	context.resolve_class_description();

	ClassDescription *d_desc = ast.global_data().find_class_description("D");
	ASSERT_NE(nullptr, d_desc);
	ASSERT_DEATH(d_desc->generate(), "member 'mbr' is ambiguous for class 'D'");
}
