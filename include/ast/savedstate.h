#ifndef MINT_SAVEDSTATE_H
#define MINT_SAVEDSTATE_H

#include "ast/cursor.h"

#include <stack>

namespace mint {

struct MINT_EXPORT SavedState {
	SavedState(Cursor *cursor, Cursor::Context *context);
	~SavedState();

	Cursor *cursor;
	Cursor::Context *context;
	std::stack<Cursor::RetrievePoint> retrievePoints;
};

}

#endif // MINT_SAVEDSTATE_H
