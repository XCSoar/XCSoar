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

#include "DataGlobals.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "UIGlobals.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Interface.hpp"

std::shared_ptr<RaspStore>
DataGlobals::GetRasp()
{
  auto *map = UIGlobals::GetMap();
  return map != nullptr
    ? map->GetRasp()
    : nullptr;
}

void
DataGlobals::SetRasp(std::shared_ptr<RaspStore> rasp)
{
  auto &state = CommonInterface::SetUIState().weather;
  if (state.map >= int(rasp->GetItemCount()))
    state.map = -1;

  auto *map = UIGlobals::GetMap();
  if (map != nullptr)
    map->SetRasp(std::move(rasp));
}
