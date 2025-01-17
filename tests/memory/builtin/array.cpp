#include <gtest/gtest.h>
#include <mint/memory/builtin/array.h>
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/garbagecollector.h"
#include "mint/scheduler/scheduler.h"

using namespace mint;

TEST(array, join) {

	Scheduler scheduler(0, nullptr);
	Process *thread = scheduler.enable_testing();

	WeakReference array = create_array({
		create_string("a"),
		create_string("b"),
		create_string("c"),
	});

	WeakReference result = scheduler.invoke(array, Symbol("join"), create_string(", "));
	ASSERT_EQ(Data::FMT_OBJECT, result.data()->format);
	ASSERT_EQ(Class::STRING, result.data<Object>()->metadata->metatype());
	EXPECT_EQ("a, b, c", result.data<String>()->str);

	scheduler.disable_testing(thread);
}
