#include "threadentrypoint.h"

ThreadEntryPoint::ThreadEntryPoint() {

	Node node;
	node.command = Node::exit_thread;
	pushNode(node);
}

ThreadEntryPoint *ThreadEntryPoint::instance() {

	static ThreadEntryPoint g_instance;
	return &g_instance;
}
