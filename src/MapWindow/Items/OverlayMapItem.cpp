// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OverlayMapItem.hpp"
#include "MapWindow/Overlay.hpp"

OverlayMapItem::OverlayMapItem(const MapOverlay &_overlay)
  :MapItem(Type::OVERLAY),
   label(_overlay.GetLabel()) {}
