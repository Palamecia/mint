/**
 * Copyright (c) 2026 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_MEMORY_BUILTIN_HASH_H
#define MINT_MEMORY_BUILTIN_HASH_H

#include "mint/config.h"
#include "mint/memory/class.h"
#include "mint/memory/garbagecollector.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include <cstddef>
#include <unordered_map>

namespace mint {

class AbstractSyntaxTree;
class Cursor;

class MINT_EXPORT HashClass : public Class {
public:
	HashClass(AbstractSyntaxTree& ast);
	static HashClass& instance(AbstractSyntaxTree& ast);
};

class MINT_EXPORT Hash : public Object {
	friend class GarbageCollector;
public:
	using key_type = WeakReference;
	using value_type = WeakReference;

	struct MINT_EXPORT hash {
		std::size_t operator()(const key_type& value) const;
	};

	struct MINT_EXPORT equal_to {
		bool operator()(const key_type& lhs, const key_type& rhs) const;
	};

	struct MINT_EXPORT compare_to {
		bool operator()(const key_type& lhs, const key_type& rhs) const;
	};

	using values_type = std::unordered_map<key_type, value_type, hash, equal_to>;

	explicit Hash(AbstractSyntaxTree& ast);
	Hash(Hash&& other) noexcept;
	Hash(const Hash& other);
	~Hash() override = default;

	Hash& operator=(Hash&& other) noexcept;
	Hash& operator=(const Hash& other);

	void mark() override;

	values_type values;

private:
	static LocalPool<Hash> g_pool;
};

MINT_EXPORT void hash_new(Cursor& cursor, std::size_t length);
MINT_EXPORT Hash::values_type::iterator hash_insert(Hash& hash, const Hash::key_type& key, const Reference& value);
MINT_EXPORT WeakReference hash_get_item(Hash& hash, const Hash::key_type& key);
MINT_EXPORT WeakReference hash_get_item(Hash& hash, Hash::key_type& key);
MINT_EXPORT WeakReference hash_get_key(const Hash::values_type::iterator& it);
MINT_EXPORT WeakReference hash_get_key(const Hash::values_type::value_type& item);
MINT_EXPORT WeakReference hash_get_value(const Hash::values_type::iterator& it);
MINT_EXPORT WeakReference hash_get_value(Hash::values_type::value_type& item);
MINT_EXPORT Hash::key_type hash_key(const Reference& key);
MINT_EXPORT WeakReference hash_value(const Reference& value);

}

#endif // MINT_MEMORY_BUILTIN_HASH_H
