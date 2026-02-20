#include <gtest/gtest.h>
#include "mint/memory/casttool.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/scheduler/scheduler.h"

TEST(casttool, to_number) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	EXPECT_EQ(7357, mint::to_number(process->cursor(), mint::create_number(7357)));

	EXPECT_EQ(1, mint::to_number(process->cursor(), mint::create_boolean(true)));
	EXPECT_EQ(0, mint::to_number(process->cursor(), mint::create_boolean(false)));

	EXPECT_EQ(7357, mint::to_number(process->cursor(), mint::create_string(scheduler.ast(), "7357")));
	EXPECT_EQ(0x7E57, mint::to_number(process->cursor(), mint::create_string(scheduler.ast(), "0x7E57")));
	EXPECT_EQ(07357, mint::to_number(process->cursor(), mint::create_string(scheduler.ast(), "0o7357")));
	EXPECT_EQ(0b1010, mint::to_number(process->cursor(), mint::create_string(scheduler.ast(), "0b1010")));
	EXPECT_EQ(0, mint::to_number(process->cursor(), mint::create_string(scheduler.ast(), "test")));

	const auto it = mint::create_iterator_from(process->cursor(), mint::create_number(7357), mint::create_number(7356));

	EXPECT_EQ(7357, to_number(process->cursor(), it));
	EXPECT_EQ(7357, to_number(process->cursor(), *iterator_next(process->cursor(), it.data<mint::Iterator>())));
	EXPECT_EQ(7356, to_number(process->cursor(), it));
}

TEST(casttool, to_boolean) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	EXPECT_EQ(true, to_boolean(mint::create_number(7357)));
	EXPECT_EQ(false, to_boolean(mint::create_number(0)));

	EXPECT_EQ(true, to_boolean(mint::create_boolean(true)));
	EXPECT_EQ(false, to_boolean(mint::create_boolean(false)));

	EXPECT_EQ(true, to_boolean(mint::create_iterator_from(process->cursor(), mint::create_none())));
	EXPECT_EQ(false, to_boolean(mint::create_iterator_from(process->cursor())));
}

TEST(casttool, to_char) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	EXPECT_EQ("", to_char(mint::create_none()));
	EXPECT_EQ("", to_char(mint::create_null()));

	EXPECT_EQ("\x37", to_char(mint::create_number(0x37)));

	EXPECT_EQ("n", to_char(mint::create_boolean(false)));
	EXPECT_EQ("y", to_char(mint::create_boolean(true)));

	EXPECT_EQ("t", to_char(mint::create_string(scheduler.ast(), "test")));
}

TEST(casttool, to_string) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	EXPECT_EQ("", to_string(mint::create_none()));
	EXPECT_EQ("(null)", to_string(mint::create_null()));
	EXPECT_EQ("(function)", to_string(mint::create_function()));

	EXPECT_EQ("7357", to_string(mint::create_number(7357)));
	EXPECT_EQ("73.57", to_string(mint::create_number(73.57)));

	EXPECT_EQ("false", to_string(mint::create_boolean(false)));
	EXPECT_EQ("true", to_string(mint::create_boolean(true)));

	EXPECT_EQ("test", to_string(mint::create_string(scheduler.ast(), "test")));

	EXPECT_EQ("[test1, test2]",
	    to_string(mint::create_array(scheduler.ast(), {
	                                                      mint::create_string(scheduler.ast(), "test1"),
	                                                      mint::create_string(scheduler.ast(), "test2"),
	                                                  })));

	EXPECT_EQ("{key1 : value1}",
	    to_string(mint::create_hash(scheduler.ast(),
	        {
	            {mint::create_string(scheduler.ast(), "key1"), mint::create_string(scheduler.ast(), "value1")},
	        })));

	const auto it = mint::create_iterator_from(process->cursor(), mint::create_string(scheduler.ast(), "test1"),
	    mint::create_string(scheduler.ast(), "test2"));
	EXPECT_EQ("test1", to_string(it));
	EXPECT_EQ("test1", to_string(*iterator_next(process->cursor(), it.data<mint::Iterator>())));
	EXPECT_EQ("test2", to_string(it));
}

TEST(casttool, to_regex) {
	/// \todo
}

TEST(casttool, to_array) {
	/// \todo
}

TEST(casttool, to_hash) {
	/// \todo
}
