#ifndef XCSOAR_LOG_FILE_HPP
#define XCSOAR_LOG_FILE_HPP

#include <tchar.h>

#if !defined(NDEBUG) && !defined(GNAV)

void DebugStore(const char *Str, ...);

#else /* NDEBUG */

/* not using an empty inline function here because we don't want to
   evaluate the parameters */
#define DebugStore(...)

#endif /* NDEBUG */

void StartupStore(const TCHAR *Str, ...);

#endif
