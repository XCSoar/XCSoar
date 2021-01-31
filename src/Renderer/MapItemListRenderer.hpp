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

#ifndef XCSOAR_MAP_ITEM_LIST_RENDERER_HPP
#define XCSOAR_MAP_ITEM_LIST_RENDERER_HPP

#include "time/RoughTime.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"

struct PixelRect;
class Canvas;
struct MapItem;
struct DialogLook;
struct MapLook;
struct TrafficLook;
struct FinalGlideBarLook;
struct MapSettings;
struct TrafficList;

class MapItemListRenderer {
  const MapLook &look;
  const TrafficLook &traffic_look;
  const FinalGlideBarLook &final_glide_look;
  const MapSettings &settings;
  const RoughTimeDelta utc_offset;

  TwoTextRowsRenderer row_renderer;

public:
  MapItemListRenderer(const MapLook &_look,
                      const TrafficLook &_traffic_look,
                      const FinalGlideBarLook &_final_glide_look,
                      const MapSettings &_settings,
                      RoughTimeDelta _utc_offset)
    :look(_look),
     traffic_look(_traffic_look), final_glide_look(_final_glide_look),
     settings(_settings), utc_offset(_utc_offset) {}

  unsigned CalculateLayout(const DialogLook &dialog_look);

  void Draw(Canvas &canvas, const PixelRect rc, const MapItem &item,
            const TrafficList *traffic_list=nullptr);
};

#endif
