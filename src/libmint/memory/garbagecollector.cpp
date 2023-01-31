#include "memory/garbagecollector.h"
#include "memory/reference.h"
#include "memory/data.h"
#include "system/assert.h"

#include <list>

using namespace std;
using namespace mint;

#define gc_list_insert_element(list, node) \
	if (list.tail) { \
		list.tail->next = node; \
		node->prev = list.tail; \
		list.tail = node; \
	} \
	else { \
		list.head = list.tail = node; \
	}

#define gc_list_remove_element(list, node) \
	if (node->prev) { \
		node->prev->next = node->next; \
	} \
	else { \
		list.head = node->next; \
	} \
	if (node->next) { \
		node->next->prev = node->prev; \
	} \
	else { \
		list.tail = node->prev; \
	}

GarbageCollector &MemoryRoot::g_garbageCollector = GarbageCollector::instance();

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

	// mark roots
	for (MemoryRoot *root = m_roots.head; root != nullptr; root = root->next) {
		root->mark();
	}

	// mark stacks
	for (vector<WeakReference> *stack : m_stacks) {
		for (WeakReference &reference : *stack) {
			reference.data()->mark();
		}
	}

	// sweep
	for (Data *data = m_memory.head; data != nullptr; data = data->next) {
		if (data->infos.reachable) {
			data->infos.reachable = (data->infos.refcount == 0);
		}
		else {
			gc_list_remove_element(m_memory, data);
			collected.emplace_back(data);
			data->infos.collected = true;
			++data->infos.refcount;
		}
	}

	for (Data *data : collected) {
		Reference::free(data);
	}

	return collected.size();
}

void GarbageCollector::clean() {

	assert(m_stacks.empty());
	assert(m_roots.head == nullptr);

	while (collect() > 0);

	assert(m_memory.head == nullptr);
}

void GarbageCollector::registerData(Data *data) {
	gc_list_insert_element(m_memory, data);
}

void GarbageCollector::unregisterData(Data *data) {
	gc_list_remove_element(m_memory, data);
	data->infos.collected = true;
	Reference::free(data);
}

void GarbageCollector::registerRoot(MemoryRoot *reference) {
	assert(m_roots.head == nullptr || m_roots.head->prev == nullptr);
	assert(m_roots.tail == nullptr || m_roots.tail->next == nullptr);
	gc_list_insert_element(m_roots, reference);
	assert(m_roots.head->prev == nullptr);
	assert(m_roots.tail->next == nullptr);
}

void GarbageCollector::unregisterRoot(MemoryRoot *reference) {
	assert(m_roots.head->prev == nullptr);
	assert(m_roots.tail->next == nullptr);
	gc_list_remove_element(m_roots, reference);
	assert(m_roots.head == nullptr || m_roots.head->prev == nullptr);
	assert(m_roots.tail == nullptr || m_roots.tail->next == nullptr);
}

vector<WeakReference> *GarbageCollector::createStack() {
	vector<WeakReference> *stack = new vector<WeakReference>;
	m_stacks.emplace(stack);
	stack->reserve(0x4000);
	return stack;
}

void GarbageCollector::removeStack(vector<WeakReference> *stack) {
	assert(stack->empty());
	m_stacks.erase(stack);
	delete stack;
}

MemoryRoot::MemoryRoot() {
	g_garbageCollector.registerRoot(this);
}

MemoryRoot::~MemoryRoot() {
	g_garbageCollector.unregisterRoot(this);
}
