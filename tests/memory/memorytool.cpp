#include <gtest/gtest.h>
#include <memory/memorytool.h>
#include <memory/functiontool.h>
#include <memory/objectprinter.h>
#include <memory/builtin/string.h>
#include <memory/builtin/regex.h>
#include <memory/builtin/array.h>
#include <memory/builtin/hash.h>
#include <memory/builtin/iterator.h>
#include <system/fileprinter.h>
#include <ast/abstractsyntaxtree.h>
#include <ast/cursor.h>

using namespace mint;

static Class g_test_class("test");

TEST(memorytool, get_stack_base) {

	Cursor *cursor = AbstractSyntaxTree::instance().createCursor();

	cursor->stack().emplace_back(WeakReference::create<None>());
	cursor->stack().emplace_back(WeakReference::create<None>());
	cursor->stack().emplace_back(WeakReference::create<None>());
	EXPECT_EQ(2, get_stack_base(cursor));

	cursor->stack().pop_back();
	EXPECT_EQ(1, get_stack_base(cursor));

	delete cursor;
}

TEST(memorytool, type_name) {

	WeakReference ref;

	ref = WeakReference::create<None>();
	EXPECT_EQ("none", type_name(ref));

	ref = WeakReference::create<Null>();
	EXPECT_EQ("null", type_name(ref));

	ref = WeakReference::create<Number>();
	EXPECT_EQ("number", type_name(ref));

	ref = WeakReference::create<Boolean>();
	EXPECT_EQ("boolean", type_name(ref));

	ref = WeakReference::create<Function>();
	EXPECT_EQ("function", type_name(ref));

	ref = WeakReference::create<String>();
	EXPECT_EQ("string", type_name(ref));

	ref = WeakReference::create<Regex>();
	EXPECT_EQ("regex", type_name(ref));

	ref = WeakReference::create<Array>();
	EXPECT_EQ("array", type_name(ref));

	ref = WeakReference::create<Hash>();
	EXPECT_EQ("hash", type_name(ref));

	ref = WeakReference::create<Iterator>();
	EXPECT_EQ("iterator", type_name(ref));
}

TEST(memorytool, is_class) {

	WeakReference ref = WeakReference::create<String>();
	EXPECT_TRUE(is_class(ref.data<String>()));

	ref.data<String>()->construct();
	EXPECT_FALSE(is_class(ref.data<String>()));
}

TEST(memorytool, is_object) {

	WeakReference ref = WeakReference::create<String>();
	EXPECT_FALSE(is_object(ref.data<String>()));

	ref.data<String>()->construct();
	EXPECT_TRUE(is_object(ref.data<String>()));
}

TEST(memorytool, create_printer) {

	Cursor *cursor = AbstractSyntaxTree::instance().createCursor();
	Printer *printer = nullptr;

	cursor->stack().emplace_back(create_number(0));
	printer = create_printer(cursor);
	EXPECT_NE(nullptr, dynamic_cast<FilePrinter *>(printer));
	delete printer;

	cursor->stack().emplace_back(create_string("test"));
	printer = create_printer(cursor);
	EXPECT_NE(nullptr, dynamic_cast<FilePrinter *>(printer));
	delete printer;

	cursor->stack().emplace_back(WeakReference(Reference::standard, Reference::alloc<Object>(&g_test_class)));
	printer = create_printer(cursor);
	EXPECT_NE(nullptr, dynamic_cast<ObjectPrinter *>(printer));
	delete printer;
	delete cursor;
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

TEST(memorytool, iterator_init) {
	/// \todo
}

TEST(memorytool, iterator_insert) {
	/// \todo
}

TEST(memorytool, iterator_add) {
	/// \todo
}

TEST(memorytool, iterator_next) {

	WeakReference item;
	WeakReference it = WeakReference::create<Iterator>();
	iterator_insert(it.data<Iterator>(), create_number(0));
	iterator_insert(it.data<Iterator>(), create_number(1));

	ASSERT_TRUE(iterator_get(it.data<Iterator>()));
	item = std::move(*iterator_next(it.data<Iterator>()));
	ASSERT_EQ(Data::fmt_number, item.data()->format);
	EXPECT_EQ(0., item.data<Number>()->value);

	ASSERT_TRUE(iterator_get(it.data<Iterator>()));
	item = std::move(*iterator_next(it.data<Iterator>()));
	ASSERT_EQ(Data::fmt_number, item.data()->format);
	EXPECT_EQ(1., item.data<Number>()->value);

	EXPECT_FALSE(iterator_next(it.data<Iterator>()));
}

TEST(memorytool, regex_match) {
	/// \todo
}

TEST(memorytool, regex_unmatch) {
	/// \todo
}
