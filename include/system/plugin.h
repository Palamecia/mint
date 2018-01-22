#ifndef PLUGIN_H
#define PLUGIN_H

#include "config.h"

#include <string>

namespace mint {

class Cursor;

class MINT_EXPORT Plugin {
public:
	Plugin(const std::string &path);
	~Plugin();

	static Plugin *load(const std::string &plugin);
	static std::string functionName(const std::string &name, int signature);

	bool call(const std::string &function, int signature, Cursor *cursor);

	std::string getPath() const;

protected:
	Plugin(const Plugin &other) = delete;
	Plugin &operator =(const Plugin &other) = delete;
#ifdef _WIN32
	/// \todo Windows handle_type
#else
	typedef void *handle_type;
#endif
	typedef void (*function_type)(Cursor *cursor);

	function_type getFunction(const std::string &name);

private:
	std::string m_path;
	handle_type m_handle;
};

}

#endif // PLUGIN_H
