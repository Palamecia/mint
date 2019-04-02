#ifndef GARBADGE_COLLECTOR_H
#define GARBADGE_COLLECTOR_H

#include <config.h>

#include <cstddef>
#include <set>

namespace mint {

class Reference;
class Data;

struct MemoryInfos {
	bool reachable;
	bool collected;
	bool collectable;
	size_t count;
};

class MINT_EXPORT GarbadgeCollector {
public:
	static GarbadgeCollector &instance();

	size_t collect();
	void clean();

protected:
	friend class Reference;
	void use(Data *data);
	void release(Data *data);
	Data *registerData(Data *data);
	void registerReference(Reference *reference);
	void unregisterReference(Reference *reference);

private:
	GarbadgeCollector();
	GarbadgeCollector(const GarbadgeCollector &other) = delete;
	GarbadgeCollector &operator =(const GarbadgeCollector &othet) = delete;
	~GarbadgeCollector();

	std::set<Reference *> m_references; /// \todo Use generations ???
	std::set<Data *> m_memory;
};

}

#endif // GARBADGE_COLLECTOR_H
