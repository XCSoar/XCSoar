// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UTF8Win.hpp"

#ifdef _WIN32
#include <stringapiset.h>

std::wstring
UTF8ToWide(const std::string_view s) noexcept
{
  try {
    int length = MultiByteToWideChar(CP_UTF8, 0, s.data(), s.size(),
                                    nullptr, 0);
    std::wstring w(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), s.size(), w.data(), length);
    return w;
  }
  catch(...) {
    return std::wstring();
  }
}

#endif
