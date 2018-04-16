#ifndef REFERENCE_H
#define REFERENCE_H

#include "garbagecollector.h"

namespace mint {

class MINT_EXPORT Reference {
public:
	typedef int Flags;
	enum Flag : Flags {
		standard = 0x00,
		const_value = 0x01,
		const_ref = 0x02,
		child_hiden = 0x04,
		user_hiden = 0x08,
		global = 0x10
	};

	Reference(Flags flags = standard, Data *data = nullptr);
	Reference(const Reference &other);
	~Reference();

	void clone(const Reference &other);
	void copy(const Reference &other);
	void move(const Reference &other);

	Data *data();
	const Data *data() const;

	Flags flags() const;

	template<class Type, typename... Args>
	static Type *alloc(Args... args);

	template<class Type>
	static Reference *create();

	template<class Type>
	Type *data();
	template<class Type>
	const Type *data() const;

protected:
	static void free(Data *ptr);

private:
	Flags m_flags;
	Data *m_data;

	friend class GarbadgeCollector;
};

class MINT_EXPORT SharedReference {
public:
	SharedReference();
	SharedReference(Reference *ref);
	SharedReference(const SharedReference &other);
	~SharedReference();

	static SharedReference unique(Reference *ref);
	SharedReference &operator =(const SharedReference &other);

	Reference &operator *() const;
	Reference *operator ->() const;
	Reference *get() const;

	operator bool() const;
	bool isUnique() const;

protected:
	SharedReference(Reference *ref, bool unique);

private:
	mutable Reference *m_ref;
	bool m_unique;
};

template<class Type, typename... Args>
Type *Reference::alloc(Args... args) {
	Type *data = new Type(args...);
	GarbadgeCollector::instance().m_memory[data] = false;
	return data;
}

template<>
None *Reference::alloc<None>();

template<>
Null *Reference::alloc<Null>();

template<class Type>
Reference *Reference::create() {
	return new Reference(const_ref | const_value, alloc<Type>());
}

template<class Type>
Type *Reference::data() {
	return dynamic_cast<Type *>(data());
}

template<class Type>
const Type *Reference::data() const {
	return dynamic_cast<const Type *>(data());
}

}

#endif // REFERENCE_H
