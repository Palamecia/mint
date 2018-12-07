#include "debugger.h"

int main(int argc, char **argv) {
	Debugger debuger(argc, argv);
	return debuger.run();
}
