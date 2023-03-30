#include "block.h"

using namespace mint;

Block::Block(BuildContext::BlockType type) :
	type(type) {

}

bool Block::is_breakable() const {
	switch (type) {
	case BuildContext::conditional_loop_type:
	case BuildContext::custom_range_loop_type:
	case BuildContext::range_loop_type:
	case BuildContext::switch_type:
		return true;
	default:
		break;
	}
	return false;
}

bool Block::is_continuable() const {
	switch (type) {
	case BuildContext::conditional_loop_type:
	case BuildContext::custom_range_loop_type:
	case BuildContext::range_loop_type:
		return true;
	default:
		break;
	}
	return false;
}
