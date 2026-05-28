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

#include "branch.h"
#include "mint/ast/node.h"
#include "mint/compiler/build_tools.h"
#include "mint/ast/module.h"
#include <cstddef>
#include <functional>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

#if defined(MINT_BUILD_TYPE_DEBUG) && defined(MINT_DUMP_ASSEMBLY)
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/cursor.h"
#include "mint/debug/debug_tools.h"
#include "mint/system/stdio.h"
#include <format>
#include <iostream>
#include <cstdio>
#endif

using namespace mint;

void Branch::set_pending_new_line(std::size_t line_number) {
	_pending_new_line = [this, line_number] {
		on_new_line(line_number);
	};
}

void Branch::commit_line() {
	if (_pending_new_line) {
		std::invoke(_pending_new_line.value());
		_pending_new_line = std::nullopt;
	}
}

void Branch::start_jump_forward() {
	_jump_forward.emplace_back(ForwardNodeIndex({next_node_offset()}));
	_labels.insert(next_node_offset());
	push_node(0);
}

void Branch::shift_jump_forward() {
	const std::size_t end = _jump_forward.size();
	std::swap(_jump_forward[end - 1], _jump_forward[end - 2]);
}

void Branch::resolve_jump_forward() {

	for (const std::size_t offset : _jump_forward.back()) {
		replace_node(offset, static_cast<int>(next_node_offset()));
	}

	_jump_forward.pop_back();
}

void Branch::start_jump_backward() {
	_jump_backward.emplace_back(next_node_offset());
}

void Branch::resolve_jump_backward() {
	_labels.insert(next_node_offset());
	push_node(static_cast<int>(_jump_backward.back()));
	_jump_backward.pop_back();
}

void Branch::shift_jump_backward() {
	const std::size_t end = _jump_backward.size();
	std::swap(_jump_backward[end - 1], _jump_backward[end - 2]);
}

std::size_t Branch::resolve_labels_offset(Branch& parent) {

	const std::size_t offset = parent.next_node_offset();

	for (const std::size_t label : _labels) {
		node_at(label).parameter += static_cast<int>(offset);
		parent.insert_label(offset + label);
	}

	_labels.clear();
	return offset;
}

void Branch::insert_label(std::size_t offset) {
	_labels.insert(offset);
}

void Branch::forward_jumps(Branch& parent, std::size_t offset) {

	for (const auto& jump_forward : _jump_forward) {
		parent._jump_forward.emplace_back(std::from_range,
		    std::views::transform(jump_forward, [offset](const auto jump) {
			    return jump + offset;
		    }));
	}

	_jump_forward.clear();

	for (const auto jump_backward : _jump_backward) {
		parent._jump_backward.push_back(jump_backward + offset);
	}

	_jump_backward.clear();
}

MainBranch::MainBranch(AbstractSyntaxTree& ast, ModuleInfo& data) :
#ifdef MINT_BUILD_TYPE_DEBUG
    _offset(data.bytecode.next_node_offset()),
#endif
    _ast(ast),
    _data(data) {
}

void MainBranch::push_node(const Node& node) {
	_data.get().bytecode.push_node(node);
}

void MainBranch::push_nodes(const std::vector<Node>& nodes) {
	_data.get().bytecode.push_nodes(nodes);
}

void MainBranch::replace_node(std::size_t offset, const Node& node) {
	_data.get().bytecode.node_at(offset) = node;
}

std::size_t MainBranch::next_node_offset() const {
	return _data.get().bytecode.next_node_offset();
}

Node& MainBranch::node_at(std::size_t offset) {
	return _data.get().bytecode.node_at(offset);
}

void MainBranch::on_new_line(std::size_t offset, std::size_t line_number) {
	_data.get().debug_info.new_line(offset, line_number);
}

void MainBranch::on_new_line(std::size_t line_number) {
	_data.get().debug_info.new_line(_data.get().bytecode, line_number);
}

void MainBranch::build() {
#if defined(MINT_BUILD_TYPE_DEBUG) && defined(MINT_DUMP_ASSEMBLY)
	if (_data.get().id != Module::invalid_id) {
		const auto& bytecode = _data.get().bytecode;
		auto cursor = Cursor(_ast, bytecode);
		mint::print(stdout, std::format("## MODULE: {} ({})\n", _data.get().id, _ast.get().get_module_name(bytecode)));
		cursor.jmp(_offset);
		for (std::size_t offset = cursor.offset(); offset < bytecode.next_node_offset(); offset = cursor.offset()) {
			mint::print(stdout, std::format("LINE {} ", _data.get().debug_info.line_number()(offset)));
			if (dump_command(cursor, std::cout) == Node::Command::exit_module) {
				cursor.jmp(bytecode.next_node_offset());
			}
		}
	}
#endif
}

SubBranch::SubBranch(Branch& parent) :
    _parent(parent) {
	_tree.reserve(tree_base_capacity);
}

void SubBranch::push_node(const Node& node) {
	_tree.emplace_back(node);
}

void SubBranch::push_nodes(const std::vector<Node>& nodes) {
	_tree.insert(_tree.end(), nodes.begin(), nodes.end());
}

void SubBranch::replace_node(std::size_t offset, const Node& node) {
	_tree[offset] = node;
}

std::size_t SubBranch::next_node_offset() const {
	return _tree.size();
}

Node& SubBranch::node_at(std::size_t offset) {
	return _tree[offset];
}

void SubBranch::on_new_line(std::size_t offset, std::size_t line_number) {
	_lines.emplace_back(offset, line_number);
}

void SubBranch::on_new_line(std::size_t line_number) {
	_lines.emplace_back(_tree.size(), line_number);
}

void SubBranch::build() {
	const std::size_t offset = resolve_labels_offset(_parent);
	for (const auto& line : _lines) {
		_parent.get().on_new_line(offset + line.first, line.second);
	}
	_parent.get().push_nodes(_tree);
	forward_jumps(_parent, offset);
	_tree.clear();
}
