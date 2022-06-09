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

	AbstractSyntaxTree ast;
	unique_ptr<Process> process;
	unique_ptr<Process> destructor;

	process.reset(new Process(ast.createCursor()));
	EXPECT_FALSE(is_destructor(process.get()));

	WeakReference object;
	destructor.reset(new Destructor(new TestObject, move(object), nullptr, process.get()));
	EXPECT_TRUE(is_destructor(destructor.get()));
}
