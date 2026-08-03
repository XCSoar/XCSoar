// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
