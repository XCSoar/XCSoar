// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapItem.hpp"

class MapOverlay;

/**
 * A #MapItem describing a #MapOverlay.
 */
struct RaspMapItem : public MapItem
{
  const StaticString<64> label;

  explicit RaspMapItem(const char *_label)
    :MapItem(Type::RASP), label(_label) {}
};
