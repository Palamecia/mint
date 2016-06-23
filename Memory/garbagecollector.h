#ifndef GARBADGE_COLLECTOR_H
#define GARBADGE_COLLECTOR_H

#include <map>
#include <set>

struct Data {
	enum Format {
		fmt_none,
		fmt_null,
		fmt_number,
		fmt_object,
		fmt_function
	};
	Format format;
	Data() { format = fmt_none; }
	virtual ~Data() {}
};

class Reference;

class GarbadgeCollector {
public:
	static void free();

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
