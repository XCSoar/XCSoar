// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <stdio.h>

#ifdef _WIN32
#include <string.h>
#endif

template<typename... Args>
static inline int
StringFormat(wchar_t *buffer, size_t size, const wchar_t *fmt,
	     Args&&... args) noexcept
{
  /* unlike snprintf(), _sntprintf() does not guarantee that the
     destination buffer is terminated */

#ifdef _WIN32
  /* usually, it would be enough to clear the last byte in the output
     buffer after the _sntprintf() call, but unfortunately WINE 1.4.1
     has a bug that applies the wrong limit in the overflow branch
     (confuses number of characters with number of bytes), therefore
     we must clear the whole buffer and pass an even number of
     characters; this terminates the string at half the buffer size,
     but is better than exposing undefined bytes */
  size &= ~decltype(size)(sizeof(wchar_t) - 1);
  memset(buffer, 0, size * sizeof(wchar_t));
  --size;
#endif

  return _snwprintf(buffer, size, fmt, args...);
}

template<typename... Args>
static inline int
StringFormatUnsafe(wchar_t *buffer, const wchar_t *fmt,
		   Args&&... args) noexcept
{
  /* work around a problem in mingw-w64/libstdc++: libstdc++ defines
     __USE_MINGW_ANSI_STDIO=1 and forces mingw to expose the
     POSIX-compatible stdio functions instead of the
     Microsoft-compatible ones, but those have a major problem for us:
     "%s" denotes a "narrow" string, not a "wide" string, and we'd
     need to use "%ls"; this workaround explicitly selects the
     Microsoft-compatible implementation */
  return _swprintf(buffer, fmt, args...);
}
