#include "memory/data.h"
#include "memory/garbagecollector.h"

using namespace mint;

Data::Data(Format fmt) :
	format(fmt) {

}

None::None() : Data(fmt_none) {

}

Null::Null() : Data(fmt_null) {

}
