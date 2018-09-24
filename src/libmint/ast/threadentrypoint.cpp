#include "threadentrypoint.h"

using namespace mint;

ThreadEntryPoint::ThreadEntryPoint() {

	Node node;
	node.command = Node::exit_thread;
	pushNode(node);
}

ThreadEntryPoint *ThreadEntryPoint::instance() {
	return &g_instance;
}
