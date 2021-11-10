#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "scheduler/process.h"

namespace mint {

class MINT_EXPORT Exception : public Process {
public:
	Exception(Reference &&reference, Process *process);
	~Exception() override;

	void setup() override;
	void cleanup() override;

private:
	StrongReference m_reference;
	bool m_handled;
};

MINT_EXPORT bool is_exception(Process *process);

}

#endif // EXCEPTION_H
