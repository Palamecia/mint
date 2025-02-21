/**
 * Copyright (c) 2025 Gauvain CHERY.
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

namespace mint {

class Class;
class PackageData;

struct MINT_EXPORT Number : public Data {
	template<typename Type>
	friend class LocalPool;
	friend class GarbageCollector;
public:
	Number() = delete;
	Number(Number &&) = delete;

	Number &operator=(Number &&) = delete;
	Number &operator=(const Number &) = delete;

	double value;

protected:
	explicit Number(double value);
	Number(const Number &other);
	~Number() override = default;

private:
	static LocalPool<Number> g_pool;
};

struct MINT_EXPORT Boolean : public Data {
	template<typename Type>
	friend class LocalPool;
	friend class GarbageCollector;
public:
	Boolean() = delete;
	Boolean(Boolean &&) = delete;

	Boolean &operator=(Boolean &&) = delete;
	Boolean &operator=(const Boolean &) = delete;

	bool value;

protected:
	explicit Boolean(bool value);
	Boolean(const Boolean &other);
	~Boolean() override = default;

private:
	static LocalPool<Boolean> g_pool;
};

struct MINT_EXPORT Object : public Data {
	template<typename Type>
	friend class LocalPool;
	friend class GarbageCollector;
public:
	Object(Object &&) = delete;
	Object(const Object &) = delete;

	Object &operator=(Object &&) = delete;
	Object &operator=(const Object &) = delete;

	Class *const metadata;
	WeakReference *data;

	void construct();
	void construct(const Object &other);

	void mark() override;

protected:
	explicit Object(Class *type);
	~Object() override;

private:
	void construct(const Object &other, std::unordered_map<const Data *, Data *> &memory_map);

	static LocalPool<Object> g_pool;
};

struct MINT_EXPORT Package : public Data {
	template<typename Type>
	friend class LocalPool;
	friend class GarbageCollector;
public:
	PackageData *const data;

protected:
	explicit Package(PackageData *package);

private:
	static LocalPool<Package> g_pool;
};

struct MINT_EXPORT Function : public Data {
	template<typename Type>
	friend class LocalPool;
	friend class GarbageCollector;
public:
	using Capture = SymbolMapping<WeakReference>;

	struct MINT_EXPORT Signature {
		Signature(Module::Handle *handle, bool capture = false);
		Signature(Signature &&other) noexcept;
		Signature(const Signature &other);
		~Signature();

		Signature &operator=(Signature &&) = delete;
		Signature &operator=(const Signature &) = delete;

		Module::Handle *const handle;
		Capture *capture;
	};

	class MINT_EXPORT Mapping {
	public:
		using iterator = std::map<int, Signature>::iterator;
		using const_iterator = std::map<int, Signature>::const_iterator;

		Mapping();
		Mapping(Mapping &&other) noexcept;
		Mapping(const Mapping &other);
		~Mapping();

		Mapping &operator=(Mapping &&other) noexcept;
		Mapping &operator=(const Mapping &other);

		bool operator==(const Mapping &other) const;
		bool operator!=(const Mapping &other) const;

		std::pair<iterator, bool> emplace(int signature, const Signature &handle);
		std::pair<iterator, bool> insert(const std::pair<int, Signature> &signature);

		[[nodiscard]] iterator lower_bound(int signature) const;
		[[nodiscard]] iterator find(int signature) const;

		[[nodiscard]] const_iterator cbegin() const;
		[[nodiscard]] const_iterator begin() const;
		iterator begin();

		[[nodiscard]] const_iterator cend() const;
		[[nodiscard]] const_iterator end() const;
		iterator end();

		[[nodiscard]] bool empty() const;

	private:
		struct SharedData {
			std::map<int, Signature> signatures;
			size_t refcount = 1;
			bool sharable = true;

			SharedData(const std::map<int, Signature> &signatures, bool sharable) :
				signatures(signatures),
				sharable(sharable) {}

			SharedData() = default;

			[[nodiscard]] bool is_sharable() const {
				return sharable;
			}

			[[nodiscard]] bool is_shared() const {
				return refcount > 1;
			}

			SharedData *share() {
				++refcount;
				return this;
			}

			[[nodiscard]] SharedData *detach() const {
				return new SharedData(signatures, sharable);
			}
		};

		SharedData *m_data;
	};

	Function(Function &&) = delete;

	Function &operator=(Function &&) = delete;
	Function &operator=(const Function &) = delete;

	Mapping mapping;

	void mark() override;

protected:
	Function();
	Function(const Function &other);
	~Function() override = default;

private:
	static LocalPool<Function> g_pool;
};

}

#endif // MINT_OBJECT_H
