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

	Reference();
	template<std::derived_from<Data> Type, typename... Args>
	    requires std::constructible_from<Type, Args...>
	Reference(Flags flags, std::in_place_type_t<Type> /*in_place_type*/, Args&&... args);
	explicit Reference(Flags flags);
	Reference(Flags flags, Data& data);
	Reference(FromCreate /*create_from*/, const Reference& other);
	Reference(FromCopy /*copy_from*/, const Reference& other);
	Reference(FromCopy /*copy_from*/, Flags flags, const Data& data);
	Reference(const Reference& other) = default;
	Reference(Reference&& other) = default;
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

	static LocalPool<Info> g_pool;

private:
	std::shared_ptr<Info> _info;
};

class MINT_EXPORT RootReference final : public Reference, public MemoryRoot {
public:
	RootReference();
	template<std::derived_from<Data> Type, typename... Args>
	    requires std::constructible_from<Type, Args...>
	RootReference(Flags flags, std::in_place_type_t<Type> /*in_place_type*/, Args&&... args);
	explicit RootReference(Flags flags);
	RootReference(Flags flags, Data& data);
	RootReference(FromCreate /*create_from*/, const Reference& other);
	RootReference(FromCopy /*copy_from*/, const Reference& other);
	RootReference(FromCopy /*copy_from*/, Flags flags, const Data& data);
	RootReference(const RootReference& other);
	RootReference(const Reference& other);
	RootReference(RootReference&& other) noexcept;
	RootReference(Reference&& other) noexcept;
	~RootReference() override;

	RootReference& operator=(RootReference&& other) = default;
	RootReference& operator=(Reference&& other) noexcept;
	RootReference& operator=(const RootReference& other) = default;
	RootReference& operator=(const Reference& other);

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
Reference make_reference(Reference::Flags flags, Args&&... args) {
	return Reference(flags, std::in_place_type<Type>, std::forward<Args>(args)...);
}

template<std::derived_from<Data> Type, typename... Args>
    requires std::constructible_from<Type, Args...>
Reference::Reference(Reference::Flags flags, std::in_place_type_t<Type> /*in_place_type*/, Args&&... args) :
    Reference(flags, Info::alloc<Type>(std::forward<Args>(args)...)) {}

inline Reference::Reference(FromCreate /*create_from*/, const Reference& other) :
    Reference(other.flags(), &other.data()) {}

inline Reference::Reference(FromCopy /*copy_from*/, const Reference& other) :
    Reference(other.flags(), Info::copy(other.data())) {}

inline Reference::Reference(FromCopy /*copy_from*/, Flags flags, const Data& data) :
    Reference(flags, Info::copy(data)) {}

template<std::derived_from<Data> Type, typename... Args>
    requires std::constructible_from<Type, Args...>
RootReference make_root_reference(Reference::Flags flags, Args&&... args) {
	return RootReference(flags, std::in_place_type<Type>, std::forward<Args>(args)...);
}

template<std::derived_from<Data> Type, typename... Args>
    requires std::constructible_from<Type, Args...>
RootReference::RootReference(Reference::Flags flags, std::in_place_type_t<Type> /*in_place_type*/, Args&&... args) :
    Reference(flags, Info::alloc<Type>(std::forward<Args>(args)...)) {
	register_root();
}

inline RootReference::RootReference(FromCreate /*create_from*/, const Reference& other) :
    Reference(other.flags(), &other.data()) {
	register_root();
}

inline RootReference::RootReference(FromCopy /*copy_from*/, const Reference& other) :
    Reference(other.flags(), Info::copy(other.data())) {
	register_root();
}

inline RootReference::RootReference(FromCopy /*copy_from*/, Flags flags, const Data& data) :
    Reference(flags, Info::copy(data)) {
	register_root();
}

}

#endif // MINT_MEMORY_REFERENCE_H
