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

#include "Pan.hpp"
#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Interface.hpp"
#include "PageActions.hpp"
#include "Input/InputEvents.hpp"

#include <assert.h>

bool
IsPanning()
{
  assert(CommonInterface::main_window != NULL);

  const GlueMapWindow *map = CommonInterface::main_window->GetMapIfActive();
  return map != NULL && map->IsPanning();
}

void
EnterPan()
{
  assert(CommonInterface::main_window != NULL);

  GlueMapWindow *map = CommonInterface::main_window->ActivateMap();
  if (map == NULL || map->IsPanning())
    return;

  map->SetPan(true);

  InputEvents::setMode(InputEvents::MODE_PAN);
  CommonInterface::main_window->SetFullScreen(true);
}

bool
PanTo(const GeoPoint &location)
{
  assert(CommonInterface::main_window != NULL);

  GlueMapWindow *map = CommonInterface::main_window->ActivateMap();
  if (map == NULL)
    return false;

  map->PanTo(location);

  InputEvents::setMode(InputEvents::MODE_PAN);
  CommonInterface::main_window->SetFullScreen(true);
  return true;
}

void
LeavePan()
{
  assert(CommonInterface::main_window != NULL);

  GlueMapWindow *map = CommonInterface::main_window->GetMapIfActive();
  if (map == NULL || !map->IsPanning())
    return;

  map->SetPan(false);

  setMode(InputEvents::MODE_DEFAULT);
  Pages::Update();
}

void
TogglePan()
{
  assert(CommonInterface::main_window != NULL);

  const GlueMapWindow *map = CommonInterface::main_window->GetMap();
  if (map == NULL)
    EnterPan();
  else if (map->IsPanning())
    LeavePan();
}
