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

#ifndef XCSOAR_AIRSPACE_AT_POINT_DIALOG_HPP
#define XCSOAR_AIRSPACE_AT_POINT_DIALOG_HPP

class MapItemList;
struct DialogLook;
struct MapLook;
struct TrafficLook;
struct FinalGlideBarLook;
struct MapSettings;
class ProtectedAirspaceWarningManager;

/*
 * return values of ShowMapItemListDialog, values >= 0 is the index of the selected MapItem
 * MAPITEMLIST_CANCEL  : listdialog escapes
 * MAPITEMLIST_REFRESH : listdialog needs a refresh
 */
const int MAPITEMLIST_CANCEL = -1;
const int MAPITEMLIST_REBUILD = -2;

int
ShowMapItemListDialog(const MapItemList &_list, const DialogLook &_dialog_look,
                      const MapLook &_look, const TrafficLook &_traffic_look,
                      const FinalGlideBarLook &_final_glide_look,
                      const MapSettings &_settings,
                      ProtectedAirspaceWarningManager *airspace_warnings);

void
ShowMapItemListSettingsDialog();

#endif
