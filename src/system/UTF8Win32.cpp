// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UTF8Win32.hpp"

#ifdef _WIN32

#include "util/UTF8.hpp"

#include <cassert>
#include <stringapiset.h>

std::wstring
UTF8ToWide(std::string_view s) noexcept
{
  if (s.empty())
    return {};

  /* Catch ACP/ANSI bytes mistaken for UTF-8 before the Win32 call
     (#2824). MB_ERR_INVALID_CHARS would also fail; assert early. */
  if (!ValidateUTF8(s)) {
    assert(false);
    return {};
  }

  const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                         s.data(), (int)s.size(),
                                         nullptr, 0);
  if (length <= 0) {
    assert(false);
    return {};
  }

  std::wstring w(length, L'\0');
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                          s.data(), (int)s.size(),
                          w.data(), length) <= 0) {
    assert(false);
    return {};
  }

  return w;
}

std::string
WideToUTF8(std::wstring_view s) noexcept
{
  if (s.empty())
    return {};

  const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                         s.data(), (int)s.size(),
                                         nullptr, 0,
                                         nullptr, nullptr);
  if (length <= 0) {
    assert(false);
    return {};
  }

  std::string out(length, '\0');
  if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                          s.data(), (int)s.size(),
                          out.data(), length,
                          nullptr, nullptr) <= 0) {
    assert(false);
    return {};
  }

  return out;
}

#endif
