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

	enum LoopType {
		conditional_loop,
		custom_range_loop,
		range_loop
	};

	void beginLoop(LoopType type);
	void endLoop();
	bool isInLoop() const;
	void prepareReturn();

	void startJumpForward();
	void loopJumpForward();
	void shiftJumpForward();
	void resolveJumpForward();

	void startJumpBackward();
	void loopJumpBackward();
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
	struct Loop {
		LoopType type;
		std::list<size_t> *forward;
		size_t *backward;
	};

	struct Definition {
		Reference *function;
		std::stack<std::string> parameters;
		std::list<std::string> capture;
		std::list<Loop> loops;
		int beginOffset;
		bool variadic;
		bool capture_all;
	};

	std::list<Loop> &loops();
	const std::list<Loop> &loops() const;

private:
	std::stack<PackageData *> m_packages;
	ClassDescription::Path m_classBase;
	std::stack<ClassDescription *> m_classDescription;
	int m_nextEnumValue;

	std::stack<std::list<size_t>> m_jumpForward;
	std::stack<size_t> m_jumpBackward;
	std::list<Loop> m_loops;

	std::stack<Definition *> m_definitions;
	std::stack<int> m_calls;

	Reference::Flags m_modifiers;
};

}

#endif // BUILD_TOOL_H
