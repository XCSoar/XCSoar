/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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

BasicAllocatedString<wchar_t>
ConvertUTF8ToWide(const char *p) noexcept
{
  assert(p != nullptr);

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

AllocatedString
ConvertWideToUTF8(const wchar_t *p) noexcept
{
  assert(p != nullptr);

  return ConvertFromWide(p, CP_UTF8);
}

AllocatedString
ConvertWideToACP(const wchar_t *p) noexcept
{
  assert(p != nullptr);

  return ConvertFromWide(p, CP_ACP);
}

#endif
