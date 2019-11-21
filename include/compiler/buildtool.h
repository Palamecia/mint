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

	typedef std::list<size_t> ForewardNodeIndex;
	typedef size_t BackwardNodeIndex;

	class Branch {
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
		size_t *default_label;
		std::string current_token;
		Label current_label;
		size_t origin;
	};

	struct Bloc {
		enum Type {
			conditional_loop_type,
			custom_range_loop_type,
			range_loop_type,
			switch_type
		};

		Type type;
		ForewardNodeIndex *foreward;
		BackwardNodeIndex *backward;
		CaseTable *case_table;
	};

	void openBloc(Bloc::Type type);
	void closeBloc();
	bool isInLoop() const;
	bool isInSwitch() const;
	bool isInFunction() const;
	void prepareBreak();
	void prepareReturn();

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

	void openPackage(const std::string &name);
	void closePackage();
	PackageData *currentPackage() const;

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

	void pushNode(Node::Command command);
	void pushNode(int parameter);
	void pushNode(const char *symbol);
	void pushNode(Data *constant);

	void setModifiers(Reference::Flags flags);
	Reference::Flags getModifiers() const;

	int next_token(std::string *token);
	void parse_error(const char *error_msg);

protected:
	struct Definition {
		Reference *function;
		std::stack<std::string> parameters;
		std::list<std::string> capture;
		std::list<Bloc> blocs;
		int beginOffset;
		bool variadic;
		bool capture_all;
	};

	struct Call {
		int argc;
	};

	std::list<Bloc> &blocs();
	const std::list<Bloc> &blocs() const;

private:
	std::stack<PackageData *> m_packages;
	ClassDescription::Path m_classBase;
	std::stack<ClassDescription *> m_classDescription;
	int m_nextEnumValue;

	std::stack<ForewardNodeIndex> m_jumpForeward;
	std::stack<BackwardNodeIndex> m_jumpBackward;
	std::list<Bloc> m_blocs;

	std::stack<Definition *> m_definitions;
	std::stack<Call *> m_calls;

	Reference::Flags m_modifiers;
};

}

#endif // BUILD_TOOL_H
