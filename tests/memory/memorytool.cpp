#include <gtest/gtest.h>
#include <utility>
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/ast/fileprinter.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/classtool.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/objectprinter.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"

TEST(memorytool, get_stack_base) {

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

TEST(memorytool, type_name) {

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

TEST(memorytool, is_class) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	const auto ref = mint::make_reference<mint::String>(mint::create_flags, scheduler.ast());
	EXPECT_TRUE(is_class(ref.data<mint::String>()));

	ref.data<mint::String>().construct();
	EXPECT_FALSE(is_class(ref.data<mint::String>()));
}

TEST(memorytool, is_object) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	const auto ref = mint::make_reference<mint::String>(mint::create_flags, scheduler.ast());
	EXPECT_FALSE(is_object(ref.data<mint::String>()));

	ref.data<mint::String>().construct();
	EXPECT_TRUE(is_object(ref.data<mint::String>()));
}

TEST(memorytool, create_printer_from_fd) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	mint::Cursor& cursor = process->cursor();

	cursor.stack().emplace_back(mint::create_number(0));
	auto printer = create_printer(cursor);
	EXPECT_NE(nullptr, dynamic_cast<mint::FilePrinter*>(printer.get()));
}

TEST(memorytool, create_printer_from_path) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	mint::Cursor& cursor = process->cursor();

	cursor.stack().emplace_back(mint::create_string(scheduler.ast(), "test"));
	auto printer = create_printer(cursor);
	EXPECT_NE(nullptr, dynamic_cast<mint::FilePrinter*>(printer.get()));
}

TEST(memorytool, create_printer_from_object) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	mint::Cursor& cursor = process->cursor();
	mint::Class& test_class = mint::create_class(scheduler.ast(), "__test_class__", {});

	cursor.stack().emplace_back(mint::create_object(test_class));
	auto printer = create_printer(cursor);
	EXPECT_NE(nullptr, dynamic_cast<mint::ObjectPrinter*>(printer.get()));
}

TEST(memorytool, print) {
	/// \todo
}

TEST(memorytool, capture_symbol) {
	/// \todo
}

TEST(memorytool, capture_all_symbols) {
	/// \todo
}

TEST(memorytool, init_call) {
	/// \todo
}

TEST(memorytool, init_member_call) {
	/// \todo
}

TEST(memorytool, exit_call) {
	/// \todo
}

TEST(memorytool, init_parameter) {
	/// \todo
}

TEST(memorytool, find_function_signature) {
	/// \todo
}

TEST(memorytool, yield) {
	/// \todo
}

TEST(memorytool, load_default_result) {
	/// \todo
}

TEST(memorytool, get_symbol_reference) {
	/// \todo
}

TEST(memorytool, get_object_member) {
	/// \todo
}

TEST(memorytool, reduce_member) {
	/// \todo
}

TEST(memorytool, var_symbol) {
	/// \todo
}

TEST(memorytool, create_symbol) {
	/// \todo
}

TEST(memorytool, array_append_from_stack) {
	/// \todo
}

TEST(memorytool, array_append) {
	/// \todo
}

TEST(memorytool, array_get_item) {
	/// \todo
}

TEST(memorytool, array_index) {
	/// \todo
}

TEST(memorytool, hash_insert_from_stack) {
	/// \todo
}

TEST(memorytool, hash_insert) {
	/// \todo
}

TEST(memorytool, hash_get_item) {
	/// \todo
}

TEST(memorytool, hash_get_key) {
	/// \todo
}

TEST(memorytool, hash_get_value) {
	/// \todo
}

TEST(memorytool, iterator_yield) {
	/// \todo
}

TEST(memorytool, iterator_add) {
	/// \todo
}

TEST(memorytool, iterator_next) {

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

TEST(memorytool, regex_match) {
	/// \todo
}

TEST(memorytool, regex_unmatch) {
	/// \todo
}
