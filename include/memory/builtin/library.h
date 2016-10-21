#ifndef LIBRARY_H
#define LIBRARY_H

#include "memory/class.h"
#include "memory/object.h"

class Plugin;

class LibraryClass : public Class {
public:
	static LibraryClass *instance();

private:
	LibraryClass();
};

struct Library : public Object {
	Library();
	~Library();
	Plugin *plugin;
};

#endif // LIBRARY_H
