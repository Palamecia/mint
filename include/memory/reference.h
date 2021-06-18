#ifndef REFERENCE_H
#define REFERENCE_H

#include "memory/data.h"
#include "memory/garbagecollector.h"

#include <cinttypes>
#include <memory>

namespace mint {

class MemoryPool;
class SharedReference;
struct Object;
struct ReferenceInfos;

class MINT_EXPORT Reference {
	friend class Destructor;
	friend class GarbageCollector;
public:
	struct copy_tag {};

	using Flags = int;
	enum Flag : Flags {
		standard              = 0x00,
		const_value           = 0x01,
		const_address         = 0x02,
		private_visibility    = 0x04,
		protected_visibility  = 0x08,
		package_visibility    = 0x10,
		global                = 0x20
	};

	virtual ~Reference();

	Reference &operator =(const Reference &other);
	Reference &operator =(Reference &&other) noexcept;

	void clone(const Reference &other);
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
	Reference(const Reference &other, copy_tag);
	Reference(Reference &&other) noexcept;
	Reference(const Reference &other);
	Reference(ReferenceInfos* infos);

	static GarbageCollector &g_garbageCollector;
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
	WeakReference(const Reference &other, copy_tag);
	WeakReference(WeakReference &&other) noexcept;
	WeakReference(const WeakReference &other);
	WeakReference(const Reference &other);
	~WeakReference();

	WeakReference &operator =(const WeakReference &other);
	WeakReference &operator =(WeakReference &&other) noexcept;

protected:
	WeakReference(ReferenceInfos* infos);
};

class MINT_EXPORT StrongReference : public Reference {
	template<typename Type> friend class LocalPool;
	friend class GarbageCollector;
public:
	StrongReference(Flags flags = standard, Data *data = nullptr);
	StrongReference(StrongReference &&other) noexcept;
	StrongReference(const StrongReference &other);
	StrongReference(const WeakReference &other);
	StrongReference(const Reference &other);
	~StrongReference();

	StrongReference &operator =(const WeakReference &other);
	StrongReference &operator =(const StrongReference &other);
	StrongReference &operator =(StrongReference &&other) noexcept;

protected:
	StrongReference(ReferenceInfos* infos);

private:
	StrongReference* prev = nullptr;
	StrongReference* next = nullptr;
};

class MINT_EXPORT SharedReference {
public:
	SharedReference();
	SharedReference(std::nullptr_t);
	SharedReference(SharedReference &&other) noexcept;
	~SharedReference();

	template<class Type, typename... Args>
	static SharedReference strong(Args&&... args);
	static SharedReference strong(Data *data);
	static SharedReference strong(Reference::Flags flags, Data *data = nullptr);
	static SharedReference strong(Reference &reference);
	static SharedReference weak(Reference &reference);

	SharedReference &operator =(SharedReference &&other) noexcept;

	inline Reference &operator *() const;
	inline Reference *operator ->() const;
	inline Reference *get() const;

	inline operator bool() const;

protected:
	SharedReference(MemoryPool *pool, Reference *reference);

private:
	Reference *m_reference;
	MemoryPool *m_pool;
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
SharedReference SharedReference::strong(Args&&... args) {
	return SharedReference::strong(Reference::const_address | Reference::const_value, Reference::alloc<Type>(std::forward<Args>(args)...));
}

Reference &SharedReference::operator *() const {
	return *m_reference;
}

Reference *SharedReference::operator ->() const {
	return m_reference;
}

Reference *SharedReference::get() const {
	return m_reference;
}

SharedReference::operator bool() const {
	return m_reference != nullptr;
}

}

#endif // REFERENCE_H
