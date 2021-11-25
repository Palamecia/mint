#include "memory/functiontool.h"
#include "system/assert.h"

using namespace std;
using namespace mint;

namespace symbols {
static const Symbol System("System");
static const Symbol OSType("OSType");
static const Symbol Linux("linux");
static const Symbol Windows("windows");
static const Symbol MacOS("mac_os");
}

MINT_FUNCTION(mint_os_get_type, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	ReferenceHelper OSType = helper.reference(symbols::System).member(symbols::OSType);

#if defined (OS_UNIX)
	helper.returnValue(OSType.member(symbols::Linux));
#elif defined (OS_WINDOWS)
	helper.returnValue(OSType.member(symbols::Windows));
#elif defined (OS_MAC)
	helper.returnValue(OSType.member(symbols::MacOS));
#else
	assert_x(false, "mint_os_get_type", "unsuported operating system");
#endif
}
