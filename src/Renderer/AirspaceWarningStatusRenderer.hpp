// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct PixelRect;
class AirspaceWarning;
class Canvas;
class Font;

/**
 * Inside / Near badge used by the airspace warnings list and the
 * map-items dialog.
 */
struct AirspaceWarningStatusBadge {
  /**
   * Cleared is an airspace the pilot has a clearance for.  The
   * Covered* kinds are a warning suppressed by another airspace's
   * clearance; their caption still says where the aircraft is.
   */
  enum class Kind : uint8_t {
    None,
    Inside,
    Near,
    Cleared,
    CoveredInside,
    CoveredNear,
    Covered,
  } kind = Kind::None;

  /**
   * Has the warning no valid "ACK"?  Ignored for Cleared and the
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
 * Badge for @p warning.  Kind::None when the warning is quiet.
 * A clearance is its own badge.  Coverage keeps the warning colour
 * off and the caption says where the aircraft is.
 */
[[nodiscard]]
AirspaceWarningStatusBadge
AirspaceWarningStatusFrom(const AirspaceWarning &warning) noexcept;

/**
 * Width to reserve at the right of a row for
 * DrawAirspaceWarningStatus().  Uses @p font for the caption measure.
 */
[[nodiscard]]
int
AirspaceWarningStatusWidth(const Font &font) noexcept;

void
DrawAirspaceWarningStatus(Canvas &canvas, const Font &font,
                          PixelRect status_rc,
                          AirspaceWarningStatusBadge status) noexcept;
