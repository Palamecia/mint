#include "memory/functiontool.h"
#include "memory/builtin/string.h"
#include "system/assert.h"
#include "ast/cursor.h"

using namespace std;
using namespace mint;

ReferenceHelper::ReferenceHelper(const FunctionHelper *function, SharedReference &&reference) :
	m_function(function),
	m_reference(move(reference)) {

}

ReferenceHelper ReferenceHelper::operator [](const Symbol &symbol) const {
	return m_function->member(m_reference, symbol);
}

ReferenceHelper ReferenceHelper::member(const Symbol &symbol) const {
	return m_function->member(m_reference, symbol);
}

ReferenceHelper::operator SharedReference &() {
	return m_reference;
}

ReferenceHelper::operator SharedReference &&() {
	return move(m_reference);
}

Reference &ReferenceHelper::operator *() const {
	return m_reference.operator *();
}

Reference *ReferenceHelper::operator ->() const {
	return m_reference.operator ->();
}

Reference *ReferenceHelper::get() const {
	return m_reference.get();
}

FunctionHelper::FunctionHelper(Cursor *cursor, size_t argc) :
	m_cursor(cursor),
	m_valueReturned(false) {

	m_base = static_cast<ssize_t>(get_stack_base(cursor));
	m_top = m_base - static_cast<ssize_t>(argc);
}

FunctionHelper::~FunctionHelper() {

	if (!m_valueReturned) {
		returnValue(SharedReference::strong<None>());
	}
}

SharedReference &FunctionHelper::popParameter() {

	assert(m_base > m_top);
	return load_from_stack(m_cursor, static_cast<size_t>(m_base--));
}

ReferenceHelper FunctionHelper::reference(const Symbol &symbol) const {
	auto it = GlobalData::instance().symbols().find(symbol);
	if (it != GlobalData::instance().symbols().end()) {
		return ReferenceHelper(this, SharedReference::weak(it->second));
	}
	return ReferenceHelper(this, SharedReference::strong<None>());
}

ReferenceHelper FunctionHelper::member(const SharedReference &object, const Symbol &symbol) const {
	return ReferenceHelper(this, get_object_member(m_cursor, *object, symbol));
}

void FunctionHelper::returnValue(SharedReference &&value) {

	assert(m_valueReturned == false);

	while (static_cast<ssize_t>(get_stack_base(m_cursor)) > m_top) {
		m_cursor->stack().pop_back();
	}

	m_cursor->stack().emplace_back(move(value));
	m_valueReturned = true;
}

SharedReference mint::create_number(double value) {
	return SharedReference::strong<Number>(value);
}

SharedReference mint::create_boolean(bool value) {
	return SharedReference::strong<Boolean>(value);
}

SharedReference mint::create_string(const string &value) {
	SharedReference ref = SharedReference::strong<String>(value);
	ref->data<String>()->construct();
	return ref;
}

SharedReference mint::create_array(Array::values_type &&values) {

	SharedReference ref = SharedReference::strong<Array>();
	ref->data<Array>()->construct();
	ref->data<Array>()->values.swap(values);
	return ref;
}

SharedReference mint::create_array(initializer_list<SharedReference> items) {

	SharedReference ref = SharedReference::strong<Array>();
	ref->data<Array>()->construct();
	for (auto i = items.begin(); i != items.end(); ++i) {
		array_append(ref->data<Array>(), *i);
	}
	return ref;
}

SharedReference mint::create_hash(Hash::values_type &&values) {

	SharedReference ref = SharedReference::strong<Hash>();
	ref->data<Hash>()->construct();
	ref->data<Hash>()->values.swap(values);
	return ref;
}

SharedReference mint::create_hash(initializer_list<pair<SharedReference, SharedReference> > items) {

	SharedReference ref = SharedReference::strong<Hash>();
	ref->data<Hash>()->construct();
	for (auto i = items.begin(); i != items.end(); ++i) {
		hash_insert(ref->data<Hash>(), i->first, i->second);
	}
	return ref;
}

SharedReference mint::create_array() {

	SharedReference ref = SharedReference::strong<Array>();
	ref->data<Array>()->construct();
	return ref;
}

SharedReference mint::create_hash() {

	SharedReference ref = SharedReference::strong<Hash>();
	ref->data<Hash>()->construct();
	return ref;
}

SharedReference mint::create_iterator() {

	SharedReference ref = SharedReference::strong<Iterator>();
	ref->data<Iterator>()->construct();
	return ref;
}
