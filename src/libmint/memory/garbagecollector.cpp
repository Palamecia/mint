#include "memory/garbagecollector.h"
#include "memory/reference.h"
#include "memory/data.h"
#include "system/assert.h"
#include "system/error.h"

#include <list>

using namespace std;
using namespace mint;

#define gc_list_insert_element(list, element) \
	if (list.tail) { \
		list.tail->next = element; \
		element->prev = list.tail; \
		list.tail = element; \
	} \
	else { \
		list.head = list.tail = element; \
	}

#define gc_list_remove_element(list, element) \
	if (element->prev) { \
		element->prev->next = element->next; \
	} \
	else { \
		list.head = element->next; \
	} \
	if (element->next) { \
		element->next->prev = element->prev; \
	} \
	else { \
		list.tail = element->prev; \
	}

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

	list<Data *> collected;

	// mark
	for (StrongReference *reference = m_roots.head; reference != nullptr; reference = reference->next) {
		reference->data()->mark();
	}

	// sweep
	for (Data *data = m_memory.head; data != nullptr; data = data->next) {
		if (data->infos.reachable) {
			data->infos.reachable = (data->infos.refcount > 0);
		}
		else {
			collected.push_back(data);
			gc_list_remove_element(m_memory, data);
			data->infos.collected = true;
			data->infos.refcount++;
		}
	}

	for (Data *data : collected) {
		delete data;
	}

	return collected.size();
}

void GarbageCollector::clean() {

	while (collect() > 0);

	assert(m_memory.head == nullptr);
}

void GarbageCollector::use(Data *data) {
	++data->infos.refcount;
}

void GarbageCollector::release(Data *data) {

	assert(data);

	if (!--data->infos.refcount && !data->infos.collected) {
		gc_list_remove_element(m_memory, data);
		data->infos.collected = true;
		Reference::free(data);
	}
}

void GarbageCollector::registerData(Data *data) {
	gc_list_insert_element(m_memory, data);
}

void GarbageCollector::registerRoot(StrongReference *reference) {
	assert(m_roots.head == nullptr || m_roots.head->prev == nullptr);
	assert(m_roots.tail == nullptr || m_roots.tail->next == nullptr);
	gc_list_insert_element(m_roots, reference);
	assert(m_roots.head->prev == nullptr);
	assert(m_roots.tail->next == nullptr);
}

void GarbageCollector::unregisterRoot(StrongReference *reference) {
	assert(m_roots.head->prev == nullptr);
	assert(m_roots.tail->next == nullptr);
	gc_list_remove_element(m_roots, reference);
	assert(m_roots.head == nullptr || m_roots.head->prev == nullptr);
	assert(m_roots.tail == nullptr || m_roots.tail->next == nullptr);
}
