// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataGlobals.hpp"
#include "Profile/Current.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Weather/Skysight/Skysight.hpp"
#include "UIGlobals.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Computer/GlideComputer.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
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

  if (backend_components->glide_computer)
    backend_components->glide_computer->SetTerrain(nullptr);

  data_components->terrain.reset();
}

void
DataGlobals::SetTerrain(std::unique_ptr<RasterTerrain> _terrain) noexcept
{
  assert(!data_components->terrain);

  auto &main_window = *CommonInterface::main_window;

  data_components->terrain = std::move(_terrain);
  main_window.SetTerrain(data_components->terrain.get());

  if (backend_components->glide_computer)
    backend_components->glide_computer->SetTerrain(data_components->terrain.get());

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

std::shared_ptr<Skysight>
DataGlobals::GetSkysight()
{
  auto *map = UIGlobals::GetMap();
  return map != nullptr
    ? map->GetSkysight()
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
  WaypointGlue::SetHome(*data_components->waypoints,
                        data_components->terrain.get(),
                        CommonInterface::SetComputerSettings().poi,
                        CommonInterface::SetComputerSettings().team_code,
                        backend_components->device_blackboard.get(),
                        reset);
  WaypointGlue::SaveHome(Profile::map,
                         CommonInterface::GetComputerSettings().poi,
                         CommonInterface::GetComputerSettings().team_code);
}

void
DataGlobals::SetSkysight(std::shared_ptr<Skysight> skysight)
{
  auto *map = UIGlobals::GetMap();
  if (map != nullptr)
    map->SetSkysight(std::move(skysight));
}
