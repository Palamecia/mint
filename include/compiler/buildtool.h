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

	enum BlocType {
		conditional_loop,
		custom_range_loop,
		range_loop,
		switch_case
	};

	void openBloc(BlocType type);
	void closeBloc();
	bool isInLoop() const;
	bool isInSwitch() const;
	void prepareReturn();

	void setCaseLabel(const std::string &token);
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
	struct Bloc {
		BlocType type;
		std::list<size_t> *forward;
		size_t *backward;
	};

	struct CaseTable {
		std::map<std::string, size_t> labels;
		size_t *default_label;
		size_t origin;
	};

	struct Definition {
		Reference *function;
		std::stack<std::string> parameters;
		std::list<std::string> capture;
		std::list<CaseTable> case_tables;
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

	std::list<CaseTable> &caseTables();
	const std::list<CaseTable> &caseTables() const;

private:
	std::stack<PackageData *> m_packages;
	ClassDescription::Path m_classBase;
	std::stack<ClassDescription *> m_classDescription;
	int m_nextEnumValue;

	std::stack<std::list<size_t>> m_jumpForward;
	std::stack<size_t> m_jumpBackward;
	std::list<CaseTable> m_caseTables;
	std::list<Bloc> m_blocs;

	std::stack<Definition *> m_definitions;
	std::stack<Call *> m_calls;

	Reference::Flags m_modifiers;
};

}

#endif // BUILD_TOOL_H
