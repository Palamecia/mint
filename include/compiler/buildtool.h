#ifndef MINT_BUILDTOOL_H
#define MINT_BUILDTOOL_H

#include "compiler/lexer.h"
#include "ast/classregister.h"
#include "ast/module.h"

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
		catch_type,
		print_type
	};

	BuildContext(DataStream *stream, Module::Infos data);
	~BuildContext();

	Lexer lexer;
	Module::Infos data;

	int fastScopedSymbolIndex(const std::string &symbol);
	int fastSymbolIndex(const std::string &symbol);
	bool hasReturned() const;

	void openBlock(BlockType type);
	void resetScopedSymbols();
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

	void setExceptionSymbol(const std::string &symbol);
	void resetException();

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
	bool addParameter(const std::string &symbol, Reference::Flags flags = Reference::standard);
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

	void startCondition();
	void resolveCondition();

	void pushNode(Node::Command command);
	void pushNode(int parameter);
	void pushNode(const char *symbol);
	void pushNode(Data *constant);

	void setOperator(Class::Operator op);
	Class::Operator getOperator() const;

	void startModifiers(Reference::Flags flags);
	void addModifiers(Reference::Flags flags);
	Reference::Flags retrieveModifiers();

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

	Block *currentContinuableBlock();
	const Block *currentContinuableBlock() const;

	Context *currentContext();
	const Context *currentContext() const;

	Definition *currentDefinition();
	const Definition *currentDefinition() const;

	int findFastSymbolIndex(Symbol *symbol);
	void resetScopedSymbols(const std::vector<Symbol *> *symbols);

private:
	std::unique_ptr<Context> m_moduleContext;
	Branch *m_branch;

	std::stack<PackageData *, std::vector<PackageData *>> m_packages;
	std::stack<Definition *, std::vector<Definition *>> m_definitions;
	std::stack<Branch *, std::vector<Branch *>> m_branches;
	std::stack<Call *, std::vector<Call *>> m_calls;

	int m_nextEnumValue;
	Class::Operator m_operator;
	ClassDescription::Path m_classBase;
	std::stack<Reference::Flags> m_modifiers;
};

}

#endif // MINT_BUILDTOOL_H
