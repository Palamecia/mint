#include <gtest/gtest.h>
#include <memory/memorytool.h>
#include <memory/functiontool.h>
#include <memory/builtin/string.h>
#include <memory/builtin/array.h>
#include <memory/builtin/hash.h>
#include <memory/builtin/iterator.h>
#include <ast/cursor.h>
#include <ast/abstractsyntaxtree.h>

using namespace mint;

TEST(memorytool, get_stack_base) {

	AbstractSyntaxTree ast;
	Cursor *cursor = ast.createCursor();

	cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
	cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
	cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
	EXPECT_EQ(2, get_stack_base(cursor));

	cursor->stack().pop_back();
	EXPECT_EQ(1, get_stack_base(cursor));

	delete cursor;
}

TEST(memorytool, type_name) {

	Reference *ref = nullptr;

	ref = Reference::create<None>();
	EXPECT_EQ("none", type_name(*ref));
	delete ref;

	ref = Reference::create<Null>();
	EXPECT_EQ("null", type_name(*ref));
	delete ref;

	ref = Reference::create<Number>();
	EXPECT_EQ("number", type_name(*ref));
	delete ref;

	ref = Reference::create<Boolean>();
	EXPECT_EQ("boolean", type_name(*ref));
	delete ref;

	ref = Reference::create<Function>();
	EXPECT_EQ("function", type_name(*ref));
	delete ref;

	ref = Reference::create<String>();
	EXPECT_EQ("string", type_name(*ref));
	delete ref;

	ref = Reference::create<Array>();
	EXPECT_EQ("array", type_name(*ref));
	delete ref;

	ref = Reference::create<Hash>();
	EXPECT_EQ("hash", type_name(*ref));
	delete ref;

	ref = Reference::create<Iterator>();
	EXPECT_EQ("iterator", type_name(*ref));
	delete ref;
}

TEST(memorytool, is_class) {

	Reference *ref = Reference::create<String>();
	EXPECT_TRUE(is_class(ref->data<String>()));

	ref->data<String>()->construct();
	EXPECT_FALSE(is_class(ref->data<String>()));

	delete ref;
}

TEST(memorytool, is_object) {

	Reference *ref = Reference::create<String>();
	EXPECT_FALSE(is_object(ref->data<String>()));

	ref->data<String>()->construct();
	EXPECT_TRUE(is_object(ref->data<String>()));

	delete ref;
}

TEST(memorytool, to_printer) {
	/// \todo
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

	SharedReference item(nullptr);
	Reference *it = Reference::create<Iterator>();
	iterator_insert(it->data<Iterator>(), create_number(0));
	iterator_insert(it->data<Iterator>(), create_number(1));

	ASSERT_TRUE(iterator_next(it->data<Iterator>(), item));
	ASSERT_EQ(Data::fmt_number, item->data()->format);
	EXPECT_EQ(0, item->data<Number>()->value);

	ASSERT_TRUE(iterator_next(it->data<Iterator>(), item));
	ASSERT_EQ(Data::fmt_number, item->data()->format);
	EXPECT_EQ(1, item->data<Number>()->value);

	EXPECT_FALSE(iterator_next(it->data<Iterator>(), item));
	delete it;
}

TEST(memorytool, regex_match) {
	/// \todo
}

TEST(memorytool, regex_unmatch) {
	/// \todo
}
