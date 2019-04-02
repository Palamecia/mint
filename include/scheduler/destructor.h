#ifndef DESTRUCTOR_H
#define DESTRUCTOR_H

#include "scheduler/process.h"
#include "memory/object.h"

namespace mint {

class MINT_EXPORT Destructor : public Process {
public:
	Destructor(Object *object, Process *process = nullptr);
	~Destructor() override;

	void setup() override;
	void cleanup() override;

private:
	Object *m_object;
};

MINT_EXPORT bool is_destructor(Process *process);

}

#endif // DESTRUCTOR_H
