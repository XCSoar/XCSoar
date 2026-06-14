// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OverlayMapItem.hpp"
#include "MapWindow/Overlay.hpp"

OverlayMapItem::OverlayMapItem(const MapOverlay &_overlay, GeoPoint location)
  :MapItem(Type::OVERLAY),
   label(_overlay.GetLabel())
{
  detail.clear();
  char buffer[256];
  if (_overlay.FormatPointInfo(location, buffer, sizeof(buffer)))
    detail = buffer;
}
