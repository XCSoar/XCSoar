/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_RESOURCE_LOADER_HPP
#define XCSOAR_RESOURCE_LOADER_HPP

#include "Util/ConstBuffer.hxx"

#ifdef WIN32
#include <windef.h>
#endif

#include <tchar.h>

class ResourceId;

namespace ResourceLoader {
#ifdef WIN32
  void
  Init(HINSTANCE hInstance);
#endif

  typedef ConstBuffer<void> Data;

  Data
  Load(const TCHAR *name, const TCHAR *type);

#ifndef ANDROID
  Data
  Load(ResourceId id);
#endif

#ifdef WIN32
  HBITMAP
  LoadBitmap2(ResourceId id);
#endif
}

#endif
