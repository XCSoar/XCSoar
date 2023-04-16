// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlueMapWindow.hpp"
#include "Items/List.hpp"
#include "Items/Builder.hpp"
#include "Items/OverlayMapItem.hpp"
#include "Items/RaspMapItem.hpp"
#include "Dialogs/MapItemListDialog.hpp"
#include "UIGlobals.hpp"
#include "Screen/Layout.hpp"
#include "Computer/GlideComputer.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"
#include "Weather/Rasp/RaspRenderer.hpp"
#include "net/client/tim/Glue.hpp"
#include "net/client/tim/Thermal.hpp"
#include "Interface.hpp"
#include "Overlay.hpp"

bool GlueMapWindow::ShowMapItems(const GeoPoint &location,
                                 bool show_empty_message,
                                 bool pointer_in_use) const
{
  /* not using MapWindowBlackboard here because this method is called
     by the main thread */
  const ComputerSettings &computer_settings =
    CommonInterface::GetComputerSettings();
  const MapSettings &settings = CommonInterface::GetMapSettings();
  const MoreData &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  int range;
  if (pointer_in_use)
    range = visible_projection.DistancePixelsToMeters(Layout::GetHitRadius());
  else
    /* FastScale 29 is the radius of the shortest point in the cross hair */
    range = visible_projection.DistancePixelsToMeters(Layout::FastScale(29));

  MapItemList list;
  MapItemListBuilder builder(list, location, range);

  if (settings.item_list.add_location)
      builder.AddLocation(basic, terrain);

  if (settings.item_list.add_arrival_altitude && route_planner)
    builder.AddArrivalAltitudes(*route_planner, terrain,
                                computer_settings.task.safety_height_arrival);

  if (basic.location_available)
    builder.AddSelfIfNear(basic.location, basic.attitude.heading);

  if (task)
    builder.AddTaskOZs(*task);

  const Airspaces *airspace_database = airspace_renderer.GetAirspaces();
  if (airspace_database)
    builder.AddVisibleAirspace(*airspace_database,
                               airspace_renderer.GetWarningManager(),
                               computer_settings.airspace,
                               settings.airspace, basic,
                               calculated);

  if (visible_projection.GetMapScale() <= 4000) {
    builder.AddThermals(calculated.thermal_locator, basic, calculated);

    if (tim_glue != nullptr && computer_settings.weather.enable_tim)
      builder.AddThermals(tim_glue->Get());
  }

  if (waypoints)
    builder.AddWaypoints(*waypoints);

#ifdef HAVE_NOAA
  if (noaa_store)
    builder.AddWeatherStations(*noaa_store);
#endif

  builder.AddTraffic(basic.flarm.traffic);

#ifdef HAVE_SKYLINES_TRACKING
  builder.AddSkyLinesTraffic();
#endif

#ifdef ENABLE_OPENGL
  if (!list.full() && overlay && overlay->IsInside(location))
    list.push_back(new OverlayMapItem(*overlay));
#endif

  if (!list.full()) {
#ifndef ENABLE_OPENGL
    const std::lock_guard lock{mutex};
#endif

    if (rasp_renderer && rasp_renderer->IsInside(location))
      list.push_back(new RaspMapItem(rasp_renderer->GetLabel()));
  }

  // Sort the list of map items
  list.Sort();

  // Show the list dialog
  if (list.empty()) {
    if (show_empty_message)
      ShowMessageBox(_("There is nothing interesting near this location."),
                  _("Map elements at this location"), MB_OK | MB_ICONINFORMATION);

    return false;
  }

  ShowMapItemListDialog(list,
                        UIGlobals::GetDialogLook(), look, traffic_look,
                        final_glide_bar_renderer.GetLook(), settings,
                        glide_computer != nullptr
                        ? &glide_computer->GetAirspaceWarnings() : nullptr);
  return true;
}
