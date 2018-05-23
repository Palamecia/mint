#include <memory/functiontool.h>
#include <memory/operatortool.h>

#include <mutex>

using namespace mint;
using namespace std;

enum MutexType {
	normal,
	recursive
};

MINT_FUNCTION(mint_mutex_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference type = helper.popParameter();

	switch (static_cast<MutexType>(to_number(cursor, *type))) {
	case normal:
		helper.returnValue(create_object(new mutex));
		break;
	case recursive:
		helper.returnValue(create_object(new recursive_mutex));
		break;
	}
}

MINT_FUNCTION(mint_mutex_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference self = helper.popParameter();

	if (LibObject<mutex> *m = self->data<LibObject<mutex>>()) {
		delete m->impl;
	}
	else if (LibObject<recursive_mutex> *m = self->data<LibObject<recursive_mutex>>()) {
		delete m->impl;
	}
}

MINT_FUNCTION(mint_mutex_lock, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference self = helper.popParameter();

	if (LibObject<mutex> *m = self->data<LibObject<mutex>>()) {
		m->impl->lock();
	}
	else if (LibObject<recursive_mutex> *m = self->data<LibObject<recursive_mutex>>()) {
		m->impl->lock();
	}
}

MINT_FUNCTION(mint_mutex_unlock, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference self = helper.popParameter();

	if (LibObject<mutex> *m = self->data<LibObject<mutex>>()) {
		m->impl->unlock();
	}
	else if (LibObject<recursive_mutex> *m = self->data<LibObject<recursive_mutex>>()) {
		m->impl->unlock();
	}
}

MINT_FUNCTION(mint_mutex_try_lock, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference self = helper.popParameter();

	if (LibObject<mutex> *m = self->data<LibObject<mutex>>()) {
		helper.returnValue(create_boolean(m->impl->try_lock()));
	}
	else if (LibObject<recursive_mutex> *m = self->data<LibObject<recursive_mutex>>()) {
		helper.returnValue(create_boolean(m->impl->try_lock()));
	}
}
