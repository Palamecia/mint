#include "scheduler/generator.h"
#include "scheduler/processor.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/savedstate.h"

using namespace mint;
using namespace std;

Generator::Generator(unique_ptr<SavedState> state, Process *process) :
	Process(AbstractSyntaxTree::instance().createCursor(process->cursor())),
	m_state(move(state)) {
	setThreadId(process->getThreadId());
}

Generator::~Generator() {

}

void Generator::setup() {
	lock_processor();
	cursor()->restore(move(m_state));
	unlock_processor();
}

void Generator::cleanup() {

}

bool Generator::collectOnExit() const {
	return false;
}

bool mint::is_generator(Process *process) {
	return dynamic_cast<Generator *>(process) != nullptr;
}
