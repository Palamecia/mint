#ifndef BRANCH_H
#define BRANCH_H

#include "ast/node.h"

#include <unordered_set>
#include <deque>
#include <list>

namespace mint {

class Module;
class BuildContext;

class Branch {
public:
	using ForwardNodeIndex = std::list<size_t>;
	using BackwardNodeIndex = size_t;

	virtual ~Branch();

	virtual void pushNode(const Node &node) = 0;
	virtual void pushNodes(const std::vector<Node> &nodes) = 0;
	virtual void replaceNode(size_t offset, const Node &node) = 0;
	virtual size_t nextNodeOffset() const = 0;
	virtual Node &nodeAt(size_t offset) = 0;

	virtual void build() = 0;

	void startJumpForward();
	void shiftJumpForward();
	void resolveJumpForward();
	inline ForwardNodeIndex *nextJumpForward();
	inline ForwardNodeIndex *startEmptyJumpForward();

	void startJumpBackward();
	void resolveJumpBackward();
	void shiftJumpBackward();
	inline BackwardNodeIndex *nextJumpBackward();

protected:
	void resolveLabelsOffset(Branch *parent);
	void insertLabel(size_t offset);

private:
	std::deque<ForwardNodeIndex> m_jumpForward;
	std::deque<BackwardNodeIndex> m_jumpBackward;
	std::unordered_set<size_t> m_labels;
};

Branch::ForwardNodeIndex *Branch::nextJumpForward() {
	return &m_jumpForward.back();
}

Branch::ForwardNodeIndex *Branch::startEmptyJumpForward() {
	m_jumpForward.emplace_back(ForwardNodeIndex());
	return &m_jumpForward.back();
}

Branch::BackwardNodeIndex *Branch::nextJumpBackward() {
	return &m_jumpBackward.back();
}

class MainBranch  : public Branch {
public:
	MainBranch(BuildContext *context);

	void pushNode(const Node &node) override;
	void pushNodes(const std::vector<Node> &nodes) override;
	void replaceNode(size_t offset, const Node &node) override;
	size_t nextNodeOffset() const override;
	Node &nodeAt(size_t offset) override;

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

	void pushNode(const Node &node) override;
	void pushNodes(const std::vector<Node> &nodes) override;
	void replaceNode(size_t offset, const Node &node) override;
	size_t nextNodeOffset() const override;
	Node &nodeAt(size_t offset) override;

	void build() override;

private:
	std::vector<Node> m_tree;
	Branch *m_parent;
};

}

#endif // BRANCH_H
