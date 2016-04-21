#ifndef REFERENCE_H
#define REFERENCE_H

#include "garbagecollector.h"

class Reference {
public:
	enum Flag : int {
		standard = 0x00,
		const_value = 0x01,
		const_ref = 0x02,
	};
	typedef int Flags;

	Reference(Flags flags = standard, Data *data = Reference::alloc<Data>());
	~Reference();

	void clone(const Reference &other);
	void copy(const Reference &other);
	void move(const Reference &other);

	Data *data();
	const Data *data() const;

	Flags flags() const;

	template<class T, typename... Args> static T *alloc(Args... args)
	{ T *data = new T(args...); GarbadgeCollector::g_ptrs[data]; return data; }

	template<class T> static Reference *create()
	{ Reference *ref = new Reference(const_ref | const_value); ref->m_data = alloc<T>(); return ref; }

private:
	Flags m_flags;
	Data *m_data;

	friend class GarbadgeCollector;
};

class SharedReference {
public:
	SharedReference(Reference *ref) :
		m_ref(ref), m_unique(false) {}

	SharedReference(const SharedReference &other) {
		m_ref = other.m_ref;
		if (m_unique = other.m_unique) {
			((SharedReference &)other).m_ref = nullptr;
		}
	}

	~SharedReference()
	{ if (m_unique) delete m_ref; }

	static SharedReference unique(Reference *ref)
	{ SharedReference uniqueRef(ref); uniqueRef.m_unique = true; return uniqueRef; }

	Reference &get()
	{ return *m_ref; }

	operator Reference()
	{ return *m_ref; }

private:
	SharedReference() = delete;
	Reference *m_ref;
	bool m_unique;
};

#endif // REFERENCE_H
