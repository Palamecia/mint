#ifndef MINT_LIBRARY_H
#define MINT_LIBRARY_H

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

#endif // MINT_LIBRARY_H
