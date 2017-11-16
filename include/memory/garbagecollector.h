#ifndef GARBADGE_COLLECTOR_H
#define GARBADGE_COLLECTOR_H

#include "memory/data.h"

#include <map>
#include <set>

class Reference;

class GarbadgeCollector {
public:
	~GarbadgeCollector();

	static GarbadgeCollector &instance();

	size_t free();
	void clean();

private:
	GarbadgeCollector();

	std::set<Reference *> m_references;
	std::map<Data *, bool> m_memory;

	friend class Reference;
};

#endif // GARBADGE_COLLECTOR_H
