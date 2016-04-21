#include "garbagecollector.h"
#include "reference.h"

using namespace std;

GarbadgeCollector GarbadgeCollector::g_instance;
GarbadgeCollector::ReferenceSet GarbadgeCollector::g_refs;
GarbadgeCollector::InternalPtrMap GarbadgeCollector::g_ptrs;

GarbadgeCollector::GarbadgeCollector() {}

void GarbadgeCollector::free() {
	for (auto it = g_refs.begin(); it != g_refs.end(); ++it) {
		g_ptrs[(*it)->data()] = true;
	}

	auto it = g_ptrs.begin();
	while (it != g_ptrs.end()) {
		if (it->second) {
			it->second = false;
			++it;
		}
		else {
			delete it->first;
			it = g_ptrs.erase(it);
		}
	}
}
