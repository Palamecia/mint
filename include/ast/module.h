#ifndef MODULE_H
#define MODULE_H

#include "ast/node.h"
#include "debug/debuginfos.h"

#include "system/datastream.h"

#include <string>
#include <vector>
#include <set>

namespace mint {

class parser;

class MINT_EXPORT Module {
public:
	~Module();

	using Id = size_t;
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
	Reference *makeConstant(Data *data);
	const char *makeSymbol(const char *name);

protected:
	friend class AbstractSyntaxTree;
	Module();
	Module(const Module &other) = delete;
	Module &operator =(const Module &other) = delete;

	friend class BuildContext;
	void pushNode(const Node &node);
	void replaceNode(size_t offset, const Node &node);

private:
	struct symbol_comp {
		bool operator ()(const char *left, const char *right) const;
	};

	std::vector<Node> m_tree;
	std::vector<Reference *> m_constants;
	std::set<const char *, symbol_comp> m_symbols;
};

}

#endif // MODULE_H
