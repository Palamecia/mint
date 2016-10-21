#ifndef GARBADGE_COLLECTOR_H
#define GARBADGE_COLLECTOR_H

#include "memory/data.h"

#include <map>
#include <set>

class Reference;

class GarbadgeCollector {
public:
	static void free();
	static void clean();

private:
	typedef std::set<Reference *> ReferenceSet;
	typedef std::map<Data *, bool> InternalPtrMap;

	GarbadgeCollector();

	static GarbadgeCollector g_instance;
	static ReferenceSet g_refs;
	static InternalPtrMap g_ptrs;

	friend class Reference;
};

#endif // GARBADGE_COLLECTOR_H
