#ifndef MINT_BUILDTOOL_H
#define MINT_BUILDTOOL_H

#include "compiler/lexer.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/globaldata.h"

#include <string>
#include <stack>
#include <list>

namespace mint {

class Branch;

struct Block;
struct Context;
struct CaseTable;
struct Definition;

class MINT_EXPORT BuildContext {
public:
	enum BlockType {
		conditional_loop_type,
		custom_range_loop_type,
		range_loop_type,
		switch_type,
		if_type,
		elif_type,
		else_type,
		try_type,
		catch_type
	};

	BuildContext(DataStream *stream, Module::Infos data);
	~BuildContext();

	Lexer lexer;
	Module::Infos data;

	int fastSymbolIndex(const std::string &symbol);
	bool hasReturned() const;

	void openBlock(BlockType type);
	void closeBlock();

	bool isInLoop() const;
	bool isInSwitch() const;
	bool isInRangeLoop() const;
	bool isInFunction() const;
	bool isInGenerator() const;

	void prepareContinue();
	void prepareBreak();
	void prepareReturn();

	void registerRetrievePoint();
	void unregisterRetrievePoint();

	void startCaseLabel();
	void resolveCaseLabel(const std::string &label);
	void setDefaultLabel();
	void buildCaseTable();

	void startJumpForward();
	void blocJumpForward();
	void shiftJumpForward();
	void resolveJumpForward();

	void startJumpBackward();
	void blocJumpBackward();
	void shiftJumpBackward();
	void resolveJumpBackward();

	void startDefinition();
	bool addParameter(const std::string &symbol);
	bool setVariadic();
	void setGenerator();
	void setExitPoint();
	bool saveParameters();
	bool addDefinitionSignature();
	void saveDefinition();
	Data *retrieveDefinition();

	PackageData *currentPackage() const;
	void openPackage(const std::string &name);
	void closePackage();

	void startClassDescription(const std::string &name, Reference::Flags flags = Reference::standard);
	void appendSymbolToBaseClassPath(const std::string &symbol);
	void saveBaseClassPath();
	bool createMember(Reference::Flags flags, Class::Operator op, Data *value = Reference::alloc<None>());
	bool createMember(Reference::Flags flags, const std::string &name, Data *value = Reference::alloc<None>());
	bool updateMember(Reference::Flags flags, Class::Operator op, Data *value = Reference::alloc<None>());
	bool updateMember(Reference::Flags flags, const std::string &name, Data *value = Reference::alloc<None>());
	void resolveClassDescription();

	void startEnumDescription(const std::string &name, Reference::Flags flags = Reference::standard);
	void setCurrentEnumValue(int value);
	int nextEnumValue();
	void resolveEnumDescription();

	void startCall();
	void addToCall();
	void resolveCall();

	void startCapture();
	void resolveCapture();
	bool captureAs(const std::string &symbol);
	bool capture(const std::string &symbol);
	bool captureAll();

	bool hasPrinter() const;
	void openPrinter();
	void closePrinter();
	void forcePrinter();

	void pushNode(Node::Command command);
	void pushNode(int parameter);
	void pushNode(const char *symbol);
	void pushNode(Data *constant);

	void setOperator(Class::Operator op);
	Class::Operator getOperator() const;

	void setModifiers(Reference::Flags flags);
	Reference::Flags getModifiers() const;

	int next_token(std::string *token);
	MINT_NORETURN void parse_error(const char *error_msg);

protected:
	void pushNode(Reference *constant);
	void pushNode(Symbol *symbol);

	void pushBranch(Branch *branch);
	void popBranch();

	struct Call {
		int argc = 0;
	};

	Block *currentBreakableBlock();
	const Block *currentBreakableBlock() const;

	Context *currentContext();
	const Context *currentContext() const;

	Definition *currentDefinition();
	const Definition *currentDefinition() const;

private:
	std::unique_ptr<Context> m_moduleContext;
	Branch *m_branch;

	std::stack<PackageData *, std::vector<PackageData *>> m_packages;
	std::stack<Definition *, std::vector<Definition *>> m_definitions;
	std::stack<Branch *, std::vector<Branch *>> m_branches;
	std::stack<Call *, std::vector<Call *>> m_calls;

	int m_nextEnumValue;
	Class::Operator m_operator;
	Reference::Flags m_modifiers;
	ClassDescription::Path m_classBase;
};

}

#endif // MINT_BUILDTOOL_H
