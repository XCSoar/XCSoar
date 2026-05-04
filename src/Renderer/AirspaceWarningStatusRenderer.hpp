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
  /**
   * The Cleared* variants apply to an airspace the pilot has a
   * clearance for, the Covered* ones to an airspace whose warning is
   * suppressed by another airspace's clearance.  Both are drawn in
   * the clearance colour, but each carries its own caption so the
   * wording may differ.
   */
  enum class Kind : uint8_t {
    None,
    Inside,
    Near,
    ClearedInside,
    ClearedNear,
    Cleared,
    CoveredInside,
    CoveredNear,
    Covered,
  } kind = Kind::None;

  /**
   * Has the warning no valid "ACK"?  Ignored for the Cleared* and
   * Covered* kinds: a clearance suppresses the acknowledgement
   * distinction.
   */
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
