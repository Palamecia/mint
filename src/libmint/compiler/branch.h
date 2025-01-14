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

#ifndef BRANCH_H
#define BRANCH_H

#include "mint/ast/node.h"

#include <unordered_set>
#include <functional>
#include <optional>
#include <deque>
#include <list>

namespace mint {

class Module;
class BuildContext;

class Branch {
public:
	using ForwardNodeIndex = std::list<size_t>;
	using BackwardNodeIndex = size_t;

	Branch() = default;
	Branch(const Branch &other) = delete;
	virtual ~Branch();

	Branch &operator=(const Branch &other) = delete;

	virtual void push_node(const Node &node) = 0;
	virtual void push_nodes(const std::vector<Node> &nodes) = 0;
	virtual void replace_node(size_t offset, const Node &node) = 0;
	virtual size_t next_node_offset() const = 0;
	virtual Node &node_at(size_t offset) = 0;

	virtual void on_new_line(size_t offset, size_t line_number) = 0;
	virtual void on_new_line(size_t line_number) = 0;

	virtual void build() = 0;

	void set_pending_new_line(size_t line_number);
	void commit_line();

	void start_jump_forward();
	void shift_jump_forward();
	void resolve_jump_forward();
	inline ForwardNodeIndex *next_jump_forward();
	inline ForwardNodeIndex *start_empty_jump_forward();

	void start_jump_backward();
	void resolve_jump_backward();
	void shift_jump_backward();
	inline BackwardNodeIndex *next_jump_backward();

protected:
	size_t resolve_labels_offset(Branch *parent);
	void insert_label(size_t offset);

private:
	std::optional<std::function<void()>> m_pending_new_line;
	std::deque<ForwardNodeIndex> m_jump_forward;
	std::deque<BackwardNodeIndex> m_jump_backward;
	std::unordered_set<size_t> m_labels;
};

Branch::ForwardNodeIndex *Branch::next_jump_forward() {
	return &m_jump_forward.back();
}

Branch::ForwardNodeIndex *Branch::start_empty_jump_forward() {
	m_jump_forward.emplace_back(ForwardNodeIndex());
	return &m_jump_forward.back();
}

Branch::BackwardNodeIndex *Branch::next_jump_backward() {
	return &m_jump_backward.back();
}

class MainBranch : public Branch {
public:
	MainBranch(BuildContext *context);

	void push_node(const Node &node) override;
	void push_nodes(const std::vector<Node> &nodes) override;
	void replace_node(size_t offset, const Node &node) override;
	size_t next_node_offset() const override;
	Node &node_at(size_t offset) override;

	void on_new_line(size_t offset, size_t line_number) override;
	void on_new_line(size_t line_number) override;

	void build() override;

private:
#ifdef BUILD_TYPE_DEBUG
	size_t m_offset;
#endif
	BuildContext *m_context;
};

class SubBranch : public Branch {
public:
	SubBranch(Branch *parent);

	void push_node(const Node &node) override;
	void push_nodes(const std::vector<Node> &nodes) override;
	void replace_node(size_t offset, const Node &node) override;
	size_t next_node_offset() const override;
	Node &node_at(size_t offset) override;

	void on_new_line(size_t offset, size_t line_number) override;
	void on_new_line(size_t line_number) override;

	void build() override;

private:
	std::vector<std::pair<size_t, size_t>> m_lines;
	std::vector<Node> m_tree;
	Branch *m_parent;
};

}

#endif // BRANCH_H
