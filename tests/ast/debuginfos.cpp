#include <gtest/gtest.h>
#include <ast/debuginfos.h>
#include <ast/module.h>

using namespace mint;

class TestModule : public Module {
public:
	TestModule() = default;
	using Module::pushNode;
};

TEST(debuginfos, newLine) {

	DebugInfos infos;
	TestModule *module = new TestModule;

	module->pushNode(Node());
	infos.newLine(module, 1);
	EXPECT_EQ(1, infos.lineNumber(0));

	module->pushNode(Node());
	infos.newLine(module, 5);
	EXPECT_EQ(5, infos.lineNumber(5));
}

TEST(debuginfos, lineNumber) {

	DebugInfos infos;
	TestModule *module = new TestModule;

	infos.newLine(module, 1);

	module->pushNode(Node());
	module->pushNode(Node());
	module->pushNode(Node());
	module->pushNode(Node());
	module->pushNode(Node());
	infos.newLine(module, 2);

	module->pushNode(Node());
	module->pushNode(Node());
	module->pushNode(Node());
	module->pushNode(Node());
	module->pushNode(Node());
	infos.newLine(module, 3);

	EXPECT_EQ(1, infos.lineNumber(0));
	EXPECT_EQ(1, infos.lineNumber(1));
	EXPECT_EQ(1, infos.lineNumber(2));
	EXPECT_EQ(1, infos.lineNumber(3));
	EXPECT_EQ(1, infos.lineNumber(4));
	EXPECT_EQ(2, infos.lineNumber(5));
	EXPECT_EQ(2, infos.lineNumber(6));
	EXPECT_EQ(2, infos.lineNumber(7));
	EXPECT_EQ(2, infos.lineNumber(8));
	EXPECT_EQ(2, infos.lineNumber(9));
}
