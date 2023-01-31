#ifndef MINT_GARBAGECOLLECTOR_H
#define MINT_GARBAGECOLLECTOR_H

#include "config.h"
#include "memory/data.h"

#include <cstddef>
#include <vector>
#include <set>

namespace mint {

class WeakReference;
class MemoryRoot;

class MINT_EXPORT GarbageCollector {
	friend struct Data;
	friend class MemoryRoot;
public:
	static GarbageCollector &instance();

	size_t collect();
	void clean();

	inline void use(Data *data);
	inline void release(Data *data);

	std::vector<WeakReference> *createStack();
	void removeStack(std::vector<WeakReference> *stack);

protected:
	void registerData(Data *data);
	void unregisterData(Data *data);
	void registerRoot(MemoryRoot *reference);
	void unregisterRoot(MemoryRoot *reference);

private:
	GarbageCollector();
	GarbageCollector(const GarbageCollector &other) = delete;
	GarbageCollector &operator =(const GarbageCollector &othet) = delete;
	~GarbageCollector();

	std::set<std::vector<WeakReference> *> m_stacks;

	struct {
		MemoryRoot *head = nullptr;
		MemoryRoot *tail = nullptr;
	} m_roots;

	struct {
		Data *head = nullptr;
		Data *tail = nullptr;
	} m_memory;
};

class MINT_EXPORT MemoryRoot {
	friend class GarbageCollector;
public:
	MemoryRoot();
	virtual ~MemoryRoot();

protected:
	static GarbageCollector &g_garbageCollector;
	virtual void mark() = 0;

private:
	MemoryRoot *prev = nullptr;
	MemoryRoot *next = nullptr;
};

void GarbageCollector::use(Data *data) {
	++data->infos.refcount;
}

void GarbageCollector::release(Data *data) {
	if (!--data->infos.refcount && !data->infos.collected) {
		unregisterData(data);
	}
}

}

#endif // MINT_GARBAGECOLLECTOR_H
