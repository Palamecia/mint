#ifndef REFERENCE_H
#define REFERENCE_H

#include "memory/data.h"
#include "memory/garbagecollector.h"

#include <cinttypes>
#include <memory>

namespace mint {

class SharedReference;
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
		global                = 0x20
	};

	virtual ~Reference();

	Reference &operator =(const Reference &other);

	void clone(const Reference &other);
	void copy(const Reference &other);
	void move(const Reference &other);

	template<class Type = Data>
	Type *data() const;

	Flags flags() const;

	template<class Type, typename... Args>
	static Type *alloc(Args... args);

	ReferenceInfos *infos();

protected:
	Reference(Flags flags = standard, Data *data = nullptr);
	Reference(const Reference &other);
	Reference(ReferenceInfos* infos);

	static GarbageCollector &g_garbageCollector;
	static void free(Data *ptr);
	static void destroy(Data *ptr);

	void setData(Data *data);

private:
	ReferenceInfos *m_infos;
};

struct ReferenceInfos {
	Reference::Flags flags = Reference::standard;
	MemoryInfos *infos = nullptr;
	Data *data = nullptr;
	size_t refcount = 0;
};

class MINT_EXPORT WeakReference : public Reference {
	friend class SharedReference;
public:
	WeakReference(Flags flags = standard, Data *data = nullptr);
	WeakReference(const Reference &other);
	~WeakReference();

protected:
	WeakReference(ReferenceInfos* infos);
};

class MINT_EXPORT StrongReference : public Reference {
	friend class GarbageCollector;
	friend class SharedReference;
public:
	StrongReference(Flags flags = standard, Data *data = nullptr);
	StrongReference(const Reference &other);
	~StrongReference();

	StrongReference &operator =(const StrongReference &other);

protected:
	StrongReference(ReferenceInfos* infos);

private:
	StrongReference* prev = nullptr;
	StrongReference* next = nullptr;
};

class MINT_EXPORT SharedReference {
public:
	SharedReference(std::nullptr_t);
	SharedReference(SharedReference &&other);
	~SharedReference();

	template<class Type>
	static SharedReference strong();
	static SharedReference strong(Data *data);
	static SharedReference strong(Reference::Flags flags, Data *data = nullptr);
	static SharedReference strong(Reference &reference);
	static SharedReference weak(Reference &reference);

	SharedReference &operator =(SharedReference &&other);

	Reference &operator *() const;
	Reference *operator ->() const;
	Reference *get() const;

	operator bool() const;

protected:
	SharedReference(Reference *reference);

private:
	Reference *m_reference;
};

template<class Type, typename... Args>
Type *Reference::alloc(Args... args) {
	return static_cast<Type *>(g_garbageCollector.registerData(new Type(std::forward<Args>(args)...)));
}

template<>
MINT_EXPORT None *Reference::alloc<None>();

template<>
MINT_EXPORT Null *Reference::alloc<Null>();

template<class Type>
Type *Reference::data() const {
	return static_cast<Type *>(m_infos->data);
}

template<class Type>
SharedReference SharedReference::strong() {
	return SharedReference(new StrongReference(Reference::const_address | Reference::const_value, Reference::alloc<Type>()));
}

}

#endif // REFERENCE_H
