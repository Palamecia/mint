#ifndef REFERENCE_H
#define REFERENCE_H

#include "memory/data.h"
#include "memory/garbagecollector.h"

namespace mint {

class SharedReference;

class MINT_EXPORT Reference {
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

	Reference(Flags flags = standard, Data *data = nullptr);
	Reference(const Reference &other);
	~Reference();

	Reference &operator =(const Reference &other);

	void clone(const Reference &other);
	void copy(const Reference &other);
	void move(const Reference &other);

	template<class Type = Data>
	Type *data() const;

	Flags flags() const;

	template<class Type, typename... Args>
	static Type *alloc(Args... args);

	template<class Type>
	static Reference *create();

	static Reference *create(Data *data);

protected:
	static void free(Data *ptr);

	void setData(Data *data);

private:
	Data *m_data;
	Flags m_flags;

	friend class GarbadgeCollector;
};

class MINT_EXPORT ReferenceManager {
public:
	explicit ReferenceManager();
	~ReferenceManager();

	ReferenceManager &operator=(const ReferenceManager &other);

	void link(SharedReference *reference);
	void unlink(SharedReference *reference);

private:
	std::set<SharedReference *> m_references;
};

class MINT_EXPORT SharedReference {
public:
	SharedReference();
	SharedReference(std::nullptr_t);
	SharedReference(SharedReference &&other);
	~SharedReference();

	static SharedReference unsafe(Reference *reference);
	static SharedReference unique(Reference *reference);
	static SharedReference linked(ReferenceManager *manager, Reference *reference);

	SharedReference &operator =(SharedReference &&other);

	Reference &operator *() const;
	Reference *operator ->() const;
	Reference *get() const;

	operator bool() const;
	bool isUnique() const;

	void makeUnique();

protected:
	SharedReference(Reference *reference, bool unique);
	SharedReference(Reference *reference, ReferenceManager *manager);

private:
	Reference *m_reference;
	ReferenceManager *m_linked;
	bool m_unique;
};

template<class Type, typename... Args>
Type *Reference::alloc(Args... args) {
	return static_cast<Type *>(GarbadgeCollector::instance().registerData(new Type(args...)));
}

template<>
MINT_EXPORT None *Reference::alloc<None>();

template<>
MINT_EXPORT Null *Reference::alloc<Null>();

template<class Type>
Reference *Reference::create() {
	return new Reference(const_address | const_value, alloc<Type>());
}

template<class Type>
Type *Reference::data() const {
	return static_cast<Type *>(m_data);
}

}

#endif // REFERENCE_H
