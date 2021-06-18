#ifndef BUILD_TOOL_H
#define BUILD_TOOL_H

#include "compiler/lexer.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/globaldata.h"

#include <string>
#include <stack>

namespace mint {

class MINT_EXPORT BuildContext {
public:
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

	struct Block {
		enum Type {
			conditional_loop_type,
			custom_range_loop_type,
			range_loop_type,
			switch_type
		};

		Block(Type type);

		Type type;
		ForewardNodeIndex *foreward = nullptr;
		BackwardNodeIndex *backward = nullptr;
		CaseTable *case_table = nullptr;
		size_t retrievePointCount = 0;
	};

	int fastSymbolIndex(const std::string &symbol);

	void openBloc(Block::Type type);
	void closeBloc();

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
	bool createMember(Reference::Flags flags, const std::string &name, Data *value = Reference::alloc<None>());
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

	void setModifiers(Reference::Flags flags);
	Reference::Flags getModifiers() const;

	int next_token(std::string *token);
	MINT_NORETURN void parse_error(const char *error_msg);

protected:
	void pushNode(Symbol *symbol);

	struct Context {
		size_t printers = 0;
		std::list<Block *> blocks;
	};

	struct Definition : public Context {
		Reference *function = nullptr;
		std::stack<Symbol *> parameters;
		std::vector<Symbol *> capture;
		size_t beginOffset = static_cast<size_t>(-1);
		bool variadic = false;
		bool generator = false;
		bool capture_all = false;
		size_t retrievePointCount = 0;
		SymbolMapping<int> fastSymbolIndexes;
	};

	struct Call {
		int argc = 0;
	};

	Block *currentBlock();
	const Block *currentBlock() const;

	Context *currentContext();
	const Context *currentContext() const;

	Definition *currentDefinition();
	const Definition *currentDefinition() const;

private:
	Context m_moduleContext;

	std::stack<PackageData *> m_packages;
	std::stack<ClassDescription *> m_classDescription;
	std::stack<Definition *> m_definitions;
	std::stack<Call *> m_calls;

	std::stack<ForewardNodeIndex> m_jumpForeward;
	std::stack<BackwardNodeIndex> m_jumpBackward;

	int m_nextEnumValue;
	Reference::Flags m_modifiers;
	ClassDescription::Path m_classBase;
};

}

#endif // BUILD_TOOL_H
