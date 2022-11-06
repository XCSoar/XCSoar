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

#include "UtilsSystem.hpp"
#include "CommandLine.hpp"
#include "ui/dim/Size.hpp"

#ifdef ANDROID
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#endif

#include <tchar.h>

#ifdef _WIN32
#include <windef.h> // for HWND (needed by winuser.h)
#include <winuser.h>
#endif

#ifndef ANDROID

[[gnu::const]]
static PixelSize
GetWindowDecorationOverhead() noexcept
{
#ifdef _WIN32
  return {
    2 * GetSystemMetrics(SM_CXFIXEDFRAME),
    2 * GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION),
  };
#else
  return {};
#endif
}

#endif

/**
 * Returns the screen dimension rect to be used
 * @return The screen dimension rect to be used
 */
PixelSize
SystemWindowSize() noexcept
{
#ifdef ANDROID
  return native_view->GetSize();
#else
  /// @todo implement this properly for SDL/UNIX
  return PixelSize{ CommandLine::width, CommandLine::height } + GetWindowDecorationOverhead();
#endif
}
