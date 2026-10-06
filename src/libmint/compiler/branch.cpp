/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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
#include "mint/compiler/compiler.h"
#include "mint/program/node.h"
#include "mint/program/module.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

#if defined(MINT_BUILD_TYPE_DEBUG) && defined(MINT_DUMP_ASSEMBLY)
#include "mint/program/program.h"
#include "mint/program/cursor.h"
#include "mint/debug/debug_tools.h"
#include "mint/system/stdio.h"
#include <format>
#include <iostream>
#include <cstdio>
#endif

using namespace mint;

void Branch::set_pending_new_line(std::size_t line_number) {
	_pending_new_line = line_number;
}

void Branch::commit_line() {
	if (_pending_new_line) {
		on_new_line(*std::exchange(_pending_new_line, std::nullopt));
	}
}

void Branch::shift_jump_forward() {
	const std::size_t end = _jump_forward.size();
	std::swap(_jump_forward[end - 1], _jump_forward[end - 2]);
}

void Branch::resolve_jump_forward() {
	assert(!_jump_forward.empty());
	for (const std::size_t offset : *_jump_forward.back()) {
		replace_node(offset, static_cast<int>(next_node_offset()));
	}
	_jump_forward.pop_back();
}

void mint::Branch::resolve_jump_forward(ForwardNodeIndex& forward) {
	const auto it = std::ranges::find(_jump_forward, &forward, &std::unique_ptr<ForwardNodeIndex>::get);
	assert(it != _jump_forward.end());
	for (const std::size_t offset : forward) {
		replace_node(offset, static_cast<int>(next_node_offset()));
	}
	_jump_forward.erase(it);
}

void Branch::start_jump_forward() {
	_jump_forward.emplace_back(std::make_unique<ForwardNodeIndex>(ForwardNodeIndex {next_node_offset()}));
	insert_label(next_node_offset());
	push_node(0);
}

void Branch::start_jump_forward(ForwardNodeIndex& forward) {
	const auto offset = next_node_offset();
	const auto owner = std::ranges::find(_jump_forward, &forward, &std::unique_ptr<ForwardNodeIndex>::get);
	if (owner != _jump_forward.end()) {
		forward.push_back(offset);
		insert_label(offset);
	}
	else {
		insert_label(offset, &forward);
	}
	push_node(0);
}

void Branch::start_jump_backward() {
	_jump_backward.emplace_back(std::make_unique<BackwardNodeIndex>(next_node_offset()));
}

void Branch::push_jump_backward(BackwardNodeIndex& backward) {
	const auto offset = next_node_offset();
	const auto owner = std::ranges::find(_jump_backward, &backward, &std::unique_ptr<BackwardNodeIndex>::get);
	insert_label(offset, nullptr, owner == _jump_backward.end() ? &backward : nullptr);
	push_node(static_cast<int>(backward));
}

void Branch::shift_jump_backward() {
	const std::size_t end = _jump_backward.size();
	std::swap(_jump_backward[end - 1], _jump_backward[end - 2]);
}

void Branch::resolve_jump_backward() {
	assert(!_jump_backward.empty());
	insert_label(next_node_offset());
	push_node(static_cast<int>(*_jump_backward.back()));
	_jump_backward.pop_back();
}

void Branch::track_offset(std::size_t& offset) {
	_offset_relocations.push_back(&offset);
}

std::size_t Branch::resolve_labels_offset(Branch& parent) {

	const std::size_t offset = parent.next_node_offset();

	for (const auto& [label, relocation] : _labels) {
		const auto parent_offset = offset + label;
		if (relocation.backward) {
			const auto owner = std::ranges::find(parent._jump_backward, relocation.backward,
			    &std::unique_ptr<BackwardNodeIndex>::get);
			parent.insert_label(parent_offset, nullptr,
			    owner == parent._jump_backward.end() ? relocation.backward : nullptr);
			continue;
		}

		node_at(label).parameter += static_cast<int>(offset);
		if (!relocation.forward) {
			parent.insert_label(parent_offset);
			continue;
		}

		const auto owner = std::ranges::find(parent._jump_forward, relocation.forward,
		    &std::unique_ptr<ForwardNodeIndex>::get);
		if (owner != parent._jump_forward.end()) {
			relocation.forward->push_back(parent_offset);
			parent.insert_label(parent_offset);
		}
		else {
			parent.insert_label(parent_offset, relocation.forward);
		}
	}

	_labels.clear();
	for (auto* offset_reference : _offset_relocations) {
		*offset_reference += offset;
		parent._offset_relocations.push_back(offset_reference);
	}
	_offset_relocations.clear();
	return offset;
}

void Branch::insert_label(std::size_t offset, ForwardNodeIndex* forward, BackwardNodeIndex* backward) {
	const auto [label, inserted] = _labels.emplace(offset, Label {
	                                                           .forward = forward,
	                                                           .backward = backward,
	                                                       });
	assert(inserted || (!label->second.forward || !forward || label->second.forward == forward));
	assert(inserted || (!label->second.backward || !backward || label->second.backward == backward));
	if (!inserted) {
		if (!label->second.forward) {
			label->second.forward = forward;
		}
		if (!label->second.backward) {
			label->second.backward = backward;
		}
	}
}

void Branch::forward_jumps(Branch& parent, std::size_t offset) {

	for (auto& jump_forward : _jump_forward) {
		for (auto& jump : *jump_forward) {
			jump += offset;
		}
		parent._jump_forward.push_back(std::move(jump_forward));
	}

	_jump_forward.clear();

	for (auto& jump_backward : _jump_backward) {
		*jump_backward += offset;
		parent._jump_backward.push_back(std::move(jump_backward));
	}

	_jump_backward.clear();
}

MainBranch::MainBranch(Compiler& compiler) :
#ifdef MINT_BUILD_TYPE_DEBUG
    _offset(compiler.data().bytecode.next_node_offset()),
#endif
    _compiler(compiler) {
}

void MainBranch::push_node(const Node& node) {
	_compiler.get().push_node(node);
}

void MainBranch::push_nodes(const std::vector<Node>& nodes) {
	_compiler.get().push_nodes(nodes);
}

void MainBranch::replace_node(std::size_t offset, const Node& node) {
	_compiler.get().data().bytecode.node_at(offset) = node;
}

std::size_t MainBranch::next_node_offset() const {
	return _compiler.get().data().bytecode.next_node_offset();
}

Node& MainBranch::node_at(std::size_t offset) {
	return _compiler.get().data().bytecode.node_at(offset);
}

void MainBranch::on_new_line(std::size_t offset, std::size_t line_number) {
	_compiler.get().data().debug_info.new_line(offset, line_number);
}

void MainBranch::on_new_line(std::size_t line_number) {
	_compiler.get().data().debug_info.new_line(_compiler.get().data().bytecode, line_number);
}

void MainBranch::build() {
#if defined(MINT_BUILD_TYPE_DEBUG) && defined(MINT_DUMP_ASSEMBLY)
	if (_compiler.get().data().id != Module::invalid_id) {
		const auto& bytecode = _compiler.get().data().bytecode;
		auto cursor = Cursor(_compiler.get().program(), bytecode);
		std::cout << std::format("## MODULE: {} ({})\n", _compiler.get().data().id,
		    _compiler.get().data().description.get_path().to_string());
		cursor.jmp(_offset);
		for (std::size_t offset = cursor.offset(); offset < bytecode.next_node_offset(); offset = cursor.offset()) {
			std::cout << std::format("LINE {} ", _compiler.get().data().debug_info.line_number(offset));
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
