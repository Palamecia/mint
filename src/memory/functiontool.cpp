#include "memory/functiontool.h"
#include "memory/builtin/string.h"
#include "ast/cursor.h"

#include <assert.h>

using namespace std;

FunctionHelper::FunctionHelper(Cursor *cursor, size_t argc) :
	m_cursor(cursor),
	m_valueReturned(false) {

	m_base = get_base(cursor);
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

	while (get_base(m_cursor) > m_top) {
		m_cursor->stack().pop_back();
	}

	m_cursor->stack().push_back(value);
	m_valueReturned = true;
}

void FunctionHelper::returnValue(Reference *value) {
	returnValue(SharedReference::unique(value));
}

SharedReference create_number(double value) {

	Reference *ref = Reference::create<Number>();
	((Number *)ref->data())->value = value;
	return SharedReference::unique(ref);
}

SharedReference create_boolean(bool value) {

	Reference *ref = Reference::create<Boolean>();
	((Boolean *)ref->data())->value = value;
	return SharedReference::unique(ref);
}

SharedReference create_string(const string &value) {

	Reference *ref = Reference::create<String>();
	((String *)ref->data())->construct();
	((String *)ref->data())->str = value;
	return SharedReference::unique(ref);
}
