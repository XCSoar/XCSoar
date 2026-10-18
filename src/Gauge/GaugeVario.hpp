// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/AntiFlickerWindow.hpp"
#include "ui/canvas/Color.hpp"
#include "Blackboard/FullBlackboard.hpp"

struct VarioLook;
class ContainerWindow;

class GaugeVario : public AntiFlickerWindow
{
  /** 5 m/s */
  static constexpr int GAUGEVARIORANGE = 5;

  /** degrees total sweep */
  static constexpr int GAUGEVARIOSWEEP = 90;

  static constexpr int gmax = GAUGEVARIOSWEEP + 2;
  static constexpr int gmin = -gmax;

  /** Ballast (top) and bugs (bottom) share this layout. */
  struct PercentGeometry {
    PixelRect label_rect, value_rect;
    PixelPoint label_pos, value_pos;

    PercentGeometry() = default;
    PercentGeometry(const VarioLook &look, const PixelRect &rc,
                    bool top, const char *caption) noexcept;
  };

  struct LabelValueGeometry {
    int label_right, label_top, label_bottom, label_y;
    int value_right, value_top, value_bottom, value_y;

    LabelValueGeometry() = default;
    LabelValueGeometry(const VarioLook &look, PixelPoint position) noexcept;

    static unsigned GetHeight(const VarioLook &look) noexcept;
  };

  struct Geometry {
    int nlength0, nlength1, nwidth;
    int dial_radius;

    PixelPoint offset;

    LabelValueGeometry average, gross, mc;

    PercentGeometry ballast, bugs;

    Geometry() = default;
    Geometry(const VarioLook &look, const PixelRect &rc) noexcept;
  } geometry;

  struct DrawInfo {
    double last_value;
    char last_text[32];
    Unit last_unit;

    void Reset() noexcept {
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

  const VarioLook &look;

  bool dirty = true;

  /** Finger is down on the gauge.  Lift inside opens Flight Setup. */
  bool dragging = false;

  bool background_dirty = true;
  bool vario_line_drawn = false;

  LabelValueDrawInfo average_di, mc_di, gross_di;

  /** InfoBox value-colour index last used for the MacCready figure. */
  int mc_color_index = -1;

  /** 0 normal, 1 red. Tracks the TC 30s InfoBox rule for Avg. */
  int average_color_index = -1;

  int ival_av_last = 0;
  int vval_last = 0;
  int sval_last = 0;
  int ival_last = 0;

  double last_v_diff = 0;

  int last_ballast = -1;

  int last_bugs = -1;

  /** 1 STF, 0 Var, -1 not painted. */
  int mode_stf = -1;

  /** Needle angle of the MacCready mark, or below the dial when hidden. */
  int mc_mark_pos = gmin - 1;

  /** InfoBox colour index last painted for that mark. */
  int mc_mark_color = -1;

public:
  GaugeVario(const FullBlackboard &blackboard,
             ContainerWindow &parent, const VarioLook &look,
             PixelRect rc, const WindowStyle style=WindowStyle()) noexcept;

  void ReinitialiseLook() noexcept;

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
  void OnResize(PixelSize new_size) noexcept override;
  void OnCancelMode() noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;

  /* virtual methods from class AntiFlickerWindow */
  virtual void OnPaintBuffer(Canvas &canvas) noexcept override;

private:
  void RenderBackground(Canvas &canvas) noexcept;
  void RenderValue(Canvas &canvas, const LabelValueGeometry &g,
                   LabelValueDrawInfo &di,
                   double Value, const char *Label,
                   Color value_color) noexcept;
  void RenderSpeedToFly(Canvas &canvas) noexcept;
  void RenderPercent(Canvas &canvas, const PercentGeometry &corner,
                     const char *caption, int percent, int &last) noexcept;
  void RenderBallast(Canvas &canvas) noexcept;
  void RenderBugs(Canvas &canvas) noexcept;
  int  ValueToNeedlePos(double Value) noexcept;
  void RenderNeedle(Canvas &canvas, int i, bool average, bool clear) noexcept;
  void RenderMcMark(Canvas &canvas, int i, Color color) noexcept;
  void RenderVarioLine(Canvas &canvas, int i, int sink, bool clear) noexcept;
  void RenderClimb(Canvas &canvas) noexcept;
  void RenderMode(Canvas &canvas) noexcept;
};
