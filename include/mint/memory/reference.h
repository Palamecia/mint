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
#include <type_traits>

namespace mint {

struct Object;
struct ReferenceInfo;

class MINT_EXPORT Reference {
	friend class GarbageCollector;
public:
	enum Flag : std::uint16_t {
		DEFAULT = 0x000,
		CONST_VALUE = 0x001,
		CONST_ADDRESS = 0x002,
		PRIVATE_VISIBILITY = 0x004,
		PROTECTED_VISIBILITY = 0x008,
		PACKAGE_VISIBILITY = 0x010,
		GLOBAL = 0x020,
		TEMPORARY = 0x040,
		FINAL_MEMBER = 0x080,
		OVERRIDE_MEMBER = 0x100,

		VISIBILITY_MASK = (PRIVATE_VISIBILITY | PROTECTED_VISIBILITY | PACKAGE_VISIBILITY)
	};

	using Flags = std::underlying_type_t<Flag>;

	struct Info {
		Flags flags = DEFAULT;
		Data *data = nullptr;
		size_t refcount = 0;
	};

	Reference(const Reference &) = delete;
	virtual ~Reference();

	Reference &operator=(Reference &&other) noexcept;
	Reference &operator=(const Reference &) = delete;

	void copy_data(const Reference &other);
	void move_data(const Reference &other);

	template<class Type = Data, typename = std::enable_if_t<std::is_base_of_v<Data, Type>>>
	[[nodiscard]] Type *data() const;

	[[nodiscard]] inline Flags flags() const;

	Info *info();

protected:
	explicit Reference(Flags flags = DEFAULT, Data *data = nullptr);
	explicit Reference(Reference &&other) noexcept;
	explicit Reference(Info *infos) noexcept;

	static GarbageCollector &g_garbage_collector;
	static LocalPool<Info> g_pool;

	void set_data(Data *data);

private:
	Info *m_info;
};

class MINT_EXPORT WeakReference final : public Reference {
public:
	WeakReference(Flags flags = DEFAULT, Data *data = nullptr);
	WeakReference(WeakReference &&other) noexcept;
	WeakReference(const WeakReference &) = delete;
	WeakReference(Reference &&other) noexcept;
	~WeakReference() override;

	WeakReference &operator=(WeakReference &&other) noexcept;
	WeakReference &operator=(const WeakReference &) = delete;

	template<class Type, typename... Args>
	static WeakReference create(Args &&...args);
	static inline WeakReference create(Data *data);
	static inline WeakReference share(Reference &other);
	static inline WeakReference copy(const Reference &other);
	static inline WeakReference clone(const Data *data);
	static inline WeakReference clone(const Reference &other);

protected:
	explicit WeakReference(Info *infos);
};

class MINT_EXPORT StrongReference final : public Reference, public MemoryRoot {
public:
	StrongReference(Flags flags = DEFAULT, Data *data = nullptr);
	StrongReference(StrongReference &&other) noexcept;
	StrongReference(const StrongReference &) = delete;
	StrongReference(WeakReference &&other) noexcept;
	StrongReference(Reference &&other) noexcept;
	~StrongReference() override;

	StrongReference &operator=(StrongReference &&other) noexcept;
	StrongReference &operator=(const StrongReference &) = delete;
	StrongReference &operator=(WeakReference &&other) noexcept;

	static inline StrongReference share(Reference &other);
	static inline StrongReference copy(const Reference &other);
	static inline StrongReference clone(const Reference &other);

protected:
	void mark() override {
		data()->mark();
	}

	explicit StrongReference(Info *infos);
};

template<class Type, typename>
Type *Reference::data() const {
	return static_cast<Type *>(m_info->data);
}

Reference::Flags Reference::flags() const {
	return m_info->flags;
}

template<class Type, typename... Args>
WeakReference WeakReference::create(Args &&...args) {
	return WeakReference(CONST_ADDRESS | CONST_VALUE | TEMPORARY,
						 g_garbage_collector.alloc<Type>(std::forward<Args>(args)...));
}

WeakReference WeakReference::create(Data *data) {
	return {CONST_ADDRESS | CONST_VALUE | TEMPORARY, data};
}

WeakReference WeakReference::share(Reference &other) {
	return WeakReference(other.info());
}

WeakReference WeakReference::copy(const Reference &other) {
	return {other.flags(), other.data()};
}

WeakReference WeakReference::clone(const Data *data) {
	return {CONST_ADDRESS | CONST_VALUE | TEMPORARY, g_garbage_collector.copy(data)};
}

WeakReference WeakReference::clone(const Reference &other) {
	return {other.flags(), g_garbage_collector.copy(other.data())};
}

StrongReference StrongReference::share(Reference &other) {
	return StrongReference(other.info());
}

StrongReference StrongReference::copy(const Reference &other) {
	return {other.flags(), other.data()};
}

StrongReference StrongReference::clone(const Reference &other) {
	return {other.flags(), Reference::g_garbage_collector.copy(other.data())};
}

}

#endif // MINT_REFERENCE_H
