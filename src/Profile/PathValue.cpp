// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Map.hpp"
#include "LocalPath.hpp"
#include "system/Path.hpp"
#include "Compatibility/path.h"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/StringPointer.hxx"
#include "util/Macros.hpp"

#ifdef _UNICODE
#include "util/AllocatedString.hxx"
#endif

#include <windef.h> /* for MAX_PATH */

AllocatedPath
ProfileMap::GetPath(const char *key) const noexcept
{
  TCHAR buffer[MAX_PATH];
  if (!Get(key, buffer, ARRAY_SIZE(buffer)))
      return nullptr;

  if (StringIsEmpty(buffer))
    return nullptr;

  return ExpandLocalPath(Path(buffer));
}

bool
ProfileMap::GetPathIsEqual(const char *key, Path value) const noexcept
{
  const auto saved_value = GetPath(key);
  if (saved_value == nullptr)
    return false;

  return saved_value == value;
}

[[gnu::pure]]
static Path
BackslashBaseName(const TCHAR *p) noexcept
{
  if (DIR_SEPARATOR != '\\') {
    const auto *backslash = StringFindLast(p, _T('\\'));
    if (backslash != NULL)
      p = backslash + 1;
  }

  return Path(p).GetBase();
}

#ifdef _UNICODE

BasicAllocatedString<TCHAR>
ProfileMap::GetPathBase(const char *key) const noexcept
{
  TCHAR buffer[MAX_PATH];
  if (!Get(key, buffer, ARRAY_SIZE(buffer)))
      return nullptr;

  const TCHAR *base = BackslashBaseName(buffer).c_str();
  if (base == nullptr)
    return nullptr;

  return BasicAllocatedString<TCHAR>(base);
}

#else

StringPointer<TCHAR>
ProfileMap::GetPathBase(const char *key) const noexcept
{
  const auto *path = Get(key);
  if (path != nullptr)
    path = BackslashBaseName(path).c_str();

  return path;
}

#endif

void
ProfileMap::SetPath(const char *key, Path value) noexcept
{
  if (value == nullptr || StringIsEmpty(value.c_str()))
    Set(key, _T(""));
  else {
    const auto contracted = ContractLocalPath(value);
    if (contracted != nullptr)
      value = contracted;

    Set(key, value.c_str());
  }
}
