#ifndef LIBRARY_H
#define LIBRARY_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class Plugin;

class MINT_EXPORT LibraryClass : public Class {
public:
	static LibraryClass *instance();

private:
	LibraryClass();
};

struct MINT_EXPORT Library : public Object {
	Library();
	Library(const Library &other);
	~Library();

	Plugin *plugin;

private:
	friend class Reference;
	static LocalPool<Library> g_pool;
};

}

#endif // LIBRARY_H
