#include "./errno.h"

thread_local int LIBC_NAMESPACE_PREFIX _errno = 0;
