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

#include "Path.hpp"
#include "Compatibility/path.h"
#include "util/StringCompare.hxx"
#include "util/StringAPI.hxx"
#include "util/CharUtil.hxx"

#ifdef _UNICODE
#include "util/ConvertString.hpp"
#endif

#include <algorithm>

#include <cassert>

std::string
Path::ToUTF8() const noexcept
{
  if (*this == nullptr)
    return std::string();

#ifdef _UNICODE
  const WideToUTF8Converter utf8(c_str());
  if (!utf8.IsValid())
    return std::string();

  return (const char *)utf8;
#else
  return c_str();
#endif
}

AllocatedPath
Path::operator+(const_pointer other) const noexcept
{
  assert(*this != nullptr);
  assert(other != nullptr);

  size_t this_length = StringLength(value.c_str());
  size_t other_length = StringLength(other);

  auto result = new char_type[this_length + other_length + 1];
  auto p = std::copy_n(c_str(), this_length, result);
  p = std::copy_n(other, other_length, p);
  *p = SENTINEL;
  return AllocatedPath::Donate(result);
}

#ifdef _WIN32

[[gnu::pure]] [[gnu::nonnull]]
static constexpr bool
IsDrive(Path::const_pointer p) noexcept
{
  return IsAlphaASCII(p[0]) && p[1] == ':';
}

#endif

bool
Path::IsAbsolute() const noexcept
{
  assert(*this != nullptr);

#ifdef _WIN32
  if (IsDrive(c_str()) && IsDirSeparator(c_str()[2]))
    return true;
#endif

  return IsDirSeparator(c_str()[0]);
}

bool
Path::IsBase() const noexcept
{
  assert(*this != nullptr);

#ifdef _WIN32
  return _tcspbrk(c_str(), _T("/\\")) == nullptr;
#else
  return StringFind(c_str(), _T('/')) == nullptr;
#endif
}

[[gnu::pure]]
static Path::const_pointer
LastSeparator(Path::const_pointer path) noexcept
{
  const auto *p = StringFindLast(path, _T('/'));
#ifdef _WIN32
  const auto *backslash = StringFindLast(path, _T('\\'));
  if (p == nullptr || backslash > p)
    p = backslash;
#endif
  return p;
}

AllocatedPath
Path::GetParent() const noexcept
{
  assert(*this != nullptr);

  const const_pointer v = c_str();
  const const_pointer p = LastSeparator(v);
  if (p == nullptr || p == v)
    return AllocatedPath(_T("."));

  return AllocatedPath(v, p);
}

Path
Path::GetBase() const noexcept
{
  assert(*this != nullptr);

  const_pointer result = c_str();
  const_pointer p = LastSeparator(result);
  if (p != nullptr)
    result = p + 1;

  if (StringIsEmpty(result))
    return nullptr;

  return Path(result);
}

bool
Path::operator==(Path other) const noexcept
{
  return *this == nullptr || other == nullptr
    ? (*this == nullptr && other == nullptr)
    : StringIsEqual(c_str(), other.c_str());
}

Path
Path::RelativeTo(Path parent) const noexcept
{
  assert(*this != nullptr);
  assert(parent != nullptr);

  auto p = StringAfterPrefix(c_str(), parent.c_str());
  return p != nullptr && IsDirSeparator(*p)
    ? Path(p + 1)
    : nullptr;
}

bool
Path::MatchesExtension(const_pointer extension) const noexcept
{
  size_t filename_length = StringLength(c_str());
  size_t extension_length = StringLength(extension);

  return filename_length > extension_length &&
    StringIsEqualIgnoreCase(c_str() + filename_length - extension_length,
                            extension);
}

Path::const_pointer
Path::GetExtension() const noexcept
{
  auto base = GetBase();
  if (base == nullptr)
    return nullptr;

  assert(!StringIsEmpty(base.c_str()));

  return StringFindLast(base.c_str() + 1, _T('.'));
}

AllocatedPath
Path::WithExtension(const_pointer new_extension) const noexcept
{
  assert(new_extension != nullptr);
  assert(*new_extension == _T('.'));

  auto old_extension = GetExtension();
  return old_extension != nullptr
    ? AllocatedPath(c_str(), old_extension) + new_extension
    : *this + new_extension;
}

AllocatedPath
AllocatedPath::Build(string_view a, string_view b) noexcept
{
  auto result = new char_type[a.size() + 1 + b.size() + 1];
  auto p = std::copy(a.begin(), a.end(), result);
  *p++ = DIR_SEPARATOR;
  p = std::copy(b.begin(), b.end(), p);
  *p = SENTINEL;
  return Donate(result);
}
