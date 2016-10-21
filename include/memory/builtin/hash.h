#ifndef HASH_H
#define HASH_H

#include "memory/class.h"
#include "memory/object.h"

class HashClass : public Class {
public:
	static HashClass *instance();

private:
	HashClass();
};

struct Hash : public Object {
	Hash();
	struct compare {
		bool operator ()(const SharedReference &a, const SharedReference &b) const;
	};
	typedef std::map<SharedReference, SharedReference, compare> values_type;
	values_type values;
};

#endif // HASH_H
