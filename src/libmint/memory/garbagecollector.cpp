#include "memory/garbagecollector.h"
#include "memory/reference.h"
#include "system/error.h"

using namespace std;

GarbadgeCollector::ReferenceSet GarbadgeCollector::g_refs;
GarbadgeCollector::InternalPtrMap GarbadgeCollector::g_ptrs;

GarbadgeCollector::GarbadgeCollector() {}

size_t GarbadgeCollector::free() {

	size_t count = 0;

	for (auto &ref : g_refs) {
		auto it = g_ptrs.find(ref->data());
		if (it != g_ptrs.end()) {
			it->second = true;
		}
	}

	auto it = g_ptrs.begin();
	while (it != g_ptrs.end()) {
		if (it->second) {
			it->second = false;
			++it;
		}
		else {
			Reference::free(it->first);
			it = g_ptrs.erase(it);
			++count;
		}
	}

	return count;
}

void GarbadgeCollector::clean() {

	g_refs.clear();

	while (!g_ptrs.empty()) {
		auto ptr = g_ptrs.begin();
		g_ptrs.erase(ptr);
		delete ptr->first;
	}
}
