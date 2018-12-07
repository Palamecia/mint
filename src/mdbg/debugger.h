#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <debug/debuginterface.h>
#include <scheduler/scheduler.h>

class Debugger : public mint::DebugInterface {
public:
	Debugger(int argc, char **argv);

	int run();

protected:
	bool parseArguments(int argc, char **argv, std::vector<char *> &args);
	bool parseArgument(int argc, int &argn, char **argv, std::vector<char *> &args);

	void printCommands();
	void printVersion();
	void printHelp();

	bool check(mint::CursorDebugger *cursor) override;

private:
	std::unique_ptr<mint::Scheduler> m_scheduler;
};

#endif // DEBUGGER_H
