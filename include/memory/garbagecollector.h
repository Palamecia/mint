#ifndef GARBADGE_COLLECTOR_H
#define GARBADGE_COLLECTOR_H

#include "memory/data.h"

#include <mutex>
#include <map>
#include <set>

namespace mint {

class Reference;

class MINT_EXPORT GarbadgeCollector {
public:
	~GarbadgeCollector();

	static GarbadgeCollector &instance();

	size_t collect();
	void clean();

protected:
	friend class Reference;
	void use(Data *data);
	void release(Data *data);
	Data *registerData(Data *data);
	void registerReference(Reference *ref);
	void unregisterReference(Reference *ref);

private:
	GarbadgeCollector();
	GarbadgeCollector(const GarbadgeCollector &other) = delete;
	GarbadgeCollector &operator =(const GarbadgeCollector &othet) = delete;

	struct MemoryInfos {
		bool reachable;
		bool collectable;
		size_t count;
	};

	std::mutex m_mutex;
	std::set<Reference *> m_references; /// \todo Use generations ???
	std::map<Data *, MemoryInfos> m_memory;
};

}

#endif // GARBADGE_COLLECTOR_H
