#ifndef BUILD_TOOL_H
#define BUILD_TOOL_H

#include "compiler/lexer.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/globaldata.h"

#include <string>
#include <stack>
#include <list>

namespace mint {

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

	Lexer lexer;
	Module::Infos data;

	using ForewardNodeIndex = std::list<size_t>;
	using BackwardNodeIndex = size_t;

	class MINT_EXPORT Branch {
	public:
		Branch(BuildContext *context);

		void startJumpForward();
		void resolveJumpForward();

		void startJumpBackward();
		void resolveJumpBackward();

		void pushNode(Node::Command command);
		void pushNode(int parameter);
		void pushNode(const char *symbol);
		void pushNode(Data *constant);
		void build();

	private:
		BuildContext *m_context;
		std::vector<Node> m_tree;

		std::stack<ForewardNodeIndex> m_jumpForeward;
		std::stack<BackwardNodeIndex> m_jumpBackward;
		std::set<size_t> m_labels;
	};

	struct CaseTable {
		struct Label {
			Label(BuildContext *context);

			size_t offset;
			Branch condition;
		};

		CaseTable(BuildContext *context);

		std::map<std::string, Label> labels;
		size_t *default_label = nullptr;
		std::string current_token;
		Label current_label;
		size_t origin = 0;
	};

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

	void addInclusiveRangeCaseLabel(const std::string &begin, const std::string &end);
	void addExclusiveRangeCaseLabel(const std::string &begin, const std::string &end);
	void addConstantCaseLabel(const std::string &token);
	void addSymbolCaseLabel(const std::string &token);
	void addMemberCaseLabel(const std::string &token);
	void resolveEqCaseLabel();
	void resolveIsCaseLabel();
	void setCaseLabel();
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

	void capture(const std::string &symbol);
	void captureAll();

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
	int fastSymbolIndex(Symbol *symbol);
	void pushNode(Symbol *symbol);

	struct Block {
		Block(BlockType type);

		BlockType type;
		ForewardNodeIndex *foreward = nullptr;
		BackwardNodeIndex *backward = nullptr;
		CaseTable *case_table = nullptr;
		size_t retrievePointCount = 0;
	};

	struct Context {
		std::stack<ClassDescription *> classes;
		std::list<Block *> blocks;
		size_t printers = 0;
		size_t packages = 0;
	};

	struct Definition : public Context {
		SymbolMapping<int> fastSymbolIndexes;
		std::stack<Symbol *> parameters;
		std::vector<Symbol *> capture;
		size_t beginOffset = static_cast<size_t>(-1);
		size_t retrievePointCount = 0;
		Reference *function = nullptr;
		bool variadic = false;
		bool generator = false;
		bool returned = false;
		bool capture_all = false;
	};

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
	Context m_moduleContext;

	std::stack<PackageData *> m_packages;
	std::stack<Definition *> m_definitions;
	std::stack<Call *> m_calls;

	std::stack<ForewardNodeIndex> m_jumpForeward;
	std::stack<BackwardNodeIndex> m_jumpBackward;

	int m_nextEnumValue;
	Class::Operator m_operator;
	Reference::Flags m_modifiers;
	ClassDescription::Path m_classBase;
};

}

#endif // BUILD_TOOL_H
