#include "branch.h"
#include "compiler/buildtool.h"
#include "ast/module.h"

#ifdef BUILD_TYPE_DEBUG
#include "debug/debugtool.h"
#include "ast/cursor.h"
#include <iostream>
#endif

using namespace mint;

Branch::~Branch() {
	assert(m_jumpBackward.empty());
	assert(m_jumpForward.empty());
}

void Branch::startJumpForward() {
	m_jumpForward.emplace_back(ForwardNodeIndex({nextNodeOffset()}));
	m_labels.insert(nextNodeOffset());
	pushNode(0);
}

void Branch::shiftJumpForward() {
	const size_t end = m_jumpForward.size();
	std::swap(m_jumpForward[end - 1], m_jumpForward[end - 2]);
}

void Branch::resolveJumpForward() {

	for (size_t offset : m_jumpForward.back()) {
		replaceNode(offset, static_cast<int>(nextNodeOffset()));
	}

	m_jumpForward.pop_back();
}

void Branch::startJumpBackward() {
	m_jumpBackward.emplace_back(nextNodeOffset());
}

void Branch::resolveJumpBackward() {
	m_labels.insert(nextNodeOffset());
	pushNode(static_cast<int>(m_jumpBackward.back()));
	m_jumpBackward.pop_back();
}

void Branch::shiftJumpBackward() {
	const size_t end = m_jumpBackward.size();
	std::swap(m_jumpBackward[end - 1], m_jumpBackward[end - 2]);
}

void Branch::resolveLabelsOffset(Branch *parent) {

	const size_t offset = parent->nextNodeOffset();

	for (size_t label : m_labels) {
		nodeAt(label).parameter += static_cast<int>(offset);
		parent->insertLabel(offset + label);
	}

	m_labels.clear();
}

void Branch::insertLabel(size_t offset) {
	m_labels.insert(offset);
}

MainBranch::MainBranch(BuildContext *context) :
#ifdef BUILD_TYPE_DEBUG
	m_offset(context->data.module->nextNodeOffset()),
#endif
	m_context(context) {

}

void MainBranch::pushNode(const Node &node) {
	m_context->data.module->pushNode(node);
}

void MainBranch::pushNodes(const std::vector<Node> &nodes) {
	m_context->data.module->pushNodes(nodes);
}

void MainBranch::replaceNode(size_t offset, const Node &node) {
	m_context->data.module->at(offset) = node;
}

size_t MainBranch::nextNodeOffset() const {
	return m_context->data.module->nextNodeOffset();
}

Node &MainBranch::nodeAt(size_t offset) {
	return m_context->data.module->at(offset);
}

void MainBranch::build() {

#ifdef BUILD_TYPE_DEBUG
	Cursor *cursor = AbstractSyntaxTree::instance().createCursor(m_context->data.id);
	printf("## MODULE: %zu (%s)\n", m_context->data.id, AbstractSyntaxTree::instance().getModuleName(m_context->data.module).c_str());
	cursor->jmp(m_offset);

	for (size_t offset = cursor->offset(); offset < m_context->data.module->nextNodeOffset(); offset = cursor->offset()) {
		switch (Node::Command command = cursor->next().command) {
		case Node::module_end:
			dump_command(offset, command, cursor, std::cout);
			cursor->jmp(m_context->data.module->nextNodeOffset());
			break;
		default:
			dump_command(offset, command, cursor, std::cout);
		}
	}
#endif
}

SubBranch::SubBranch(Branch *parent) :
	m_parent(parent) {
	m_tree.reserve(500);
}

void SubBranch::pushNode(const Node &node) {
	m_tree.emplace_back(node);
}

void SubBranch::pushNodes(const std::vector<Node> &nodes) {
	m_tree.insert(m_tree.end(), nodes.begin(), nodes.end());
}

void SubBranch::replaceNode(size_t offset, const Node &node) {
	m_tree[offset] = node;
}

size_t SubBranch::nextNodeOffset() const {
	return m_tree.size();
}

Node &SubBranch::nodeAt(size_t offset) {
	return m_tree[offset];
}

void SubBranch::build() {
	resolveLabelsOffset(m_parent);
	m_parent->pushNodes(m_tree);
	m_tree.clear();
}
