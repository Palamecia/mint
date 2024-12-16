/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#ifndef MINT_OBJECT_H
#define MINT_OBJECT_H

#include "mint/ast/module.h"
#include "mint/ast/symbolmapping.hpp"
#include "mint/memory/data.h"
#include "mint/memory/reference.h"
#include "mint/memory/memorypool.hpp"

#include <unordered_map>
#include <string>

namespace mint {

class Class;
class PackageData;

struct MINT_EXPORT Number : public Data {
	double value;

protected:
	template<typename Type> friend class LocalPool;
	friend class GarbageCollector;

	Number() = delete;
	explicit Number(double value);
	Number(const Number &other);

private:
	static LocalPool<Number> g_pool;
};

struct MINT_EXPORT Boolean : public Data {
	bool value;

protected:
	template<typename Type> friend class LocalPool;
	friend class GarbageCollector;

	Boolean() = delete;
	explicit Boolean(bool value);
	Boolean(const Boolean &other);

private:
	static LocalPool<Boolean> g_pool;
};

struct MINT_EXPORT Object : public Data {
	Class *const metadata;
	WeakReference *data;

	void construct();
	void construct(const Object &other);

	void mark() override;

protected:
	template<typename Type> friend class LocalPool;
	friend class GarbageCollector;

	explicit Object(Class *type);
	Object(const Object &other) = delete;
	~Object() override;

private:
	void construct(const Object &other, std::unordered_map<const Data *, Data *> &memory_map);

	static LocalPool<Object> g_pool;
};

struct MINT_EXPORT Package : public Data {
	PackageData *const data;

protected:
	template<typename Type> friend class LocalPool;
	friend class GarbageCollector;

	explicit Package(PackageData *package);

private:
	static LocalPool<Package> g_pool;
};

struct MINT_EXPORT Function : public Data {
	using Capture = SymbolMapping<WeakReference>;

	struct MINT_EXPORT Signature {
		Signature(Module::Handle *handle, bool capture = false);
		Signature(Signature &&other) noexcept;
		Signature(const Signature &other);
		~Signature();

		Module::Handle *const handle;
		Capture *capture;
	};

	class MINT_EXPORT mapping_type {
	public:
		using iterator = std::map<int, Signature>::iterator;
		using const_iterator = std::map<int, Signature>::const_iterator;

		mapping_type();
		mapping_type(mapping_type &&other) noexcept;
		mapping_type(const mapping_type &other);
		~mapping_type();

		mapping_type &operator =(mapping_type &&other) noexcept;
		mapping_type &operator =(const mapping_type &other);

		bool operator ==(const mapping_type &other) const;
		bool operator !=(const mapping_type &other) const;

		std::pair<iterator, bool> emplace(int signature, const Signature &handle);
		std::pair<iterator, bool> insert(const std::pair<int, Signature> &signature);

		iterator lower_bound(int signature) const;
		iterator find(int signature) const;

		const_iterator cbegin() const;
		const_iterator begin() const;
		iterator begin();

		const_iterator cend() const;
		const_iterator end() const;
		iterator end();

		bool empty() const;

	private:
		struct shared_data_t {
			std::map<int, Signature> signatures;
			size_t refcount = 1;
			bool sharable = true;

			shared_data_t(const std::map<int, Signature> &signatures, bool sharable) :
				signatures(signatures),
				sharable(sharable) {

			}

			shared_data_t() = default;

			inline bool is_sharable() const{
				return sharable;
			}

			inline bool is_shared() const {
				return refcount > 1;
			}

			inline shared_data_t *share() {
				++refcount;
				return this;
			}

			inline shared_data_t *detach() const {
				return new shared_data_t(signatures, sharable);
			}
		};
		shared_data_t *m_data;
	};

	mapping_type mapping;

	void mark() override;

protected:
	template<typename Type> friend class LocalPool;
	friend class GarbageCollector;

	Function();
	Function(const Function &other);

private:
	static LocalPool<Function> g_pool;
};

}

#endif // MINT_OBJECT_H
