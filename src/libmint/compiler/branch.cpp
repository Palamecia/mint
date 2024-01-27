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

#include "branch.h"
#include "mint/compiler/buildtool.h"
#include "mint/ast/module.h"

#if defined(BUILD_TYPE_DEBUG) && defined(MINT_DUMP_ASSEMBLY)
#include "mint/system/terminal.h"
#include "mint/debug/debugtool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include <iostream>
#endif

using namespace mint;

Branch::~Branch() {
	assert(m_jump_backward.empty());
	assert(m_jump_forward.empty());
}

void Branch::set_pending_new_line(size_t line_number) {
	m_pending_new_line = [this, line_number] {
		on_new_line(line_number);
	};
}

void Branch::commit_line() {
	if (m_pending_new_line) {
		std::invoke(m_pending_new_line.value());
		m_pending_new_line = std::nullopt;
	}
}

void Branch::start_jump_forward() {
	m_jump_forward.emplace_back(ForwardNodeIndex({next_node_offset()}));
	m_labels.insert(next_node_offset());
	push_node(0);
}

void Branch::shift_jump_forward() {
	const size_t end = m_jump_forward.size();
	std::swap(m_jump_forward[end - 1], m_jump_forward[end - 2]);
}

void Branch::resolve_jump_forward() {
	
	for (size_t offset : m_jump_forward.back()) {
		replace_node(offset, static_cast<int>(next_node_offset()));
	}
	
	m_jump_forward.pop_back();
}

void Branch::start_jump_backward() {
	m_jump_backward.emplace_back(next_node_offset());
}

void Branch::resolve_jump_backward() {
	m_labels.insert(next_node_offset());
	push_node(static_cast<int>(m_jump_backward.back()));
	m_jump_backward.pop_back();
}

void Branch::shift_jump_backward() {
	const size_t end = m_jump_backward.size();
	std::swap(m_jump_backward[end - 1], m_jump_backward[end - 2]);
}

size_t Branch::resolve_labels_offset(Branch *parent) {
	
	const size_t offset = parent->next_node_offset();

	for (size_t label : m_labels) {
		node_at(label).parameter += static_cast<int>(offset);
		parent->insert_label(offset + label);
	}

	m_labels.clear();
	return offset;
}

void Branch::insert_label(size_t offset) {
	m_labels.insert(offset);
}

MainBranch::MainBranch(BuildContext *context) :
#ifdef BUILD_TYPE_DEBUG
	m_offset(context->data.module->next_node_offset()),
#endif
	m_context(context) {

}

void MainBranch::push_node(const Node &node) {
	m_context->data.module->push_node(node);
}

void MainBranch::push_nodes(const std::vector<Node> &nodes) {
	m_context->data.module->push_nodes(nodes);
}

void MainBranch::replace_node(size_t offset, const Node &node) {
	m_context->data.module->at(offset) = node;
}

size_t MainBranch::next_node_offset() const {
	return m_context->data.module->next_node_offset();
}

Node &MainBranch::node_at(size_t offset) {
	return m_context->data.module->at(offset);
}

void MainBranch::on_new_line(size_t offset, size_t line_number) {
	m_context->data.debug_info->new_line(offset, line_number);
}

void MainBranch::on_new_line(size_t line_number) {
	m_context->data.debug_info->new_line(m_context->data.module, line_number);
}

void MainBranch::build() {

#if defined(BUILD_TYPE_DEBUG) && defined(MINT_DUMP_ASSEMBLY)
	if (m_context->data.id != Module::invalid_id) {
		AbstractSyntaxTree *ast = AbstractSyntaxTree::instance();
		Cursor *cursor = ast->create_cursor(m_context->data.id);
		std::string module_name = ast->get_module_name(m_context->data.module);
		mint::printf(stdout, "## MODULE: %zu (%s)\n", m_context->data.id, module_name.c_str());
		cursor->jmp(m_offset);

		for (size_t offset = cursor->offset(); offset < m_context->data.module->next_node_offset(); offset = cursor->offset()) {
			mint::printf(stdout, "LINE %zu ", m_context->data.debug_info->line_number(offset));
			switch (Node::Command command = cursor->next().command) {
			case Node::module_end:
				dump_command(offset, command, cursor, std::cout);
				cursor->jmp(m_context->data.module->next_node_offset());
				break;
			default:
				dump_command(offset, command, cursor, std::cout);
			}
		}
	}
#endif
}

SubBranch::SubBranch(Branch *parent) :
	m_parent(parent) {
	m_tree.reserve(500);
}

void SubBranch::push_node(const Node &node) {
	m_tree.emplace_back(node);
}

void SubBranch::push_nodes(const std::vector<Node> &nodes) {
	m_tree.insert(m_tree.end(), nodes.begin(), nodes.end());
}

void SubBranch::replace_node(size_t offset, const Node &node) {
	m_tree[offset] = node;
}

size_t SubBranch::next_node_offset() const {
	return m_tree.size();
}

Node &SubBranch::node_at(size_t offset) {
	return m_tree[offset];
}

void SubBranch::on_new_line(size_t offset, size_t line_number) {
	m_lines.emplace_back(std::make_pair(offset, line_number));
}

void SubBranch::on_new_line(size_t line_number) {
	m_lines.emplace_back(std::make_pair(m_tree.size(), line_number));
}

void SubBranch::build() {
	size_t offset = resolve_labels_offset(m_parent);
	for (const auto &line : m_lines) {
		m_parent->on_new_line(offset + line.first, line.second);
	}
	m_parent->push_nodes(m_tree);
	m_tree.clear();
}
