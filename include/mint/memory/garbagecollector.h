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

#ifndef MINT_GARBAGECOLLECTOR_H
#define MINT_GARBAGECOLLECTOR_H

#include "mint/config.h"
#include "mint/memory/data.h"

#include <cstddef>
#include <vector>
#include <set>

namespace mint {

class WeakReference;
class MemoryRoot;
struct Object;

class MINT_EXPORT GarbageCollector {
	friend struct Data;
	friend class MemoryRoot;
	friend class Destructor;
	friend class Reference;
	friend class WeakReference;
	friend class StrongReference;
public:
	static GarbageCollector &instance();

	template<class Type, typename... Args>
	Type *alloc(Args &&...args);

	size_t collect();
	void clean();

	inline void use(Data *data);
	inline void release(Data *data);

	std::vector<WeakReference> *create_stack();
	void remove_stack(std::vector<WeakReference> *stack);

protected:
	void register_data(Data *data);
	void unregister_data(Data *data);
	void register_root(MemoryRoot *reference);
	void unregister_root(MemoryRoot *reference);

	Data *copy(const Data *other);
	void free(Data *ptr);
	void destroy(Data *ptr);
	void destroy(Object *ptr);

private:
	GarbageCollector();
	GarbageCollector(const GarbageCollector &other) = delete;
	GarbageCollector &operator=(const GarbageCollector &othet) = delete;
	~GarbageCollector();

	std::set<std::vector<WeakReference> *> m_stacks;

	struct {
		MemoryRoot *head = nullptr;
		MemoryRoot *tail = nullptr;
	} m_roots;

	struct {
		Data *head = nullptr;
		Data *tail = nullptr;
	} m_memory;
};

class MINT_EXPORT MemoryRoot {
	friend class GarbageCollector;
public:
	MemoryRoot();
	virtual ~MemoryRoot();

protected:
	static GarbageCollector &g_garbage_collector;
	virtual void mark() = 0;

private:
	MemoryRoot *prev = nullptr;
	MemoryRoot *next = nullptr;
};

template<class Type, typename... Args>
Type *GarbageCollector::alloc(Args &&...args) {
	return Type::g_pool.alloc(std::forward<Args>(args)...);
}

template<>
MINT_EXPORT None *GarbageCollector::alloc<None>();

template<>
MINT_EXPORT Null *GarbageCollector::alloc<Null>();

void GarbageCollector::use(Data *data) {
	++data->infos.refcount;
}

void GarbageCollector::release(Data *data) {
	if (!data->infos.collected && !--data->infos.refcount) {
		data->infos.collected = true;
		unregister_data(data);
		GarbageCollector::free(data);
	}
}

}

#endif // MINT_GARBAGECOLLECTOR_H
