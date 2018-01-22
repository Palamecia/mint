#ifndef ITERATOR_H
#define ITERATOR_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class MINT_EXPORT IteratorClass : public Class {
public:
	static IteratorClass *instance();

private:
	IteratorClass();
};

struct MINT_EXPORT Iterator : public Object {
	Iterator();
	typedef std::deque<SharedReference> ctx_type;
	ctx_type ctx;
};

}

#endif // ITERATOR_H
