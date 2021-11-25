#ifndef MINT_REFERENCE_H
#define MINT_REFERENCE_H

#include "memory/data.h"
#include "memory/memorypool.hpp"
#include "memory/garbagecollector.h"

#include <cinttypes>
#include <memory>

namespace mint {

struct Object;
struct ReferenceInfos;

class MINT_EXPORT Reference {
	friend class Destructor;
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

	template<class Type, typename... Args>
	static Type *alloc(Args&&... args);

	ReferenceInfos *infos();

protected:
	Reference(Flags flags = standard, Data *data = nullptr);
	Reference(Reference &&other) noexcept;
	Reference(ReferenceInfos *infos) noexcept;

	static GarbageCollector &g_garbageCollector;
	static LocalPool<ReferenceInfos> g_pool;
	static Data *copy(const Data *other);
	static void free(Data *ptr);
	static void destroy(Object *ptr);

	void setData(Data *data);

private:
	ReferenceInfos *m_infos;
};

struct ReferenceInfos {
	Reference::Flags flags = Reference::standard;
	Data *data = nullptr;
	size_t refcount = 0;
};

class MINT_EXPORT WeakReference : public Reference {
	template<typename Type> friend class LocalPool;
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

class MINT_EXPORT StrongReference : public Reference {
	friend class GarbageCollector;
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
	StrongReference(ReferenceInfos *infos);

private:
	StrongReference* prev = nullptr;
	StrongReference* next = nullptr;
};

template<class Type, typename... Args>
Type *Reference::alloc(Args&&... args) {
	return Type::g_pool.alloc(std::forward<Args>(args)...);
}

template<>
MINT_EXPORT None *Reference::alloc<None>();

template<>
MINT_EXPORT Null *Reference::alloc<Null>();

template<class Type>
Type *Reference::data() const {
	return static_cast<Type *>(m_infos->data);
}

Reference::Flags Reference::flags() const {
	return m_infos->flags;
}

template<class Type, typename... Args>
WeakReference WeakReference::create(Args&&... args) {
	return WeakReference(const_address | const_value | temporary, Reference::alloc<Type>(std::forward<Args>(args)...));
}
WeakReference WeakReference::create(Data *data) {
	return WeakReference(const_address | const_value | temporary, data);
}

WeakReference WeakReference::share(Reference &other) {
	return WeakReference(other.infos());
}

WeakReference WeakReference::clone(Data *data) {
	return WeakReference(const_address | const_value | temporary, Reference::copy(data));
}

WeakReference WeakReference::clone(const Reference &other) {
	return WeakReference(other.flags(), Reference::copy(other.data()));
}

StrongReference StrongReference::share(Reference &other) {
	return StrongReference(other.infos());
}

StrongReference StrongReference::clone(const Reference &other) {
	return StrongReference(other.flags(), Reference::copy(other.data()));
}

}

#endif // MINT_REFERENCE_H
