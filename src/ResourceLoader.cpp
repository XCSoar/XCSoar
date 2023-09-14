// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ResourceLoader.hpp"
#include "ResourceId.hpp"

#include <cassert>

#ifdef USE_WIN32_RESOURCES

#include <winbase.h>
#include <winuser.h>

static HINSTANCE ResourceLoaderInstance;

void
ResourceLoader::Init(HINSTANCE hInstance)
{
  assert(ResourceLoaderInstance == nullptr);

  ResourceLoaderInstance = hInstance;
}

#else /* !WIN32 */

#include "resource_data.h"
#include "util/StringAPI.hxx"

#endif /* !WIN32 */

ResourceLoader::Data
ResourceLoader::Load(const TCHAR *name, [[maybe_unused]] const TCHAR *type)
{
#ifdef USE_WIN32_RESOURCES
  assert(ResourceLoaderInstance != nullptr);

  HRSRC resource = ::FindResource(ResourceLoaderInstance, name, type);
  if (resource == nullptr)
    return {};

  DWORD size = ::SizeofResource(ResourceLoaderInstance, resource);
  if (size == 0)
    return {};

  HGLOBAL handle = ::LoadResource(ResourceLoaderInstance, resource);
  if (handle == nullptr)
    return {};

  LPVOID data = LockResource(handle);
  if (data == nullptr)
    return {};

  return {(const std::byte *)data, (std::size_t)size};
#else

  for (unsigned i = 0; named_resources[i].data.data() != nullptr; ++i)
    if (StringIsEqual(named_resources[i].name, name))
      return named_resources[i].data;

  return {};
#endif
}

#ifndef ANDROID

ResourceLoader::Data
ResourceLoader::Load(ResourceId id)
{
#ifdef USE_WIN32_RESOURCES
  return Load(MAKEINTRESOURCE((unsigned)id), RT_BITMAP);
#else
  return id;
#endif
}

#endif

#ifdef USE_WIN32_RESOURCES
HBITMAP
ResourceLoader::LoadBitmap2(ResourceId id)
{
  return ::LoadBitmap(ResourceLoaderInstance, MAKEINTRESOURCE((unsigned)id));
}
#endif
