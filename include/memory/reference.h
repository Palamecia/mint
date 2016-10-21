#ifndef REFERENCE_H
#define REFERENCE_H

#include "garbagecollector.h"

class Reference {
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

	Reference(Flags flags = standard, Data *data = Reference::alloc<Data>());
	Reference(const Reference &other);
	~Reference();

	void clone(const Reference &other);
	void copy(const Reference &other);
	void move(const Reference &other);

	Data *data();
	const Data *data() const;

	Flags flags() const;

	template<class T, typename... Args> static T *alloc(Args... args)
	{ T *data = new T(args...); GarbadgeCollector::g_ptrs[data] = false; return data; }

	template<class T> static Reference *create()
	{ Reference *ref = new Reference(const_ref + const_value, alloc<T>()); return ref; }

private:
	Flags m_flags;
	Data *m_data;

	friend class GarbadgeCollector;
};

class SharedReference {
public:
	SharedReference() :
		m_ref(new Reference()), m_unique(true) {}

	SharedReference(Reference *ref) :
		m_ref(ref), m_unique(false) {}

	SharedReference(const SharedReference &other) {
		m_ref = other.m_ref;
		if ((m_unique = other.m_unique)) {
			((SharedReference &)other).m_ref = nullptr;
		}
	}

	~SharedReference()
	{ if (m_unique) { delete m_ref; m_ref = nullptr; } }

	static SharedReference unique(Reference *ref)
	{ SharedReference uniqueRef(ref); uniqueRef.m_unique = true; return uniqueRef; }

	bool isUnique() const
	{ return m_unique; }

	Reference &operator *() const
	{ return *m_ref; }

	Reference *operator ->() const
	{ return m_ref; }

	Reference *get() const
	{ return m_ref; }

private:
	Reference *m_ref;
	bool m_unique;
};

#endif // REFERENCE_H
