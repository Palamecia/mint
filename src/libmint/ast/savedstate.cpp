#include "ast/savedstate.h"
#include "ast/cursor.h"

using namespace mint;

SavedState::SavedState(Cursor *cursor, Cursor::Context *context) :
	cursor(cursor),
	context(context) {

}

SavedState::~SavedState() {
	cursor->destroy(this);
}
