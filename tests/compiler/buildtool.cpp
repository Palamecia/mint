#include <gtest/gtest.h>
#include <compiler/buildtool.h>
#include <system/bufferstream.h>

using namespace mint;

TEST(buildtool, resolveClassDescription) {

	BufferStream stream("");
	BuildContext context(&stream, AbstractSyntaxTree::instance().createModule());

	context.startClassDescription("A");
	context.createMember(Reference::standard, "mbr");
	context.resolveClassDescription();

	ClassDescription *a_desc = GlobalData::instance().findClassDescription("A");
	ASSERT_NE(nullptr, a_desc);
	EXPECT_NE(nullptr, a_desc->generate());

	context.startClassDescription("B");
	context.createMember(Reference::standard, "mbr");
	context.resolveClassDescription();

	ClassDescription *b_desc = GlobalData::instance().findClassDescription("B");
	ASSERT_NE(nullptr, b_desc);
	EXPECT_NE(nullptr, b_desc->generate());

	context.startClassDescription("C");
	context.appendSymbolToBaseClassPath("A");
	context.saveBaseClassPath();
	context.appendSymbolToBaseClassPath("B");
	context.saveBaseClassPath();
	context.createMember(Reference::standard, "mbr");
	context.resolveClassDescription();

	ClassDescription *c_desc = GlobalData::instance().findClassDescription("C");
	ASSERT_NE(nullptr, c_desc);
	EXPECT_NE(nullptr, c_desc->generate());

	context.startClassDescription("D");
	context.appendSymbolToBaseClassPath("A");
	context.saveBaseClassPath();
	context.appendSymbolToBaseClassPath("B");
	context.saveBaseClassPath();
	context.resolveClassDescription();

	ClassDescription *d_desc = GlobalData::instance().findClassDescription("D");
	ASSERT_NE(nullptr, d_desc);
	ASSERT_DEATH(d_desc->generate(), "member 'mbr' is ambiguous for class 'D'");
}
