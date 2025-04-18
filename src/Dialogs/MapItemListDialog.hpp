// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class MapItemList;
struct DialogLook;
struct MapLook;
struct TrafficLook;
struct FinalGlideBarLook;
struct MapSettings;
class Waypoints;
class ProtectedAirspaceWarningManager;

void
ShowMapItemListDialog(const MapItemList &_list,
                      const DialogLook &_dialog_look,
                      const MapLook &_look,
                      const TrafficLook &_traffic_look,
                      const FinalGlideBarLook &_final_glide_look,
                      const MapSettings &_settings,
                      Waypoints *waypoints,
                      ProtectedAirspaceWarningManager *airspace_warnings);

void ShowMapItemListSettingsDialog();
