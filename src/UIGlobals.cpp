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

#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Look/Look.hpp"

SingleWindow &
UIGlobals::GetMainWindow()
{
  assert(CommonInterface::main_window != nullptr);

  return *CommonInterface::main_window;
}

GlueMapWindow *
UIGlobals::GetMap()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetMap();
}

GlueMapWindow *
UIGlobals::GetMapIfActive()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetMapIfActive();
}

const DialogSettings &
UIGlobals::GetDialogSettings()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::GetUISettings().dialog;
}

const FormatSettings &
UIGlobals::GetFormatSettings()
{
  return CommonInterface::GetUISettings().format;
}

const Look &
UIGlobals::GetLook()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetLook();
}

const DialogLook &
UIGlobals::GetDialogLook()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetLook().dialog;
}

const IconLook &
UIGlobals::GetIconLook()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetLook().icon;
}

const MapLook &
UIGlobals::GetMapLook()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetLook().map;
}
