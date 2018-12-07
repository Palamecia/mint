#ifndef DESTRUCTOR_H
#define DESTRUCTOR_H

#include "scheduler/process.h"
#include "memory/object.h"

namespace mint {

class MINT_EXPORT Destructor : public Process {
public:
	Destructor(Object *object, Process *process);
	~Destructor();

	void setup() override;

private:
	Object *m_object;
};

MINT_EXPORT bool is_destructor(Process *process);

}

#endif // DESTRUCTOR_H
