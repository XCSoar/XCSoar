// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConvertString.hpp"

#ifdef _WIN32

#include <windows.h>

std::wstring
UTF8ToWide(std::string_view s) noexcept
{
  try {
    int length = MultiByteToWideChar(CP_UTF8, 0, s.data(), s.size(),
                                      nullptr, 0);
    std::wstring w(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), s.size(), w.data(), length);
    return w;
  }
  catch (...) {
    return std::wstring();
  }
}

std::string
WideToUTF8(std::wstring_view s) noexcept
{
  try {
    int length = WideCharToMultiByte(CP_UTF8, 0, s.data(), s.size(),
                                      nullptr, 0, nullptr, nullptr);
    std::string u(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), s.size(),
                        u.data(), length, nullptr, nullptr);
    return u;
  }
  catch (...) {
    return std::string();
  }
}

#endif
