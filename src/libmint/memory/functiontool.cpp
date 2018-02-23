#include "memory/functiontool.h"
#include "memory/builtin/string.h"
#include "ast/cursor.h"

#include <assert.h>

using namespace std;
using namespace mint;

FunctionHelper::FunctionHelper(Cursor *cursor, size_t argc) :
	m_cursor(cursor),
	m_valueReturned(false) {

	m_base = get_stack_base(cursor);
	m_top = m_base - argc;
}

FunctionHelper::~FunctionHelper() {

	if (!m_valueReturned) {
		returnValue(Reference::create<None>());
	}
}

SharedReference &FunctionHelper::popParameter() {

	assert(m_base > m_top);
	return m_cursor->stack().at(m_base--);
}

void FunctionHelper::returnValue(const SharedReference &value) {

	assert(!m_valueReturned);

	while (get_stack_base(m_cursor) > m_top) {
		m_cursor->stack().pop_back();
	}

	m_cursor->stack().push_back(value);
	m_valueReturned = true;
}

void FunctionHelper::returnValue(Reference *value) {
	returnValue(SharedReference::unique(value));
}

SharedReference mint::create_number(double value) {

	Reference *ref = Reference::create<Number>();
	ref->data<Number>()->value = value;
	return SharedReference::unique(ref);
}

SharedReference mint::create_boolean(bool value) {

	Reference *ref = Reference::create<Boolean>();
	ref->data<Boolean>()->value = value;
	return SharedReference::unique(ref);
}

SharedReference mint::create_string(const string &value) {

	Reference *ref = Reference::create<String>();
	ref->data<String>()->construct();
	ref->data<String>()->str = value;
	return SharedReference::unique(ref);
}

SharedReference mint::create_array(initializer_list<SharedReference> items) {

	Reference *ref = Reference::create<Array>();
	ref->data<Array>()->construct();
	for (auto i = items.begin(); i != items.end(); ++i) {
		array_append(ref->data<Array>(), *i);
	}
	return SharedReference::unique(ref);
}

SharedReference mint::create_hash(initializer_list<pair<SharedReference, SharedReference> > items) {

	Reference *ref = Reference::create<Hash>();
	ref->data<Hash>()->construct();
	for (auto i = items.begin(); i != items.end(); ++i) {
		hash_insert(ref->data<Hash>(), i->first, i->second);
	}
	return SharedReference::unique(ref);
}
