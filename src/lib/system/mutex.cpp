#include <memory/functiontool.h>
#include <memory/operatortool.h>
#include <scheduler/processor.h>

#include <mutex>

using namespace mint;
using namespace std;

struct AbstractMutex {
	enum Type {
		normal,
		recursive
	};

	virtual ~AbstractMutex() = default;
	virtual Type type() const = 0;
};

struct Mutex : public AbstractMutex {
	Type type() const override {
		return normal;
	}
	std::mutex handle;
};

struct RecursiveMutex : public AbstractMutex {
	Type type() const override {
		return recursive;
	}
	std::recursive_mutex handle;
};

MINT_FUNCTION(mint_mutex_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference type = move(helper.popParameter());

	switch (static_cast<AbstractMutex::Type>(static_cast<int>(to_number(cursor, type)))) {
	case AbstractMutex::normal:
		helper.returnValue(create_object(new Mutex));
		break;
	case AbstractMutex::recursive:
		helper.returnValue(create_object(new RecursiveMutex));
		break;
	}
}

MINT_FUNCTION(mint_mutex_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference self = move(helper.popParameter());

	delete self->data<LibObject<AbstractMutex>>()->impl;
}

MINT_FUNCTION(mint_mutex_lock, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference self = move(helper.popParameter());

	unlock_processor();

	switch (self->data<LibObject<AbstractMutex>>()->impl->type()) {
	case AbstractMutex::normal:
		self->data<LibObject<Mutex>>()->impl->handle.lock();
		break;
	case AbstractMutex::recursive:
		self->data<LibObject<RecursiveMutex>>()->impl->handle.lock();
		break;
	}

	lock_processor();
}

MINT_FUNCTION(mint_mutex_unlock, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference self = move(helper.popParameter());

	switch (self->data<LibObject<AbstractMutex>>()->impl->type()) {
	case AbstractMutex::normal:
		self->data<LibObject<Mutex>>()->impl->handle.unlock();
		break;
	case AbstractMutex::recursive:
		self->data<LibObject<RecursiveMutex>>()->impl->handle.unlock();
		break;
	}
}

MINT_FUNCTION(mint_mutex_try_lock, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference self = move(helper.popParameter());

	switch (self->data<LibObject<AbstractMutex>>()->impl->type()) {
	case AbstractMutex::normal:
		helper.returnValue(create_boolean(self->data<LibObject<Mutex>>()->impl->handle.try_lock()));
		break;
	case AbstractMutex::recursive:
		helper.returnValue(create_boolean(self->data<LibObject<RecursiveMutex>>()->impl->handle.try_lock()));
		break;
	}
}
