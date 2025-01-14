#include <gtest/gtest.h>
#include <mint/ast/abstractsyntaxtree.h>
#include <mint/scheduler/destructor.h>
#include <mint/scheduler/scheduler.h>
#include <mint/memory/object.h>

using namespace mint;

struct TestObject : public Object {
	TestObject() :
		Object(nullptr) {}
};

TEST(destructor, is_destructor) {

	AbstractSyntaxTree ast;
	std::unique_ptr<Process> process;
	std::unique_ptr<Process> destructor;

	process.reset(new Process(ast.create_cursor()));
	EXPECT_FALSE(is_destructor(process.get()));

	WeakReference object;
	destructor.reset(new Destructor(new TestObject, std::move(object), nullptr, process.get()));
	EXPECT_TRUE(is_destructor(destructor.get()));
}
