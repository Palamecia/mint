#ifndef GARBADGE_COLLECTOR_H
#define GARBADGE_COLLECTOR_H

#include <map>
#include <set>

struct Data {
	enum Format {
		fmt_null,
		fmt_none,
		fmt_number,
		fmt_object,
		fmt_function,
		fmt_hash,
		fmt_array
	};
	Format format;
	Data() { format = fmt_none; }
};

class Reference;

class GarbadgeCollector {
public:
	static void free();

private:
	typedef std::set<Reference*> ReferenceSet;
	typedef std::map<const Data*, bool> InternalPtrMap;

	GarbadgeCollector();

	static GarbadgeCollector g_instance;
	static ReferenceSet g_refs;
	static InternalPtrMap g_ptrs;

	friend class Reference;
};

#endif // GARBADGE_COLLECTOR_H
