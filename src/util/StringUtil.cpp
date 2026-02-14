// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StringUtil.hpp"
#include "CharUtil.hxx"
#include "UTF8.hpp"
#include "Compiler.h"

#include <algorithm>

char *
CopyString(char *gcc_restrict dest, size_t dest_size,
           std::string_view src) noexcept
{
  const bool truncated = src.size() >= dest_size;
  if (truncated)
    src = src.substr(0, dest_size - 1);

  char *p = std::copy(src.begin(), src.end(), dest);
  *p = '\0';

  /* If truncation occurred, the cut may have landed inside a
     multi-byte UTF-8 sequence.  Trim any trailing incomplete
     sequence so the result is always valid UTF-8. */
  if (truncated)
    p = CropIncompleteUTF8(dest);

  return p;
}

char *
NormalizeSearchString(char *gcc_restrict dest,
                      std::string_view src) noexcept
{
  char *retval = dest;

  for (const auto ch : src)
    if (IsAlphaNumericASCII(ch))
      *dest++ = ToUpperASCII(ch);

  *dest = '\0';

  return retval;
}
