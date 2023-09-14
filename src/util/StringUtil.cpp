// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StringUtil.hpp"
#include "CharUtil.hxx"
#include "Compiler.h"

#include <algorithm>

char *
CopyString(char *gcc_restrict dest, size_t dest_size,
           std::string_view src) noexcept
{
  if (src.size() >= dest_size)
    src = src.substr(0, dest_size -1);

  char *p = std::copy(src.begin(), src.end(), dest);
  *p = '\0';
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
