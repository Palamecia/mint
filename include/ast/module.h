#ifndef MODULE_H
#define MODULE_H

#include "ast/node.h"
#include "ast/debuginfos.h"

#include "system/datastream.h"

#include <string>
#include <vector>
#include <map>

namespace yy {
class parser;
}

class Module {
public:
	Module();
	~Module();

	typedef size_t Id;
	static constexpr Id MainId = 0;

	struct Infos {
		Id id;
		Module *module;
		DebugInfos *debugInfos;
	};

	Node &at(size_t idx);
	size_t end() const;
	char *makeSymbol(const char *name);
	Reference *makeConstant(Data *data);

protected:
	void pushNode(const Node &node);
	void replaceNode(size_t offset, const Node &node);
	size_t nextNodeOffset() const;
	friend class DebugInfos;
	friend class BuildContext;
	friend class yy::parser;

private:
	std::vector<Node> m_tree;
	std::vector<char *> m_symbols;
	std::vector<Reference *> m_constants;
};

#endif // MODULE_H
