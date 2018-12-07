#ifndef MODULE_H
#define MODULE_H

#include "ast/node.h"
#include "debug/debuginfos.h"

#include "system/datastream.h"

#include <string>
#include <vector>
#include <map>

namespace mint {

class parser;

class MINT_EXPORT Module {
public:
	~Module();

	typedef size_t Id;
#ifdef OS_UNIX
	static constexpr const Id main_id = 0;
#else
	static const Id main_id = 0;
#endif

	struct Infos {
		Id id;
		Module *module;
		DebugInfos *debugInfos;
		bool loaded;
	};

	Node &at(size_t idx);
	size_t end() const;
	size_t nextNodeOffset() const;
	char *makeSymbol(const char *name);
	Reference *makeConstant(Data *data);

protected:
	friend class AbstractSyntaxTree;
	Module();
	Module(const Module &other) = delete;
	Module &operator =(const Module &other) = delete;

	friend class BuildContext;
	void pushNode(const Node &node);
	void replaceNode(size_t offset, const Node &node);

private:
	std::vector<Node> m_tree;
	std::vector<char *> m_symbols;
	std::vector<Reference *> m_constants;
};

}

#endif // MODULE_H
