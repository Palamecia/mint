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

#ifndef MINT_MEMORY_GARBAGECOLLECTOR_H
#define MINT_MEMORY_GARBAGECOLLECTOR_H

#include "mint/config.h"
#include "mint/memory/data.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <vector>

namespace mint {

class MemoryRoot;
class Object;
class Reference;
class StrongReference;
class WeakReference;

class MINT_EXPORT GarbageCollector {
	friend class Data;
	friend class MemoryRoot;
	friend class Destructor;
	friend class Reference;
	friend class WeakReference;
	friend class StrongReference;
public:
	GarbageCollector(GarbageCollector&& other) = delete;
	GarbageCollector(const GarbageCollector& other) = delete;

	GarbageCollector& operator=(GarbageCollector&& other) = delete;
	GarbageCollector& operator=(const GarbageCollector& other) = delete;

	static constexpr std::size_t default_threshold = 7000;
	static constexpr std::size_t threshold_growth_factor = 500;

	static GarbageCollector& instance();

	template<class Type, typename... Args>
	    requires std::constructible_from<Type, Args...>
	Type* alloc(Args&&... args);

	std::size_t collect();
	void clean();

	[[nodiscard]] bool is_deferred() const noexcept;
	void defer();
	void resume();

	inline void use(Data* data);
	inline void release(Data* data);

	Reference& none_ref();
	Reference& null_ref();

	[[nodiscard]] inline bool is_threshold_exceeded() const;
	[[nodiscard]] std::size_t get_threshold() const;
	void set_threshold(std::size_t threshold);

	[[nodiscard]] static std::size_t get_refcount(const Data& data);
	[[nodiscard]] std::size_t get_count() const;

protected:
	void register_data(Data* data);
	void unregister_data(Data* data);
	void register_root(MemoryRoot* root);
	void unregister_root(MemoryRoot* root);

	Data* copy(const Data& other);
	void free(Data* ptr);
	void destroy(Data* ptr);
	void destroy(Object* ptr);

private:
	GarbageCollector();
	~GarbageCollector();

	std::vector<Data*> _collecting;
	std::size_t _defer_depth = 0;
	bool _pending_collection = false;

	std::unique_ptr<StrongReference> _none;
	std::unique_ptr<StrongReference> _null;

	struct {
		MemoryRoot* head = nullptr;
		MemoryRoot* tail = nullptr;
	} _roots;

	struct {
		Data* head = nullptr;
		Data* tail = nullptr;
	} _memory;

	std::size_t _threshold = default_threshold;
	std::size_t _count = 0;

	template<typename List, typename Node>
	static constexpr void gc_list_insert_element(List& list, Node* node) {
		if (list.tail) {
			list.tail->_next = node;
			node->_prev = list.tail;
			list.tail = node;
		}
		else {
			list.head = list.tail = node;
		}
	}

	template<typename List, typename Node>
	static constexpr void gc_list_remove_element(List& list, Node* node) {
		if (node->_prev) {
			node->_prev->_next = node->_next;
		}
		else {
			list.head = node->_next;
		}
		if (node->_next) {
			node->_next->_prev = node->_prev;
		}
		else {
			list.tail = node->_prev;
		}
	}
};

class MINT_EXPORT GarbageCollectorDeferScope {
public:
	GarbageCollectorDeferScope() {
		GarbageCollector::instance().defer();
	}

	GarbageCollectorDeferScope(const GarbageCollectorDeferScope&) = delete;
	GarbageCollectorDeferScope(GarbageCollectorDeferScope&&) = delete;

	~GarbageCollectorDeferScope() {
		GarbageCollector::instance().resume();
	}

	GarbageCollectorDeferScope& operator=(const GarbageCollectorDeferScope&) = delete;
	GarbageCollectorDeferScope& operator=(GarbageCollectorDeferScope&&) = delete;
};

class MINT_EXPORT MemoryRoot {
	friend class GarbageCollector;
public:
	MemoryRoot();
	MemoryRoot(MemoryRoot&& other) noexcept;
	MemoryRoot(const MemoryRoot& other);
	virtual ~MemoryRoot();

	MemoryRoot& operator=(MemoryRoot&&) noexcept;
	MemoryRoot& operator=(const MemoryRoot& other);

	virtual void mark() = 0;

protected:
	void register_root();
	void unregister_root();

private:
	MemoryRoot* _prev = nullptr;
	MemoryRoot* _next = nullptr;
#ifdef MINT_BUILD_TYPE_DEBUG
	bool _registered = false;
#endif
};

template<class Type, typename... Args>
    requires std::constructible_from<Type, Args...>
Type* GarbageCollector::alloc(Args&&... args) {
	return Type::g_pool.alloc(std::forward<Args>(args)...);
}

template<>
MINT_EXPORT None* GarbageCollector::alloc<None>();

template<>
MINT_EXPORT Null* GarbageCollector::alloc<Null>();

void GarbageCollector::use(Data* data) {
	++data->_info.refcount;
}

void GarbageCollector::release(Data* data) {
	if (std::ranges::find(_collecting, data) != _collecting.end()) [[unlikely]] {
		return;
	}
	if (!data->_info.collected && !--data->_info.refcount) {
		data->_info.collected = true;
		unregister_data(data);
		GarbageCollector::free(data);
	}
}

bool GarbageCollector::is_threshold_exceeded() const {
	return _threshold < _count;
}

}

#endif // MINT_MEMORY_GARBAGECOLLECTOR_H
