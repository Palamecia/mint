#ifndef GARBADGE_COLLECTOR_H
#define GARBADGE_COLLECTOR_H

#include <config.h>

#include <cstddef>
#include <set>

namespace mint {

class Reference;
struct Data;

struct MemoryInfos {
	bool reachable;
	bool collected;
	bool collectable;
	size_t count;
};

class MINT_EXPORT GarbageCollector {
	friend class Reference;
	friend class StrongReference;
public:
	static GarbageCollector &instance();

	size_t collect();
	void clean();

protected:
	void use(Data *data);
	void release(Data *data);
	Data *registerData(Data *data);
	void registerRoot(Reference *reference);
	void unregisterRoot(Reference *reference);

private:
	GarbageCollector();
	GarbageCollector(const GarbageCollector &other) = delete;
	GarbageCollector &operator =(const GarbageCollector &othet) = delete;
	~GarbageCollector();

	std::set<Reference *> m_roots; /// \todo Use generations ???
	std::set<Data *> m_memory;
};

}

#endif // GARBADGE_COLLECTOR_H
