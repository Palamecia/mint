#ifndef OBJECT_H
#define OBJECT_H

#include "memory/data.h"
#include "memory/reference.h"

#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <map>

namespace mint {

class Class;
class PackageData;

struct MINT_EXPORT Number : public Data {
	double value;

protected:
	friend class Reference;
	Number();
};

struct MINT_EXPORT Boolean : public Data {
	bool value;

protected:
	friend class Reference;
	Boolean();
};

struct MINT_EXPORT Object : public Data {
	Class *const metadata;
	Reference *data;

	void construct();
	void construct(const Object &other);

	ReferenceManager *referenceManager();
	void mark() override;

protected:
	friend class Reference;
	Object(Class *type);

	friend class Destructor;
	friend class GarbageCollector;
	virtual ~Object();

	void invalidateReferenceManager();

private:
	ReferenceManager *m_referenceManager;
};

struct MINT_EXPORT Package : public Data {
	PackageData *const data;

protected:
	friend class Reference;
	Package(PackageData *package);
};

struct MINT_EXPORT Function : public Data {
	struct Handler {
		Handler(PackageData *package, int module, int offset);

		using Capture = std::map<std::string, StrongReference>;

		int module;
		int offset;
		bool generator;
		PackageData *package;
		std::shared_ptr<Capture> capture;
	};

	using mapping_type = std::map<int, Handler>;
	mapping_type mapping;

	void mark() override;

protected:
	friend class Reference;
	Function();
};

}

#endif // OBJECT_H
