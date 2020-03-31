#include "memory/garbagecollector.h"
#include "memory/reference.h"
#include "memory/data.h"
#include "system/assert.h"
#include "system/error.h"

#include <list>

using namespace std;
using namespace mint;

GarbageCollector::GarbageCollector() {

}

GarbageCollector::~GarbageCollector() {
	clean();
}

GarbageCollector &GarbageCollector::instance() {

	static GarbageCollector g_instance;
	return g_instance;
}

size_t GarbageCollector::collect() {

	list<pair<Data *, MemoryInfos *>> collected;

	// mark
	for (Reference *reference : m_roots) {
		reference->data()->mark();
	}

	// sweep
	auto it = m_memory.begin();
	while (it != m_memory.end()) {
		Data *data = *it;
		if (data->infos->reachable) {
			data->infos->reachable = !data->infos->collectable;
			++it;
		}
		else {
			it = m_memory.erase(it);
			data->infos->count++;
			data->infos->collected = true;
			collected.push_back({data, data->infos});
		}
	}

	for (pair<Data *, MemoryInfos *> data : collected) {
		delete data.first;
	}

	for (pair<Data *, MemoryInfos *> data : collected) {
		delete data.second;
	}

	return collected.size();
}

void GarbageCollector::clean() {

	while (collect() > 0);

	assert(m_memory.empty());
}

void GarbageCollector::use(Data *data) {
	assert(m_memory.find(data) != m_memory.end());
	data->infos->collectable = true;
	++data->infos->count;
}

void GarbageCollector::release(Data *data) {

	MemoryInfos *infos = data->infos;

	assert(data);
	assert(m_memory.find(data) != m_memory.end());

	if (--infos->count == 0) {
		if (!infos->collected) {
			infos->collected = true;
			m_memory.erase(data);
			Reference::free(data);
		}
	}
}

Data *GarbageCollector::registerData(Data *data) {
	data->infos = new MemoryInfos{ true, false, false, 0 };
	m_memory.insert(data);
	return data;
}

void GarbageCollector::registerRoot(Reference *reference) {
	m_roots.insert(reference);
}

void GarbageCollector::unregisterRoot(Reference *reference) {
	m_roots.erase(reference);
}
