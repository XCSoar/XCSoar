// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StringAPI.hxx"

namespace WeatherMapOverlay {

enum class OverlayInputAction {
  NONE,
  TIME_PLUS,
  TIME_MINUS,
  ALTITUDE_PLUS,
  ALTITUDE_MINUS,
  TIME_AUTO_TOGGLE,
  TIME_AUTO_ON,
  TIME_AUTO_OFF,
  TIME_AUTO_SHOW,
  ALTITUDE_AUTO_TOGGLE,
  ALTITUDE_AUTO_ON,
  ALTITUDE_AUTO_OFF,
  ALTITUDE_AUTO_SHOW,
};

/**
 * Parse a ``WeatherOverlay`` misc argument.
 *
 * Step commands use a common ``<axis> +/-`` form: ``time +``,
 * ``time -``, ``altitude +``, ``altitude -``. EDL accepts ``level``
 * as an alias for ``altitude``. Auto/manual commands use
 * ``<axis> auto …`` (``toggle``, ``on``, ``off``, ``show``).
 */
[[gnu::pure]]
inline OverlayInputAction
ParseOverlayInputAction(const char *misc) noexcept
{
  if (misc == nullptr)
    return OverlayInputAction::NONE;

  if (StringIsEqual(misc, "time +"))
    return OverlayInputAction::TIME_PLUS;
  if (StringIsEqual(misc, "time -"))
    return OverlayInputAction::TIME_MINUS;

  if (StringIsEqual(misc, "altitude +") || StringIsEqual(misc, "level +"))
    return OverlayInputAction::ALTITUDE_PLUS;
  if (StringIsEqual(misc, "altitude -") || StringIsEqual(misc, "level -"))
    return OverlayInputAction::ALTITUDE_MINUS;

  if (StringIsEqual(misc, "time auto toggle"))
    return OverlayInputAction::TIME_AUTO_TOGGLE;
  if (StringIsEqual(misc, "time auto on"))
    return OverlayInputAction::TIME_AUTO_ON;
  if (StringIsEqual(misc, "time auto off"))
    return OverlayInputAction::TIME_AUTO_OFF;
  if (StringIsEqual(misc, "time auto show"))
    return OverlayInputAction::TIME_AUTO_SHOW;

  if (StringIsEqual(misc, "altitude auto toggle") ||
      StringIsEqual(misc, "level auto toggle"))
    return OverlayInputAction::ALTITUDE_AUTO_TOGGLE;
  if (StringIsEqual(misc, "altitude auto on") ||
      StringIsEqual(misc, "level auto on"))
    return OverlayInputAction::ALTITUDE_AUTO_ON;
  if (StringIsEqual(misc, "altitude auto off") ||
      StringIsEqual(misc, "level auto off"))
    return OverlayInputAction::ALTITUDE_AUTO_OFF;
  if (StringIsEqual(misc, "altitude auto show") ||
      StringIsEqual(misc, "level auto show"))
    return OverlayInputAction::ALTITUDE_AUTO_SHOW;

  return OverlayInputAction::NONE;
}

} // namespace WeatherMapOverlay
