#ifndef OBJECT_H
#define OBJECT_H

#include "memory/reference.h"

#include <memory>
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

protected:
	friend class Reference;
	Object(Class *type);

	friend class Destructor;
	friend class GarbadgeCollector;
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

		typedef std::map<std::string, Reference> Capture;

		int module;
		int offset;
		PackageData *package;
		std::shared_ptr<Capture> capture;
	};

	typedef std::map<int, Handler> mapping_type;
	mapping_type mapping;

protected:
	friend class Reference;
	Function();
};

}

#endif // OBJECT_H
