/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_MODULE_H
#define MINT_MODULE_H

#include "mint/ast/node.h"
#include "mint/debug/debuginfo.h"

#include <string>
#include <vector>

namespace mint {

class parser;
class PackageData;

class MINT_EXPORT Module {
	friend class AbstractSyntaxTree;
	friend class MainBranch;
	friend class BubBranch;
public:
	~Module();

	using Id = size_t;

	static constexpr const char *INVALID_NAME = "unknown";
	static constexpr const Id INVALID_ID = static_cast<size_t>(-1);

	static constexpr const char *MAIN_NAME = "main";
	static constexpr const Id MAIN_ID = 0;

	enum State {
		NOT_COMPILED,
		NOT_LOADED,
		READY
	};

	struct Info {
		Id id = INVALID_ID;
		Module *module = nullptr;
		DebugInfo *debug_info = nullptr;
		State state = NOT_COMPILED;
	};

	struct Handle {
		Id module;
		size_t offset;
		PackageData *package;
		size_t fast_count;
		bool generator;
		bool symbols;
	};

	inline Node &at(size_t idx);
	inline size_t end() const;
	inline size_t next_node_offset() const;

	Handle *find_handle(Id module, size_t offset) const;
	Handle *make_handle(PackageData *package, Id module, size_t offset);
	Handle *make_builtin_handle(PackageData *package, Id module, size_t offset);

	Reference *make_constant(Data *data);
	Symbol *make_symbol(const char *name);

protected:
	Module();
	Module(const Module &other) = delete;
	Module &operator=(const Module &other) = delete;

	void push_node(const Node &node);
	void push_nodes(const std::vector<Node> &nodes);
	void push_nodes(const std::initializer_list<Node> &nodes);
	void replace_node(size_t offset, const Node &node);

private:
	std::vector<Node> m_tree;
	std::vector<Handle *> m_handles;
	std::vector<Reference *> m_constants;
	std::map<std::string, Symbol *> m_symbols;
};

Node &Module::at(size_t idx) {
	return m_tree[idx];
}

size_t Module::end() const {
	return m_tree.size() - 1;
}

size_t Module::next_node_offset() const {
	return m_tree.size();
}

}

#endif // MINT_MODULE_H
