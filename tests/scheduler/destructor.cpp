#include <gtest/gtest.h>
#include <ast/abstractsyntaxtree.h>
#include <scheduler/destructor.h>
#include <scheduler/scheduler.h>
#include <memory/object.h>

using namespace mint;
using namespace std;

struct TestObject : public Object {
	TestObject() : Object(nullptr) {}
};

TEST(destructor, is_destructor) {

	unique_ptr<Process> process;

	process.reset(new Process(AbstractSyntaxTree::instance().createCursor()));
	EXPECT_FALSE(is_destructor(process.get()));

	process.reset(new Destructor(new TestObject));
	EXPECT_TRUE(is_destructor(process.get()));
}
