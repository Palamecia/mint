#include "threadentrypoint.h"

using namespace mint;

ThreadEntryPoint::ThreadEntryPoint() {
	pushNode(Node::exit_thread);
}

ThreadEntryPoint *ThreadEntryPoint::instance() {
	return &g_instance;
}
