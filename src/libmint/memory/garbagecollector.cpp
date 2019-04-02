#include "memory/garbagecollector.h"
#include "memory/reference.h"
#include "memory/data.h"
#include "system/assert.h"
#include "system/error.h"

#include <list>

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

	list<Data *> collected;

	for (auto &reference : m_references) {
		reference->data()->infos->reachable = true;
	}

	auto it = m_memory.begin();
	while (it != m_memory.end()) {
		Data *data = *it;
		if (data->infos->reachable) {
			data->infos->reachable = !data->infos->collectable;
			++it;
		}
		else {
			it = m_memory.erase(it);
			collected.push_back(data);
			data->infos->collected = true;
		}
	}

	for (Data *data : collected) {
		Reference::free(data);
	}

	return collected.size();
}

void GarbadgeCollector::clean() {

	while (collect() > 0);

	assert(m_memory.empty());
}

void GarbadgeCollector::use(Data *data) {
	data->infos->collectable = true;
	++data->infos->count;
}

void GarbadgeCollector::release(Data *data) {

	MemoryInfos *infos = data->infos;

	assert(data);

	if (--infos->count == 0) {
		if (!infos->collected) {
			infos->collected = true;
			m_memory.erase(data);
			Reference::free(data);
		}
	}
}

Data *GarbadgeCollector::registerData(Data *data) {
	data->infos = new MemoryInfos{ true, false, false, 0 };
	m_memory.insert(data);
	return data;
}

void GarbadgeCollector::registerReference(Reference *reference) {
	m_references.insert(reference);
}

void GarbadgeCollector::unregisterReference(Reference *reference) {
	m_references.erase(reference);
}
