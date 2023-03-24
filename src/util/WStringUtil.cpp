// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WStringUtil.hpp"
#include "WCharUtil.hxx"
#include "Compiler.h"

#include <algorithm>

wchar_t *
CopyString(wchar_t *gcc_restrict dest, size_t dest_size,
           std::wstring_view src) noexcept
{
  if (src.size() >= dest_size)
    src = src.substr(0, dest_size -1);

  wchar_t *p = std::copy(src.begin(), src.end(), dest);
  *p = L'\0';
  return p;
}

wchar_t *
NormalizeSearchString(wchar_t *gcc_restrict dest,
                      std::wstring_view src) noexcept
{
  wchar_t *retval = dest;

  for (const auto ch : src)
    if (IsAlphaNumericASCII(ch))
      *dest++ = ToUpperASCII(ch);

  *dest = L'\0';

  return retval;
}
