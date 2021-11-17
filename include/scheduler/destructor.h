#ifndef DESTRUCTOR_H
#define DESTRUCTOR_H

#include "scheduler/process.h"
#include "memory/object.h"

namespace mint {

class MINT_EXPORT Destructor : public Process {
public:
	Destructor(Object *object, Reference &&member, Class *owner, Process *process = nullptr);
	~Destructor() override;

	void setup() override;
	void cleanup() override;
	bool collectOnExit() const override;

private:
	Class *m_owner;
	Object *m_object;
	StrongReference m_member;
};

MINT_EXPORT bool is_destructor(Process *process);

}

#endif // DESTRUCTOR_H
