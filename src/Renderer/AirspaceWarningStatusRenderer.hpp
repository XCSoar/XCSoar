// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct PixelRect;
class Canvas;
class Font;

/**
 * Inside / Near badge used by the airspace warnings list and the
 * map-items dialog.
 */
struct AirspaceWarningStatusBadge {
  enum class Kind : uint8_t {
    None,
    Inside,
    Near,
  } kind = Kind::None;

  bool active = true;

  [[nodiscard]]
  bool HasStatus() const noexcept {
    return kind != Kind::None;
  }
};

/**
 * Width to reserve at the right of a row for
 * DrawAirspaceWarningStatus().  Uses @p font for the caption measure.
 */
[[nodiscard]]
int
AirspaceWarningStatusWidth(Canvas &canvas, const Font &font) noexcept;

void
DrawAirspaceWarningStatus(Canvas &canvas, const Font &font,
                          PixelRect status_rc,
                          AirspaceWarningStatusBadge status) noexcept;
