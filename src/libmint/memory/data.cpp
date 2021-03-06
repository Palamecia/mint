#include "memory/data.h"
#include "memory/garbagecollector.h"

using namespace mint;

Data::Data(Format fmt) :
	format(fmt) {

}

void Data::mark() {
	infos->reachable = true;
}

bool Data::markedBit() const {
	return infos->reachable;
}

None::None() : Data(fmt_none) {

}

Null::Null() : Data(fmt_null) {

}
