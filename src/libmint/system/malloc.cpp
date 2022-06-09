#include "system/malloc.h"

#if defined(OS_WINDOWS)
#include <Windows.h>
#elif defined(OS_MAC)
#include <malloc/malloc.h>
#elif defined(OS_UNIX)
#include <malloc.h>
#endif

size_t mint::malloc_size(void *ptr) {
#if defined(OS_WINDOWS)
	return _msize(ptr);
#elif defined(OS_MAC)
	return ::malloc_size(ptr);
#elif defined(OS_UNIX)
	return malloc_usable_size(ptr);
#else
	return 0;
#endif
}
