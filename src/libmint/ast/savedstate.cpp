#include "ast/savedstate.h"

using namespace mint;

SavedState::SavedState(Cursor::Context *context) :
	context(context) {
	context->executionMode = Cursor::resumed;
}

SavedState::~SavedState() {
	delete context;
}

void SavedState::setResumeMode(Cursor::ExecutionMode mode) {
	context->executionMode = mode;
}
