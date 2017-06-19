#ifndef HASH_H
#define HASH_H

#include "memory/class.h"
#include "memory/object.h"

class AbstractSyntaxTree;

class HashClass : public Class {
public:
	static HashClass *instance();

private:
	HashClass();
};

struct Hash : public Object {
	Hash();
	typedef std::pair<SharedReference, AbstractSyntaxTree *> key_type;
	struct compare {
		bool operator ()(const key_type &a, const key_type &b) const;
	};
	typedef std::map<key_type, SharedReference, compare> values_type;
	values_type values;
};

#endif // HASH_H
