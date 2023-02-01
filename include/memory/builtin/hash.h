#ifndef MINT_HASH_H
#define MINT_HASH_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class Cursor;

class MINT_EXPORT HashClass : public Class {
public:
	static HashClass *instance();

private:
	friend class GlobalData;
	HashClass();
};

struct MINT_EXPORT Hash : public Object {
	Hash();
	Hash(const Hash &other);

	Hash &operator =(const Hash &other) = delete;

	void mark() override;

	using key_type = WeakReference;
	using value_type = WeakReference;
	struct MINT_EXPORT hash {
		size_t operator ()(const key_type &value) const;
	};
	struct MINT_EXPORT equal_to {
		bool operator ()(const key_type &lvalue, const key_type &rvalue) const;
	};
	struct MINT_EXPORT compare_to {
		bool operator ()(const key_type &lvalue, const key_type &rvalue) const;
	};
	using values_type = std::unordered_map<key_type, value_type, hash, equal_to>;
	values_type values;

private:
	friend class Reference;
	static LocalPool<Hash> g_pool;
};

MINT_EXPORT void hash_new(Cursor *cursor, size_t length);
MINT_EXPORT mint::Hash::values_type::iterator hash_insert(Hash *hash, const Hash::key_type &key, const Reference &value);
MINT_EXPORT WeakReference hash_get_item(Hash *hash, const Hash::key_type &key);
MINT_EXPORT WeakReference hash_get_item(Hash *hash, Hash::key_type &key);
MINT_EXPORT WeakReference hash_get_key(Hash::values_type::iterator &it);
MINT_EXPORT WeakReference hash_get_key(Hash::values_type::value_type &item);
MINT_EXPORT WeakReference hash_get_value(Hash::values_type::iterator &it);
MINT_EXPORT WeakReference hash_get_value(Hash::values_type::value_type &item);
MINT_EXPORT Hash::key_type hash_key(const Reference &key);
MINT_EXPORT WeakReference hash_value(const Reference &value);

}

#endif // MINT_HASH_H
