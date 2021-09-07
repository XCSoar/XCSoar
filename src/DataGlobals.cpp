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

#include "DataGlobals.hpp"
#include "Profile/Current.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Computer/GlideComputer.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "MainWindow.hpp"
#include "PageActions.hpp"

void
DataGlobals::UnsetTerrain() noexcept
{
  auto &main_window = *CommonInterface::main_window;

  /* just in case the bottom widget uses the old terrain object
     (e.g. the cross section) */
  main_window.SetBottomWidget(nullptr);

  main_window.SetTerrain(nullptr);
  glide_computer->SetTerrain(nullptr);

  delete terrain;
  terrain = nullptr;
}

void
DataGlobals::SetTerrain(std::unique_ptr<RasterTerrain> _terrain) noexcept
{
  assert(!terrain);

  auto &main_window = *CommonInterface::main_window;

  terrain = _terrain.release();
  main_window.SetTerrain(terrain);
  glide_computer->SetTerrain(terrain);

  /* re-create the bottom widget if it was deleted by
     UnsetTerrain() */
  PageActions::Update();
}

std::shared_ptr<RaspStore>
DataGlobals::GetRasp() noexcept
{
  auto *map = UIGlobals::GetMap();
  return map != nullptr
    ? map->GetRasp()
    : nullptr;
}

void
DataGlobals::SetRasp(std::shared_ptr<RaspStore> rasp) noexcept
{
  auto &state = CommonInterface::SetUIState().weather;
  if (state.map >= int(rasp->GetItemCount()))
    state.map = -1;

  auto *map = UIGlobals::GetMap();
  if (map != nullptr)
    map->SetRasp(std::move(rasp));
}

void
DataGlobals::UpdateHome(bool reset) noexcept
{
    WaypointGlue::SetHome(way_points, terrain,
                          CommonInterface::SetComputerSettings().poi,
                          CommonInterface::SetComputerSettings().team_code,
                          device_blackboard, reset);
    WaypointGlue::SaveHome(Profile::map,
                           CommonInterface::GetComputerSettings().poi,
                           CommonInterface::GetComputerSettings().team_code);
}
