#include <gtest/gtest.h>
#include <scheduler/destructor.h>
#include <scheduler/scheduler.h>
#include <memory/object.h>

using namespace mint;
using namespace std;

struct TestObject : public Object {
	TestObject() : Object(nullptr) {}
};

TEST(destructor, is_destructor) {

	vector<const char *>args = {"destructor"};
	Scheduler scheduler(args.size(), const_cast<char **>(args.data()));
	unique_ptr<Process> process;

	process.reset(new Process(scheduler.ast()->createCursor()));
	EXPECT_FALSE(is_destructor(process.get()));

	process.reset(new Destructor(new TestObject));
	EXPECT_TRUE(is_destructor(process.get()));
}
