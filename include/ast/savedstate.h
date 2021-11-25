#ifndef MINT_SAVEDSTATE_H
#define MINT_SAVEDSTATE_H

#include "ast/cursor.h"

#include <stack>

namespace mint {

struct MINT_EXPORT SavedState {
	Cursor::Context *context;
	std::stack<Cursor::RetrievePoint> retrievePoints;

	SavedState(Cursor::Context *context);
	~SavedState();

	void setResumeMode(Cursor::ExecutionMode mode);
};

}

#endif // MINT_SAVEDSTATE_H
