// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConvertString.hpp"

#ifdef _UNICODE
#include <stringapiset.h>

static BasicAllocatedString<wchar_t>
ConvertToWide(const char *p, UINT codepage) noexcept
{
  assert(p != nullptr);

  int length = MultiByteToWideChar(codepage, 0, p, -1, nullptr, 0);
  if (length <= 0)
    return nullptr;

  wchar_t *buffer = new wchar_t[length];
  length = MultiByteToWideChar(codepage, 0, p, -1, buffer, length);
  if (length <= 0) {
    delete[] buffer;
    return nullptr;
  }

  return BasicAllocatedString<wchar_t>::Donate(buffer);
}

static BasicAllocatedString<wchar_t>
ConvertToWide(std::string_view p, UINT codepage) noexcept
{
  int length = MultiByteToWideChar(codepage, 0, p.data(), p.size(), nullptr, 0);
  if (length <= 0)
    return nullptr;

  wchar_t *buffer = new wchar_t[length + 1];
  length = MultiByteToWideChar(codepage, 0, p.data(), p.size(), buffer, length);
  if (length <= 0) {
    delete[] buffer;
    return nullptr;
  }

  buffer[length] = 0;
  return BasicAllocatedString<wchar_t>::Donate(buffer);
}

BasicAllocatedString<wchar_t>
ConvertUTF8ToWide(const char *p) noexcept
{
  assert(p != nullptr);

  return ConvertToWide(p, CP_UTF8);
}

BasicAllocatedString<wchar_t>
ConvertUTF8ToWide(std::string_view p) noexcept
{
  return ConvertToWide(p, CP_UTF8);
}

BasicAllocatedString<wchar_t>
ConvertACPToWide(const char *p) noexcept
{
  assert(p != nullptr);

  return ConvertToWide(p, CP_ACP);
}

static AllocatedString
ConvertFromWide(const wchar_t *p, UINT codepage) noexcept
{
  assert(p != nullptr);

  int length = WideCharToMultiByte(codepage, 0, p, -1, nullptr, 0,
                                   nullptr, nullptr);
  if (length <= 0)
    return nullptr;

  char *buffer = new char[length];
  length = WideCharToMultiByte(codepage, 0, p, -1, buffer, length,
                               nullptr, nullptr);
  if (length <= 0) {
    delete[] buffer;
    return nullptr;
  }

  return AllocatedString::Donate(buffer);
}

static AllocatedString
ConvertFromWide(std::wstring_view p, UINT codepage) noexcept
{
  int length = WideCharToMultiByte(codepage, 0, p.data(), p.size(), nullptr, 0,
                                   nullptr, nullptr);
  if (length <= 0)
    return nullptr;

  char *buffer = new char[length + 1];
  length = WideCharToMultiByte(codepage, 0, p.data(), p.size(), buffer, length,
                               nullptr, nullptr);
  if (length <= 0) {
    delete[] buffer;
    return nullptr;
  }

  buffer[length] = 0;
  return AllocatedString::Donate(buffer);
}

AllocatedString
ConvertWideToUTF8(const wchar_t *p) noexcept
{
  assert(p != nullptr);

  return ConvertFromWide(p, CP_UTF8);
}

AllocatedString
ConvertWideToUTF8(std::wstring_view p) noexcept
{
  return ConvertFromWide(p, CP_UTF8);
}

AllocatedString
ConvertWideToACP(const wchar_t *p) noexcept
{
  assert(p != nullptr);

  return ConvertFromWide(p, CP_ACP);
}

#endif
