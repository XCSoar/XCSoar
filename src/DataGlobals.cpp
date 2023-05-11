// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "Protection.hpp" // for global_running

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

  if (glide_computer != nullptr)
    glide_computer->SetTerrain(terrain);

  /* re-create the bottom widget if it was deleted by
     UnsetTerrain() */
  if (global_running)
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
