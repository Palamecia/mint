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

#ifndef MINT_MEMORY_REFERENCE_H
#define MINT_MEMORY_REFERENCE_H

#include "mint/config.h"
#include "mint/memory/data.h"
#include "mint/memory/memorypool.h"
#include "mint/memory/garbagecollector.h"
#include <concepts>
#include <cstdint>
#include <memory>
#include <utility>

namespace mint {

class Object;
struct ReferenceInfo;

struct MINT_EXPORT FromCreate {};

struct MINT_EXPORT FromCopy {};

inline constexpr FromCreate create_from;
inline constexpr FromCopy copy_from;

class MINT_EXPORT Reference {
	friend class GarbageCollector;
public:
	using Flags = std::uint16_t;

	static constexpr Flags default_flags = 0x000;
	static constexpr Flags const_value = 0x001;
	static constexpr Flags const_address = 0x002;
	static constexpr Flags private_visibility = 0x004;
	static constexpr Flags protected_visibility = 0x008;
	static constexpr Flags package_visibility = 0x010;
	static constexpr Flags global = 0x020;
	static constexpr Flags temporary = 0x040;
	static constexpr Flags final_member = 0x080;
	static constexpr Flags override_member = 0x100;

	static constexpr Flags visibility_mask = (private_visibility | protected_visibility | package_visibility);

	class MINT_EXPORT Info {
		Flags _flags = default_flags;
		Data* _data = nullptr;
	public:
		Info(Flags flags, Data* data);
		~Info();

		Info(const Info&) = delete;
		Info(Info&&) = delete;

		Info& operator=(const Info&) = delete;
		Info& operator=(Info&&) = delete;

		template<class Type, typename... Args>
		    requires std::constructible_from<Type, Args...>
		static Type* alloc(Args&&... args) {
			static GarbageCollector& g_garbage_collector = GarbageCollector::instance();
			return g_garbage_collector.alloc<Type>(std::forward<Args>(args)...);
		}

		static Data* copy(const Data& data) {
			static GarbageCollector& g_garbage_collector = GarbageCollector::instance();
			return g_garbage_collector.copy(data);
		}

		[[nodiscard]] Flags flags() const {
			return _flags;
		}

		[[nodiscard]] Data* data() const {
			return _data;
		}

		void set_data(Data* data);
	};

	virtual ~Reference() = default;

	Reference& operator=(Reference&& other) = default;
	Reference& operator=(const Reference& other) = default;

	void copy_data(const Reference& other);
	void move_data(const Reference& other);

	template<std::derived_from<Data> Type = Data>
	[[nodiscard]] Type& data() const;

	[[nodiscard]] inline Flags flags() const;

	[[nodiscard]] std::shared_ptr<Reference::Info> info() const;

protected:
	Reference(Flags flags, Data* data);
	Reference(Reference&& other) = default;
	Reference(const Reference& other) = default;

	static LocalPool<Info> g_pool;

private:
	std::shared_ptr<Info> _info;
};

class MINT_EXPORT WeakReference final : public Reference {
public:
	WeakReference();
	template<std::derived_from<Data> Type, typename... Args>
	    requires std::constructible_from<Type, Args...>
	WeakReference(Flags flags, std::in_place_type_t<Type> /*in_place_type*/, Args&&... args);
	explicit WeakReference(Flags flags);
	WeakReference(Flags flags, Data& data);
	WeakReference(FromCreate /*create_from*/, const Reference& other);
	WeakReference(FromCopy /*copy_from*/, const Reference& other);
	WeakReference(FromCopy /*copy_from*/, Flags flags, const Data& data);
	WeakReference(const WeakReference& other) = default;
	WeakReference(const Reference& other);
	WeakReference(WeakReference&& other) = default;
	WeakReference(Reference&& other) noexcept;
	~WeakReference() override;

	WeakReference& operator=(WeakReference&& other) = default;
	WeakReference& operator=(const WeakReference& other) = default;
};

class MINT_EXPORT StrongReference final : public Reference, public MemoryRoot {
public:
	StrongReference();
	template<std::derived_from<Data> Type, typename... Args>
	    requires std::constructible_from<Type, Args...>
	StrongReference(Flags flags, std::in_place_type_t<Type> /*in_place_type*/, Args&&... args);
	explicit StrongReference(Flags flags);
	StrongReference(Flags flags, Data& data);
	StrongReference(FromCreate /*create_from*/, const Reference& other);
	StrongReference(FromCopy /*copy_from*/, const Reference& other);
	StrongReference(FromCopy /*copy_from*/, Flags flags, const Data& data);
	StrongReference(const StrongReference& other);
	StrongReference(const WeakReference& other);
	StrongReference(const Reference& other);
	StrongReference(StrongReference&& other) noexcept;
	StrongReference(WeakReference&& other) noexcept;
	StrongReference(Reference&& other) noexcept;
	~StrongReference() override;

	StrongReference& operator=(StrongReference&& other) = default;
	StrongReference& operator=(WeakReference&& other) noexcept;
	StrongReference& operator=(const StrongReference& other) = default;
	StrongReference& operator=(const WeakReference& other);

protected:
	void mark() override {
		data().mark();
	}
};

template<std::derived_from<Data> Type>
Type& Reference::data() const {
	return *static_cast<Type*>(_info->data());
}

Reference::Flags Reference::flags() const {
	return _info->flags();
}

template<std::derived_from<Data> Type, typename... Args>
    requires std::constructible_from<Type, Args...>
WeakReference make_weak_reference(Reference::Flags flags, Args&&... args) {
	return WeakReference(flags, std::in_place_type<Type>, std::forward<Args>(args)...);
}

template<std::derived_from<Data> Type, typename... Args>
    requires std::constructible_from<Type, Args...>
WeakReference::WeakReference(Reference::Flags flags, std::in_place_type_t<Type> /*in_place_type*/, Args&&... args) :
    Reference(flags, Info::alloc<Type>(std::forward<Args>(args)...)) {}

inline WeakReference::WeakReference(FromCreate /*create_from*/, const Reference& other) :
    Reference(other.flags(), &other.data()) {}

inline WeakReference::WeakReference(FromCopy /*copy_from*/, const Reference& other) :
    Reference(other.flags(), Info::copy(other.data())) {}

inline WeakReference::WeakReference(FromCopy /*copy_from*/, Flags flags, const Data& data) :
    Reference(flags, Info::copy(data)) {}

template<std::derived_from<Data> Type, typename... Args>
    requires std::constructible_from<Type, Args...>
StrongReference make_strong_reference(Reference::Flags flags, Args&&... args) {
	return StrongReference(flags, std::in_place_type<Type>, std::forward<Args>(args)...);
}

template<std::derived_from<Data> Type, typename... Args>
    requires std::constructible_from<Type, Args...>
StrongReference::StrongReference(Reference::Flags flags, std::in_place_type_t<Type> /*in_place_type*/, Args&&... args) :
    Reference(flags, Info::alloc<Type>(std::forward<Args>(args)...)) {
	register_root();
}

inline StrongReference::StrongReference(FromCreate /*create_from*/, const Reference& other) :
    Reference(other.flags(), &other.data()) {
	register_root();
}

inline StrongReference::StrongReference(FromCopy /*copy_from*/, const Reference& other) :
    Reference(other.flags(), Info::copy(other.data())) {
	register_root();
}

inline StrongReference::StrongReference(FromCopy /*copy_from*/, Flags flags, const Data& data) :
    Reference(flags, Info::copy(data)) {
	register_root();
}

}

#endif // MINT_MEMORY_REFERENCE_H
