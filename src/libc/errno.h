#ifndef LIBC_ERRNO_H
#define LIBC_ERRNO_H

#include "platform_specific.h"

LIBC_NAMESPACE_BEGIN

#define errno ::LIBC_NAMESPACE_PREFIX _errno
extern thread_local int _errno;

LIBC_NAMESPACE_END

#endif
