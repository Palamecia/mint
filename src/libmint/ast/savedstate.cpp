#include "ast/savedstate.h"
#include "ast/cursor.h"

using namespace mint;

SavedState::SavedState(Cursor *cursor, Cursor::Context *context) :
	cursor(cursor),
	context(context) {
	context->executionMode = Cursor::resumed;
}

SavedState::~SavedState() {
	cursor->destroy(this);
}

void SavedState::setResumeMode(Cursor::ExecutionMode mode) {
	context->executionMode = mode;
}
