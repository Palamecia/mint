/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#include "mint/ast/class_register.h"
#include "mint/ast/node.h"
#include "mint/ast/module.h"

#include <cstddef>
#include <unordered_set>
#include <functional>
#include <optional>
#include <deque>
#include <list>
#include <utility>
#include <vector>

namespace mint {

class Module;
class BuildContext;

class Branch {
public:
	using ForwardNodeIndex = std::list<std::size_t>;
	using BackwardNodeIndex = std::size_t;

	Branch() = default;
	Branch(const Branch&) = default;
	Branch(Branch&&) = default;
	virtual ~Branch() = default;

	Branch& operator=(const Branch&) = default;
	Branch& operator=(Branch&&) = default;

	virtual void push_node(const Node& node) = 0;
	virtual void push_nodes(const std::vector<Node>& nodes) = 0;
	virtual void replace_node(std::size_t offset, const Node& node) = 0;
	[[nodiscard]] virtual std::size_t next_node_offset() const = 0;
	[[nodiscard]] virtual Node& node_at(std::size_t offset) = 0;

	virtual void on_new_line(std::size_t offset, std::size_t line_number) = 0;
	virtual void on_new_line(std::size_t line_number) = 0;

	virtual void build() = 0;

	void set_pending_new_line(std::size_t line_number);
	void commit_line();

	void start_jump_forward();
	void shift_jump_forward();
	void resolve_jump_forward();
	[[nodiscard]] inline ForwardNodeIndex* next_jump_forward();
	[[nodiscard]] inline ForwardNodeIndex* start_empty_jump_forward();

	void start_jump_backward();
	void resolve_jump_backward();
	void shift_jump_backward();
	[[nodiscard]] inline BackwardNodeIndex* next_jump_backward();

protected:
	std::size_t resolve_labels_offset(Branch& parent);
	void insert_label(std::size_t offset);

	void forward_jumps(Branch& parent, std::size_t offset);

private:
	std::optional<std::function<void()>> _pending_new_line;
	std::deque<ForwardNodeIndex> _jump_forward;
	std::deque<BackwardNodeIndex> _jump_backward;
	std::unordered_set<std::size_t> _labels;
};

Branch::ForwardNodeIndex* Branch::next_jump_forward() {
	return &_jump_forward.back();
}

Branch::ForwardNodeIndex* Branch::start_empty_jump_forward() {
	_jump_forward.emplace_back();
	return &_jump_forward.back();
}

Branch::BackwardNodeIndex* Branch::next_jump_backward() {
	return &_jump_backward.back();
}

class MainBranch : public Branch {
public:
	MainBranch(AbstractSyntaxTree& ast, ModuleInfo& data);

	void push_node(const Node& node) override;
	void push_nodes(const std::vector<Node>& nodes) override;
	void replace_node(std::size_t offset, const Node& node) override;
	[[nodiscard]] std::size_t next_node_offset() const override;
	[[nodiscard]] Node& node_at(std::size_t offset) override;

	void on_new_line(std::size_t offset, std::size_t line_number) override;
	void on_new_line(std::size_t line_number) override;

	void build() override;

private:
#ifdef MINT_BUILD_TYPE_DEBUG
	std::size_t _offset;
#endif
	std::reference_wrapper<AbstractSyntaxTree> _ast;
	std::reference_wrapper<ModuleInfo> _data;
};

class SubBranch : public Branch {
public:
	static constexpr std::size_t tree_base_capacity = 500;

	SubBranch(Branch& parent);

	void push_node(const Node& node) override;
	void push_nodes(const std::vector<Node>& nodes) override;
	void replace_node(std::size_t offset, const Node& node) override;
	[[nodiscard]] std::size_t next_node_offset() const override;
	[[nodiscard]] Node& node_at(std::size_t offset) override;

	void on_new_line(std::size_t offset, std::size_t line_number) override;
	void on_new_line(std::size_t line_number) override;

	void build() override;

private:
	std::vector<std::pair<std::size_t, std::size_t>> _lines;
	std::vector<Node> _tree;
	std::reference_wrapper<Branch> _parent;
};

}

#endif // BRANCH_H
