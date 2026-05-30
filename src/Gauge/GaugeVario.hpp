// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/AntiFlickerWindow.hpp"
#include "ui/canvas/Color.hpp"
#include "Blackboard/FullBlackboard.hpp"
#include "Math/Angle.hpp"
#include "Renderer/VarioBarRenderer.hpp"

struct VarioLook;
struct VarioBarLook;
class ContainerWindow;

/**
 * Vario gauge based on the Larus Breeze frontend
 * (https://github.com/larus-breeze/sw_frontend_rs).
 */
class GaugeVario : public AntiFlickerWindow
{
  struct BallastGeometry {
    PixelRect label_rect, value_rect;
    PixelPoint label_pos, value_pos;

    BallastGeometry() = default;
    BallastGeometry(const VarioLook &look, const PixelRect &rc) noexcept;
  };

  struct BugsGeometry {
    PixelRect label_rect, value_rect;
    PixelPoint label_pos, value_pos;

    BugsGeometry() = default;
    BugsGeometry(const VarioLook &look, const PixelRect &rc) noexcept;
  };

  struct RowGeometry {
    PixelRect label_background, value_background;
  };

  struct Geometry {
    PixelRect content_rect;

    PixelPoint dial_center;
    unsigned outer_radius;
    unsigned inner_radius;
    unsigned inner_square_side;
    unsigned inner_row_slot;
    unsigned row_gap;

    PixelRect speed_arrows_rect;

    RowGeometry hero_row, gross_row, mc_row;

    PixelRect mc_mode_rect;
    int mc_mode_center_x = 0;

    PixelRect unit_rect;

    BallastGeometry ballast;
    BugsGeometry bugs;

    Geometry() = default;
    Geometry(const VarioLook &look, const PixelRect &rc) noexcept;
  } geometry;

  struct DrawInfo {
    unsigned last_width;
    double last_value;
    char last_text[32];
    Unit last_unit;

    void Reset() noexcept {
      last_width = 0;
      last_value = -9999;
      last_text[0] = '\0';
      last_unit = Unit::UNDEFINED;
    }
  };

  struct LabelValueDrawInfo {
    DrawInfo label;
    DrawInfo value;

    void Reset() noexcept {
      label.Reset();
      value.Reset();
    }
  };

  const FullBlackboard &blackboard;

  VarioLook &look;

  VarioBarRenderer speed_bar_renderer;

  bool dirty = true;

  bool background_dirty = true;

  LabelValueDrawInfo hero_di, gross_di, mc_di;

  DrawInfo mc_mode_di;

  int last_ballast = -1;

  int last_bugs = -1;

  unsigned cached_look_generation = 0;

  unsigned cached_scale_title_font = 0;

  bool debug_overlay = false;

public:
  GaugeVario(const FullBlackboard &blackboard,
             ContainerWindow &parent, VarioLook &look,
             const VarioBarLook &vario_bar_look,
             PixelRect rc, const WindowStyle style=WindowStyle()) noexcept;

  /** Layout debug overlay for #RunGaugeVarioRenderer only. */
  void SetDebugOverlay(bool enable) noexcept {
    debug_overlay = enable;
    Invalidate();
  }

protected:
  const MoreData &Basic() const noexcept {
    return blackboard.Basic();
  }

  const DerivedInfo &Calculated() const noexcept {
    return blackboard.Calculated();
  }

  const ComputerSettings &GetComputerSettings() const noexcept {
    return blackboard.GetComputerSettings();
  }

  const GlidePolar &GetGlidePolar() const noexcept {
    return GetComputerSettings().polar.glide_polar_task;
  }

  const VarioSettings &Settings() const noexcept {
    return blackboard.GetUISettings().vario;
  }

protected:
  /* virtual methods from class Window */
  virtual void OnResize(PixelSize new_size) noexcept override;

  /* virtual methods from class AntiFlickerWindow */
  virtual void OnPaintBuffer(Canvas &canvas) noexcept override;

private:
  void RecalculateGeometry() noexcept;
  void LayoutValueColumn() noexcept;
  void LayoutScaleUnit() noexcept;
  void LayoutMcModeHint() noexcept;

  void RenderBackground(Canvas &canvas, const PixelRect &rc) noexcept;
  void RenderScale(Canvas &canvas) noexcept;
  void RenderSpeedArrows(Canvas &canvas) noexcept;
  void RenderNeedles(Canvas &canvas) noexcept;
  void RenderInnerText(Canvas &canvas) noexcept;
  void RenderMcModeHint(Canvas &canvas) noexcept;
  void RenderUnit(Canvas &canvas) noexcept;
  void RenderRow(Canvas &canvas, const RowGeometry &row,
                 DrawInfo &label_di, DrawInfo &value_di,
                 const char *label, const char *value_text,
                 Color value_color) noexcept;
  void RenderBallast(Canvas &canvas) noexcept;
  void RenderBugs(Canvas &canvas) noexcept;
  void RenderClimb(Canvas &canvas) noexcept;
  void DrawDebugOverlay(Canvas &canvas) const noexcept;
};
