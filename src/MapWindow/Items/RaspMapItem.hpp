// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapItem.hpp"
#include "Weather/Rasp/RaspRenderer.hpp"

class MapOverlay;

/**
 * A #MapItem describing a #MapOverlay.
 */
struct RaspMapItem : public MapItem
{
  const StaticString<64> label;

  /** The field value at the queried location. */
  const RaspFieldValue value;

  RaspMapItem(const char *_label, const RaspFieldValue &_value)
    :MapItem(Type::RASP), label(_label), value(_value) {}
};
