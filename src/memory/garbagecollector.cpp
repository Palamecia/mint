#include "memory/garbagecollector.h"
#include "memory/reference.h"

using namespace std;

GarbadgeCollector GarbadgeCollector::g_instance;
GarbadgeCollector::ReferenceSet GarbadgeCollector::g_refs;
GarbadgeCollector::InternalPtrMap GarbadgeCollector::g_ptrs;

GarbadgeCollector::GarbadgeCollector() {}

void GarbadgeCollector::free() {

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
			delete it->first;
			it = g_ptrs.erase(it);
		}
	}
}

void GarbadgeCollector::clean() {

	g_refs.clear();

	for (auto ptr : g_ptrs) {
		delete ptr.first;
	}
	g_ptrs.clear();
}
