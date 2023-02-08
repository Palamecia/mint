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

#ifndef MINT_REFERENCE_H
#define MINT_REFERENCE_H

#include "mint/memory/data.h"
#include "mint/memory/memorypool.hpp"
#include "mint/memory/garbagecollector.h"

namespace mint {

struct Object;
struct ReferenceInfos;

class MINT_EXPORT Reference {
	friend class GarbageCollector;
public:
	using Flags = int;
	enum Flag : Flags {
		standard              = 0x00,
		const_value           = 0x01,
		const_address         = 0x02,
		private_visibility    = 0x04,
		protected_visibility  = 0x08,
		package_visibility    = 0x10,
		global                = 0x20,
		temporary             = 0x40,

		visibility_mask       = (Reference::private_visibility | Reference::protected_visibility | Reference::package_visibility)
	};

	virtual ~Reference();

	Reference &operator =(Reference &&other) noexcept;

	void copy(const Reference &other);
	void move(const Reference &other);

	template<class Type = Data>
	Type *data() const;

	inline Flags flags() const;

	ReferenceInfos *infos();

protected:
	explicit Reference(Flags flags = standard, Data *data = nullptr);
	explicit Reference(Reference &&other) noexcept;
	explicit Reference(ReferenceInfos *infos) noexcept;

	static GarbageCollector &g_garbage_collector;
	static LocalPool<ReferenceInfos> g_pool;

	void set_data(Data *data);

private:
	ReferenceInfos *m_infos;
};

struct ReferenceInfos {
	Reference::Flags flags = Reference::standard;
	Data *data = nullptr;
	size_t refcount = 0;
};

class MINT_EXPORT WeakReference : public Reference {
public:
	WeakReference(Flags flags = standard, Data *data = nullptr);
	WeakReference(WeakReference &&other) noexcept;
	WeakReference(Reference &&other) noexcept;
	~WeakReference();

	WeakReference &operator =(WeakReference &&other) noexcept;

	template<class Type, typename... Args>
	static WeakReference create(Args&&... args);
	static inline WeakReference create(Data *data);
	static inline WeakReference share(Reference &other);
	static inline WeakReference clone(Data *data);
	static inline WeakReference clone(const Reference &other);

protected:
	WeakReference(ReferenceInfos *infos);
};

class MINT_EXPORT StrongReference : public Reference, public MemoryRoot {
public:
	StrongReference(Flags flags = standard, Data *data = nullptr);
	StrongReference(StrongReference &&other) noexcept;
	StrongReference(WeakReference &&other) noexcept;
	StrongReference(Reference &&other) noexcept;
	~StrongReference();

	StrongReference &operator =(StrongReference &&other) noexcept;
	StrongReference &operator =(WeakReference &&other) noexcept;

	static inline StrongReference share(Reference &other);
	static inline StrongReference clone(const Reference &other);

protected:
	void mark() override {
		data()->mark();
	}

protected:
	StrongReference(ReferenceInfos *infos);
};

template<class Type>
Type *Reference::data() const {
	return static_cast<Type *>(m_infos->data);
}

Reference::Flags Reference::flags() const {
	return m_infos->flags;
}

template<class Type, typename... Args>
WeakReference WeakReference::create(Args&&... args) {
	return WeakReference(const_address | const_value | temporary, g_garbage_collector.alloc<Type>(std::forward<Args>(args)...));
}

WeakReference WeakReference::create(Data *data) {
	return WeakReference(const_address | const_value | temporary, data);
}

WeakReference WeakReference::share(Reference &other) {
	return WeakReference(other.infos());
}

WeakReference WeakReference::clone(Data *data) {
	return WeakReference(const_address | const_value | temporary, g_garbage_collector.copy(data));
}

WeakReference WeakReference::clone(const Reference &other) {
	return WeakReference(other.flags(), g_garbage_collector.copy(other.data()));
}

StrongReference StrongReference::share(Reference &other) {
	return StrongReference(other.infos());
}

StrongReference StrongReference::clone(const Reference &other) {
	return StrongReference(other.flags(), Reference::g_garbage_collector.copy(other.data()));
}

}

#endif // MINT_REFERENCE_H
