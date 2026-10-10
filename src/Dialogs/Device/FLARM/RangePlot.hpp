// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "ui/canvas/Pen.hpp"
#include "Renderer/RadarRenderer.hpp"

#include <optional>
#include <vector>

struct DialogLook;
struct FlarmTrafficLook;

/**
 * One direction of a #FlarmRangePlot.  Sector i of n covers the
 * bearings from i/n to (i+1)/n of a full circle, clockwise from the
 * direction of flight.
 */
struct FlarmRangeSector {
  /** the range drawn as the sector [m]; none leaves it out */
  std::optional<unsigned> range;

  /**
   * Are there enough data points behind #range?  If not, the sector
   * is drawn as an outline only.
   */
  bool significant = false;

  /**
   * The largest distance received [m], drawn as an arc across the
   * sector; none draws nothing.
   */
  std::optional<unsigned> maximum;
};

/**
 * Is sector @p i of @p n measured, but shorter than
 * FlarmMinimumRange()?
 */
[[gnu::pure]]
bool
IsBelowMinimum(const FlarmRangeSector &sector,
               unsigned i, unsigned n) noexcept;

/**
 * The range of a FLARM per direction, seen from above with the
 * direction of flight up, against the minimum range FLARM asks for.
 */
class FlarmRangePlot final : public PaintWindow {
  const DialogLook &dialog_look;
  const FlarmTrafficLook &look;

  RadarRenderer radar;

  const Pen minimum_pen, limit_pen;

  std::vector<FlarmRangeSector> sectors;

  /** the device's RANGE setting [m], if known */
  std::optional<unsigned> limit;

public:
  FlarmRangePlot(const DialogLook &_dialog_look,
                 const FlarmTrafficLook &_look) noexcept;

  void SetSectors(std::vector<FlarmRangeSector> &&_sectors) noexcept {
    sectors = std::move(_sectors);
    Invalidate();
  }

  void SetLimit(std::optional<unsigned> _limit) noexcept {
    limit = _limit;
    Invalidate();
  }

private:
  void PaintSectors(Canvas &canvas, double full_range) const noexcept;
  void PaintMaxima(Canvas &canvas, double full_range) const noexcept;
  void PaintMinimum(Canvas &canvas, double full_range) const noexcept;
  void PaintLimit(Canvas &canvas, double full_range) const noexcept;
  void PaintRings(Canvas &canvas, double step, unsigned rings) const noexcept;
  void PaintAircraft(Canvas &canvas) const noexcept;

protected:
  void OnResize(PixelSize new_size) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
};
