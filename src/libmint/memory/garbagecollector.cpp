#include "memory/garbagecollector.h"
#include "memory/reference.h"
#include "system/assert.h"
#include "system/error.h"

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

size_t GarbadgeCollector::collect() {

	size_t count = 0;

	unique_lock<recursive_mutex> lock(m_mutex);

	for (auto &ref : m_references) {
		auto it = m_memory.find(ref->data());
		if (it != m_memory.end()) {
			it->second.reachable = true;
		}
	}

	auto it = m_memory.begin();
	while (it != m_memory.end()) {
		if (it->second.reachable) {
			it->second.reachable = false;
			++it;
		}
		else {
			Data *data = it->first;
			it = m_memory.erase(it);
			Reference::free(data);
			++count;
		}
	}

	return count;
}

void GarbadgeCollector::clean() {

	while (collect() > 0);

	assert(m_memory.empty());
}

void GarbadgeCollector::use(Data *data) {
	unique_lock<recursive_mutex> lock(m_mutex);
	auto it = m_memory.find(data);
	if (it != m_memory.end()) {
		it->second.count++;
	}
}

void GarbadgeCollector::release(Data *data) {
	unique_lock<recursive_mutex> lock(m_mutex);
	auto it = m_memory.find(data);
	if (it != m_memory.end()) {
		if (--it->second.count == 0) {
			m_memory.erase(it);
			Reference::free(data);
		}
	}
}

Data *GarbadgeCollector::registerData(Data *data) {
	unique_lock<recursive_mutex> lock(m_mutex);
	m_memory[data] = { false, 0 };
	return data;
}

void GarbadgeCollector::registerReference(Reference *ref) {
	unique_lock<recursive_mutex> lock(m_mutex);
	m_references.insert(ref);
}

void GarbadgeCollector::unregisterReference(Reference *ref) {
	unique_lock<recursive_mutex> lock(m_mutex);
	m_references.erase(ref);
}
