#include "memory/garbagecollector.h"
#include "memory/reference.h"
#include "system/error.h"

#include <assert.h>

using namespace std;
using namespace mint;

GarbadgeCollector::GarbadgeCollector() {

}

GarbadgeCollector::~GarbadgeCollector() {
	clean();
}

GarbadgeCollector &GarbadgeCollector::instance() {

	static GarbadgeCollector g_instance;
	return g_instance;
}

size_t GarbadgeCollector::free() {

	size_t count = 0;

	for (auto &ref : m_references) {
		auto it = m_memory.find(ref->data());
		if (it != m_memory.end()) {
			it->second = true;
		}
	}

	auto it = m_memory.begin();
	while (it != m_memory.end()) {
		if (it->second) {
			it->second = false;
			++it;
		}
		else {
			Reference::free(it->first);
			it = m_memory.erase(it);
			++count;
		}
	}

	return count;
}

void GarbadgeCollector::clean() {

	while (free());

	assert(m_memory.empty());
}
