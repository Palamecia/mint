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
	Hash(const Hash &other);

	Hash &operator =(const Hash &other) = delete;

	void mark() override;

	using key_type = SharedReference;
	struct MINT_EXPORT compare {
		bool operator ()(const key_type &lvalue, const key_type &rvalue) const;
	};
	using values_type = std::map<key_type, WeakReference, compare>;
	values_type values;

private:
	friend class Reference;
	static LocalPool<Hash> g_pool;
};

MINT_EXPORT void hash_insert_from_stack(Cursor *cursor);
MINT_EXPORT mint::Hash::values_type::iterator hash_insert(Hash *hash, const Hash::key_type &key, const SharedReference &value);
MINT_EXPORT SharedReference hash_get_item(Hash *hash, const Hash::key_type &key);
MINT_EXPORT SharedReference hash_get_key(Hash::values_type::iterator &it);
MINT_EXPORT SharedReference hash_get_key(Hash::values_type::value_type &item);
MINT_EXPORT SharedReference hash_get_value(Hash::values_type::iterator &it);
MINT_EXPORT SharedReference hash_get_value(Hash::values_type::value_type &item);
MINT_EXPORT Hash::key_type hash_key(const SharedReference &key);
MINT_EXPORT WeakReference hash_value(const SharedReference &value);
MINT_EXPORT WeakReference hash_value(const Reference &value);

}

#endif // HASH_H
