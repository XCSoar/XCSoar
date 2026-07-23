// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OverlayMapItem.hpp"
#include "Language/Language.hpp"
#include "MapWindow/Overlay.hpp"

OverlayMapItem::OverlayMapItem(const MapOverlay &_overlay, GeoPoint location)
  :MapItem(Type::OVERLAY),
   label([&_overlay] {
     StaticString<64> value;
     if (!value.SetUTF8(_overlay.GetLabel()))
       value = _("Map overlay");
     return value;
   }())
{
  info.clear();

  char buffer[256] = {};
  if (_overlay.FormatPointInfo(location, buffer, sizeof(buffer)) &&
      !info.SetUTF8(buffer))
    info.clear();
}
