#include "ast/node.h"

using namespace mint;

Node::Node(Command command) :
	command(command) {

}

Node::Node(int parameter) :
	parameter(parameter) {

}

Node::Node(Symbol *symbol) :
	symbol(symbol) {

}

Node::Node(Reference *constant) :
	constant(constant) {

}
