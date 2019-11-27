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
	Hash(const Hash &other) = delete;
	~Hash();

	Hash &operator =(const Hash &other) = delete;

	using key_type = SharedReference;
	struct compare {
		bool operator ()(const key_type &lvalue, const key_type &rvalue) const;
	};
	using values_type = std::map<key_type, SharedReference, compare>;
	values_type values;
};

}

#endif // HASH_H
