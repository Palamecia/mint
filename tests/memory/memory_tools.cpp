#include <gtest/gtest.h>
#include <utility>
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/cursor.h"
#include "mint/ast/file_printer.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/class_tools.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/object_printer.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"

TEST(memory_tools, get_stack_base) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	mint::Cursor& cursor = process->cursor();

	cursor.stack().emplace_back(mint::create_none());
	cursor.stack().emplace_back(mint::create_none());
	cursor.stack().emplace_back(mint::create_none());
	EXPECT_EQ(2, mint::get_stack_base(cursor));

	cursor.stack().pop_back();
	EXPECT_EQ(1, mint::get_stack_base(cursor));
}

TEST(memory_tools, type_name) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	EXPECT_EQ("none", mint::type_name(mint::create_none()));
	EXPECT_EQ("null", mint::type_name(mint::create_null()));
	EXPECT_EQ("number", mint::type_name(mint::create_number(0.)));
	EXPECT_EQ("boolean", mint::type_name(mint::create_boolean(false)));
	EXPECT_EQ("function", mint::type_name(mint::create_function()));
	EXPECT_EQ("string", mint::type_name(mint::create_string(scheduler.ast())));
	EXPECT_EQ("regex", mint::type_name(mint::create_regex(scheduler.ast())));
	EXPECT_EQ("array", mint::type_name(mint::create_array(scheduler.ast())));
	EXPECT_EQ("hash", mint::type_name(mint::create_hash(scheduler.ast())));
	EXPECT_EQ("iterator", mint::type_name(mint::create_iterator(scheduler.ast())));
}

TEST(memory_tools, is_class) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	const auto ref = mint::make_reference<mint::String>(mint::create_flags, scheduler.ast());
	EXPECT_TRUE(is_class(ref.data<mint::String>()));

	ref.data<mint::String>().construct();
	EXPECT_FALSE(is_class(ref.data<mint::String>()));
}

TEST(memory_tools, is_object) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	const auto ref = mint::make_reference<mint::String>(mint::create_flags, scheduler.ast());
	EXPECT_FALSE(is_object(ref.data<mint::String>()));

	ref.data<mint::String>().construct();
	EXPECT_TRUE(is_object(ref.data<mint::String>()));
}

TEST(memory_tools, create_printer_from_fd) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	mint::Cursor& cursor = process->cursor();

	cursor.stack().emplace_back(mint::create_number(0));
	auto printer = create_printer(cursor);
	EXPECT_NE(nullptr, dynamic_cast<mint::FilePrinter*>(printer.get()));
}

TEST(memory_tools, create_printer_from_path) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	mint::Cursor& cursor = process->cursor();

	cursor.stack().emplace_back(mint::create_string(scheduler.ast(), "test"));
	auto printer = create_printer(cursor);
	EXPECT_NE(nullptr, dynamic_cast<mint::FilePrinter*>(printer.get()));
}

TEST(memory_tools, create_printer_from_object) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	mint::Cursor& cursor = process->cursor();
	mint::Class& test_class = mint::create_class(scheduler.ast(), "__test_class__", {});

	cursor.stack().emplace_back(mint::create_object(test_class));
	auto printer = create_printer(cursor);
	EXPECT_NE(nullptr, dynamic_cast<mint::ObjectPrinter*>(printer.get()));
}

TEST(memory_tools, print) {
	/// \todo
}

TEST(memory_tools, capture_symbol) {
	/// \todo
}

TEST(memory_tools, capture_all_symbols) {
	/// \todo
}

TEST(memory_tools, init_call) {
	/// \todo
}

TEST(memory_tools, init_member_call) {
	/// \todo
}

TEST(memory_tools, exit_call) {
	/// \todo
}

TEST(memory_tools, init_parameter) {
	/// \todo
}

TEST(memory_tools, find_function_signature) {
	/// \todo
}

TEST(memory_tools, yield) {
	/// \todo
}

TEST(memory_tools, load_default_result) {
	/// \todo
}

TEST(memory_tools, get_symbol_reference) {
	/// \todo
}

TEST(memory_tools, get_object_member) {
	/// \todo
}

TEST(memory_tools, reduce_member) {
	/// \todo
}

TEST(memory_tools, var_symbol) {
	/// \todo
}

TEST(memory_tools, create_symbol) {
	/// \todo
}

TEST(memory_tools, array_append_from_stack) {
	/// \todo
}

TEST(memory_tools, array_append) {
	/// \todo
}

TEST(memory_tools, array_get_item) {
	/// \todo
}

TEST(memory_tools, array_index) {
	/// \todo
}

TEST(memory_tools, hash_insert_from_stack) {
	/// \todo
}

TEST(memory_tools, hash_insert) {
	/// \todo
}

TEST(memory_tools, hash_get_item) {
	/// \todo
}

TEST(memory_tools, hash_get_key) {
	/// \todo
}

TEST(memory_tools, hash_get_value) {
	/// \todo
}

TEST(memory_tools, iterator_yield) {
	/// \todo
}

TEST(memory_tools, iterator_add) {
	/// \todo
}

TEST(memory_tools, iterator_next) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	const auto it = mint::create_iterator_from(process->cursor(), mint::create_number(0), mint::create_number(1));

	{
		ASSERT_TRUE(iterator_get(it.data<mint::Iterator>()));
		auto item = std::move(*iterator_next(process->cursor(), it.data<mint::Iterator>()));
		ASSERT_EQ(mint::Data::Format::number, item.data().format());
		EXPECT_EQ(0., item.data<mint::Number>().value);
	}

	{
		ASSERT_TRUE(iterator_get(it.data<mint::Iterator>()));
		auto item = std::move(*iterator_next(process->cursor(), it.data<mint::Iterator>()));
		ASSERT_EQ(mint::Data::Format::number, item.data().format());
		EXPECT_EQ(1., item.data<mint::Number>().value);
	}

	EXPECT_FALSE(iterator_next(process->cursor(), it.data<mint::Iterator>()));
}

TEST(memory_tools, regex_match) {
	/// \todo
}

TEST(memory_tools, regex_unmatch) {
	/// \todo
}
