#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "scheduler/process.h"

namespace mint {

class Exception : public Process {
public:
	Exception(SharedReference reference, Process *process);

	void setup() override;
	void cleanup() override;

private:
	SharedReference m_reference;
	bool m_handled;
};

MINT_EXPORT bool is_exception(Process *process);

}

#endif // EXCEPTION_H
