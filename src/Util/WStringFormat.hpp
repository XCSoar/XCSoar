/*
 * Copyright (C) 2010-2015 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WSTRING_FORMAT_HPP
#define WSTRING_FORMAT_HPP

#include "Compiler.h"

#include <stdio.h>

#ifdef WIN32
#include <string.h>
#endif

template<typename... Args>
static inline void
StringFormat(wchar_t *buffer, size_t size, const wchar_t *fmt, Args&&... args)
{
  /* unlike snprintf(), _sntprintf() does not guarantee that the
     destination buffer is terminated */

#ifdef WIN32
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

  _snwprintf(buffer, size, fmt, args...);
}

template<typename... Args>
static inline void
StringFormatUnsafe(wchar_t *buffer, const wchar_t *fmt, Args&&... args)
{
  /* work around a problem in mingw-w64/libstdc++: libstdc++ defines
     __USE_MINGW_ANSI_STDIO=1 and forces mingw to expose the
     POSIX-compatible stdio functions instead of the
     Microsoft-compatible ones, but those have a major problem for us:
     "%s" denotes a "narrow" string, not a "wide" string, and we'd
     need to use "%ls"; this workaround explicitly selects the
     Microsoft-compatible implementation */
  _swprintf(buffer, fmt, args...);
}

#endif
