#ifndef GENERATOR_H
#define GENERATOR_H

#include "scheduler/process.h"

namespace mint {

class MINT_EXPORT Generator : public Process {
public:
	Generator(std::unique_ptr<SavedState> state, Process *process);
	~Generator() override;

	void setup() override;
	void cleanup() override;
	bool collectOnExit() const override;

private:
	std::unique_ptr<SavedState> m_state;
};

MINT_EXPORT bool is_generator(Process *process);

}

#endif // GENERATOR_H
