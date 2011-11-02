/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#endif /* !WIN32 */

ResourceLoader::Data
ResourceLoader::Load(const TCHAR *name, const TCHAR *type)
{
#ifdef WIN32
  assert(ResourceLoaderInstance != NULL);

  HRSRC resource = ::FindResource(ResourceLoaderInstance, name, type);
  if (resource == NULL)
    return Data((const void *)NULL, 0);

  DWORD size = ::SizeofResource(ResourceLoaderInstance, resource);
  if (size == 0)
    return Data((const void *)NULL, 0);

  HGLOBAL handle = ::LoadResource(ResourceLoaderInstance, resource);
  if (handle == NULL)
    return Data((const void *)NULL, 0);

  LPVOID data = LockResource(handle);
  if (data == NULL)
    return Data((const void *)NULL, 0);

  return std::pair<const void *, size_t>(data, size);
#else

  for (unsigned i = 0; named_resources[i].data != NULL; ++i)
    if (_tcscmp(named_resources[i].name, name) == 0)
      return Data(named_resources[i].data, named_resources[i].size);

  return Data((const void *)NULL, 0);
#endif
}

ResourceLoader::Data
ResourceLoader::Load(unsigned id)
{
#ifdef WIN32
  return Load(MAKEINTRESOURCE(id), RT_BITMAP);
#else

  for (unsigned i = 0; numeric_resources[i].data != NULL; ++i)
    if (numeric_resources[i].id == id)
      return Data(numeric_resources[i].data, numeric_resources[i].size);

  return Data((const void *)NULL, 0);
#endif
}

#ifdef WIN32
HBITMAP
ResourceLoader::LoadBitmap2(unsigned id)
{
  return ::LoadBitmap(ResourceLoaderInstance, MAKEINTRESOURCE(id));
}
#endif

#ifdef HAVE_AYGSHELL_DLL
#include "OS/AYGShellDLL.hpp"

HBITMAP
ResourceLoader::SHLoadImageResource(unsigned id)
{
  const AYGShellDLL ayg_shell_dll;
  return ayg_shell_dll.SHLoadImageResource(ResourceLoaderInstance, id);
}
#endif
