#include "memory/functiontool.h"
#include "memory/builtin/iterator.h"
#include "memory/builtin/string.h"
#include "memory/globaldata.h"
#include "system/assert.h"
#include "ast/cursor.h"

using namespace std;
using namespace mint;

ReferenceHelper::ReferenceHelper(const FunctionHelper *function, Reference &&reference) :
	m_function(function),
	m_reference(std::forward<Reference>(reference)) {

}

ReferenceHelper ReferenceHelper::operator [](const Symbol &symbol) const {
	return m_function->member(m_reference, symbol);
}

ReferenceHelper ReferenceHelper::member(const Symbol &symbol) const {
	return m_function->member(m_reference, symbol);
}

ReferenceHelper::operator Reference &() {
	return m_reference;
}

ReferenceHelper::operator Reference &&() {
	return std::move(m_reference);
}

const Reference &ReferenceHelper::operator *() const {
	return m_reference;
}

const Reference *ReferenceHelper::operator ->() const {
	return &m_reference;
}

const Reference *ReferenceHelper::get() const {
	return &m_reference;
}

FunctionHelper::FunctionHelper(Cursor *cursor, size_t argc) :
	m_cursor(cursor),
	m_valueReturned(false) {

	m_base = static_cast<ssize_t>(get_stack_base(cursor));
	m_top = m_base - static_cast<ssize_t>(argc);
}

FunctionHelper::~FunctionHelper() {

	if (!m_valueReturned) {
		returnValue(WeakReference::create<None>());
	}
}

Reference &FunctionHelper::popParameter() {

	assert(m_base > m_top);
	return load_from_stack(m_cursor, static_cast<size_t>(m_base--));
}

ReferenceHelper FunctionHelper::reference(const Symbol &symbol) const {
	GlobalData *globalData = GlobalData::instance();
	auto it = globalData->symbols().find(symbol);
	if (it != globalData->symbols().end()) {
		return ReferenceHelper(this, WeakReference::share(it->second));
	}
	return ReferenceHelper(this, WeakReference::create<None>());
}

ReferenceHelper FunctionHelper::member(const Reference &object, const Symbol &symbol) const {
	return ReferenceHelper(this, get_object_member(m_cursor, object, symbol));
}

void FunctionHelper::returnValue(Reference &&value) {

	assert(m_valueReturned == false);

	while (static_cast<ssize_t>(get_stack_base(m_cursor)) > m_top) {
		m_cursor->stack().pop_back();
	}

	m_cursor->stack().emplace_back(std::forward<Reference>(value));
	m_valueReturned = true;
}

WeakReference mint::create_number(double value) {
	return WeakReference::create<Number>(value);
}

WeakReference mint::create_boolean(bool value) {
	return WeakReference::create<Boolean>(value);
}

WeakReference mint::create_string(const string &value) {
	WeakReference ref = WeakReference::create<String>(value);
	ref.data<String>()->construct();
	return ref;
}

WeakReference mint::create_array(Array::values_type &&values) {

	WeakReference ref = WeakReference::create<Array>();
	ref.data<Array>()->construct();
	ref.data<Array>()->values.swap(values);
	return ref;
}

WeakReference mint::create_array(initializer_list<WeakReference> items) {

	WeakReference ref = WeakReference::create<Array>();
	ref.data<Array>()->construct();
	for (auto i = items.begin(); i != items.end(); ++i) {
		array_append(ref.data<Array>(), array_item(*i));
	}
	return ref;
}

WeakReference mint::create_hash(Hash::values_type &&values) {

	WeakReference ref = WeakReference::create<Hash>();
	ref.data<Hash>()->construct();
	ref.data<Hash>()->values.swap(values);
	return ref;
}

WeakReference mint::create_hash(initializer_list<pair<WeakReference, WeakReference> > items) {

	WeakReference ref = WeakReference::create<Hash>();
	ref.data<Hash>()->construct();
	for (auto i = items.begin(); i != items.end(); ++i) {
		hash_insert(ref.data<Hash>(), i->first, i->second);
	}
	return ref;
}

WeakReference mint::create_array() {

	WeakReference ref = WeakReference::create<Array>();
	ref.data<Array>()->construct();
	return ref;
}

WeakReference mint::create_hash() {

	WeakReference ref = WeakReference::create<Hash>();
	ref.data<Hash>()->construct();
	return ref;
}

WeakReference mint::create_iterator() {

	WeakReference ref = WeakReference::create<Iterator>();
	ref.data<Iterator>()->construct();
	return ref;
}

#ifdef OS_WINDOWS
WeakReference mint::create_handle(mint::handle_t handle) {
	WeakReference ref = WeakReference::create<LibObject<std::remove_pointer<HANDLE>::type>>();
	ref.data<LibObject<std::remove_pointer<HANDLE>::type>>()->construct();
	ref.data<LibObject<std::remove_pointer<HANDLE>::type>>()->impl = handle;
	return ref;
}

mint::handle_t mint::to_handle(const Reference &reference) {
	return reference.data<LibObject<std::remove_pointer<HANDLE>::type>>()->impl;
}

mint::handle_t *mint::to_handle_ptr(const Reference &reference) {
	return &reference.data<LibObject<std::remove_pointer<HANDLE>::type>>()->impl;
}
#else
WeakReference mint::create_handle(mint::handle_t handle) {
	WeakReference ref = WeakReference::create<LibObject<void>>();
	ref.data<LibObject<void>>()->construct();
	ref.data<LibObject<void>>()->impl = reinterpret_cast<void *>(handle);
	return ref;
}

mint::handle_t mint::to_handle(const Reference &reference) {
	return static_cast<handle_t>(reinterpret_cast<intptr_t>(reference.data<LibObject<void>>()->impl));
}

mint::handle_t *mint::to_handle_ptr(const Reference &reference) {
	return reinterpret_cast<handle_t *>(&reference.data<LibObject<void>>()->impl);
}
#endif
