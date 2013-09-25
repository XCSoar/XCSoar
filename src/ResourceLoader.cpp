/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "ResourceLoader.hpp"
#include "ResourceId.hpp"

#include <assert.h>

#ifdef WIN32

#include <windows.h>

static HINSTANCE ResourceLoaderInstance;

void
ResourceLoader::Init(HINSTANCE hInstance)
{
  assert(ResourceLoaderInstance == NULL);

  ResourceLoaderInstance = hInstance;
}

#else /* !WIN32 */

#include "resource_data.h"

#include <string.h>

#endif /* !WIN32 */

ResourceLoader::Data
ResourceLoader::Load(const TCHAR *name, const TCHAR *type)
{
#ifdef WIN32
  assert(ResourceLoaderInstance != NULL);

  HRSRC resource = ::FindResource(ResourceLoaderInstance, name, type);
  if (resource == NULL)
    return Data::Null();

  DWORD size = ::SizeofResource(ResourceLoaderInstance, resource);
  if (size == 0)
    return Data::Null();

  HGLOBAL handle = ::LoadResource(ResourceLoaderInstance, resource);
  if (handle == NULL)
    return Data::Null();

  LPVOID data = LockResource(handle);
  if (data == NULL)
    return Data::Null();

  return Data(data, size);
#else

  for (unsigned i = 0; !named_resources[i].data.IsNull(); ++i)
    if (_tcscmp(named_resources[i].name, name) == 0)
      return named_resources[i].data;

  return Data::Null();
#endif
}

#ifndef ANDROID

ResourceLoader::Data
ResourceLoader::Load(ResourceId id)
{
#ifdef WIN32
  return Load(MAKEINTRESOURCE((unsigned)id), RT_BITMAP);
#else
  return id;
#endif
}

#endif

#ifdef WIN32
HBITMAP
ResourceLoader::LoadBitmap2(ResourceId id)
{
  return ::LoadBitmap(ResourceLoaderInstance, MAKEINTRESOURCE((unsigned)id));
}
#endif

#ifdef HAVE_AYGSHELL_DLL
#include "OS/AYGShellDLL.hpp"

HBITMAP
ResourceLoader::SHLoadImageResource(ResourceId id)
{
  const AYGShellDLL ayg_shell_dll;
  return ayg_shell_dll.SHLoadImageResource(ResourceLoaderInstance,
                                           (unsigned)id);
}
#endif
