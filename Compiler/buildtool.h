#ifndef BUILD_TOOL_H
#define BUILD_TOOL_H

#include "Compiler/lexer.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"
#include "Memory/globaldata.h"

#include <string>

class BuildContext {
public:
	BuildContext(DataStream *stream, Module::Context data);

	Lexer lexer;
	Module::Context data;

	void beginLoop();
	void endLoop();
	bool isInLoop() const;

	void startJumpForward();
	void loopJumpForward();
	void shiftJumpForward();
	void resolveJumpForward();

	void startJumpBackward();
	void loopJumpBackward();
	void shiftJumpBackward();
	void resolveJumpBackward();

	void startDefinition();
	void addParameter(const std::string &symbol);
	void setVariadic();
	void saveParameters();
	void addDefinitionSignature();
	void saveDefinition();
	Data *retriveDefinition();

	void startClassDescription(const std::string &name);
	void classInheritance(const std::string &parent);
	void addMember(Reference::Flags flags, const std::string &name, Data *value = Reference::alloc<Data>());
	void resolveClassDescription();

	void startCall();
	void addToCall();
	void resolveCall();

	void pushInstruction(Instruction::Command command);
	void pushInstruction(int parameter);
	void pushInstruction(const char *symbol);
	void pushInstruction(Data *constant);

	void setModifiers(Reference::Flags flags);
	Reference::Flags getModifiers() const;

	void parse_error(const char *error_msg);

private:
	struct Definition {
		Reference *function;
		std::stack<std::string> parameters;
		int beginOffset;
		bool variadic;
	};

	std::stack<Definition *> m_definitions;
	std::stack<int> m_calls;

	std::stack<ClassDescription> m_classDescription;

	std::stack<std::list<size_t>> m_jumpForward;
	std::stack<size_t> m_jumpBackward;

	struct Loop {
		std::list<size_t> *forward;
		size_t *backward;
	};

	std::stack<Loop> m_loops;

	Reference::Flags m_modifiers;
};

#endif // BUILD_TOOL_H
