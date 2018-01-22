#ifndef DESTRUCTOR_H
#define DESTRUCTOR_H

#include "scheduler/process.h"
#include "memory/object.h"

namespace mint {

class MINT_EXPORT Destructor : public Process {
public:
	Destructor(Object *object);
	~Destructor();

private:
	Object *m_object;
};

}

#endif // DESTRUCTOR_H
