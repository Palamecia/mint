#ifndef HASH_H
#define HASH_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class Cursor;

class MINT_EXPORT HashClass : public Class {
public:
	static HashClass *instance();

private:
	HashClass();
};

struct MINT_EXPORT Hash : public Object {
	Hash();
	~Hash();

	typedef SharedReference key_type;
	struct compare {
		bool operator ()(const key_type &lvalue, const key_type &rvalue) const;
	};
	typedef std::map<key_type, SharedReference, compare> values_type;
	values_type values;
};

}

#endif // HASH_H
