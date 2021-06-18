#ifndef GARBADGE_COLLECTOR_H
#define GARBADGE_COLLECTOR_H

#include <config.h>

#include <cstddef>

namespace mint {

struct Data;
class StrongReference;

class MINT_EXPORT GarbageCollector {
	friend struct Data;
	friend class Reference;
	friend class StrongReference;
public:
	static GarbageCollector &instance();

	size_t collect();
	void clean();

protected:
	void use(Data *data);
	void release(Data *data);
	void registerData(Data *data);
	void registerRoot(StrongReference *reference);
	void unregisterRoot(StrongReference *reference);

private:
	GarbageCollector();
	GarbageCollector(const GarbageCollector &other) = delete;
	GarbageCollector &operator =(const GarbageCollector &othet) = delete;
	~GarbageCollector();

	struct {
		StrongReference* head = nullptr;
		StrongReference* tail = nullptr;
	} m_roots;

	struct {
		Data* head = nullptr;
		Data* tail = nullptr;
	} m_memory;
};

}

#endif // GARBADGE_COLLECTOR_H
