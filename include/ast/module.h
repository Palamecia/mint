#ifndef MINT_MODULE_H
#define MINT_MODULE_H

#include "ast/node.h"
#include "debug/debuginfos.h"

#include <string>
#include <vector>
#include <set>

namespace mint {

class parser;
class PackageData;

class MINT_EXPORT Module {
public:
	~Module();

	using Id = size_t;
#ifdef OS_UNIX
	static constexpr const Id invalid_id = static_cast<size_t>(-1);
	static constexpr const Id main_id = 0;
#else
	static const Id invalid_id = static_cast<size_t>(-1);
	static const Id main_id = 0;
#endif

	struct Infos {
		Id id = invalid_id;
		Module *module = nullptr;
		DebugInfos *debugInfos = nullptr;
		bool loaded = false;
	};

	struct Handle {
		Id module;
		size_t offset;
		PackageData *package;
		size_t fastCount;
		bool generator;
		bool symbols;
	};

	inline Node &at(size_t idx);
	inline size_t end() const;
	inline size_t nextNodeOffset() const;

	Handle *findHandle(Id module, size_t offset) const;
	Handle *makeHandle(PackageData *package, Id module, size_t offset);
	Handle *makeBuiltinHandle(PackageData *package, Id module, size_t offset);

	Reference *makeConstant(Data *data);
	Symbol *makeSymbol(const char *name);

protected:
	friend class AbstractSyntaxTree;
	Module();
	Module(const Module &other) = delete;
	Module &operator =(const Module &other) = delete;

	friend class MainBranch;
	friend class BubBranch;
	void pushNode(const Node &node);
	void pushNodes(const std::vector<Node> &nodes);
	void pushNodes(const std::initializer_list<Node> &nodes);
	void replaceNode(size_t offset, const Node &node);

private:
	std::vector<Node> m_tree;
	std::vector<Handle *> m_handles;
	std::vector<Reference *> m_constants;
	std::map<std::string, Symbol *> m_symbols;
};

Node &Module::at(size_t idx) { return m_tree[idx]; }
size_t Module::end() const { return m_tree.size() - 1; }
size_t Module::nextNodeOffset() const { return m_tree.size(); }

}

#endif // MINT_MODULE_H
