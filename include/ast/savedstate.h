#ifndef SAVED_STATE_H
#define SAVED_STATE_H

#include <ast/cursor.h>

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

#endif // SAVED_STATE_H
