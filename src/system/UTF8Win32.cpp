// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UTF8Win32.hpp"

#ifdef _WIN32

#include <cassert>
#include <stringapiset.h>

std::wstring
UTF8ToWide(std::string_view s) noexcept
{
  if (s.empty())
    return {};

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
