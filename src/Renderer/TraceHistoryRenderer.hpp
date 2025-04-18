// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
struct TraceHistoryLook;
struct VarioLook;
struct ChartLook;
class ChartRenderer;
class Canvas;
class TraceVariableHistory;

/**
 * (Vario) Trace History Renderer
 * renders the variometer history graph for the vario trace Infobox
 */
class TraceHistoryRenderer {
  const TraceHistoryLook &look;
  const VarioLook &vario_look;
  const ChartLook &chart_look;

public:
  TraceHistoryRenderer(const TraceHistoryLook &_look,
                       const VarioLook &_vario_look,
                       const ChartLook &_chart_look)
    :look(_look), vario_look(_vario_look), chart_look(_chart_look) {}

  void RenderVario(Canvas& canvas,
                   const PixelRect rc,
                   const TraceVariableHistory& var,
                   const bool centered,
                   const double mc,
                   const double max,
                   const double min) const;

private:
  void ScaleChart(ChartRenderer &chart,
                   const TraceVariableHistory& var,
                  const double max,
                  const double min,
                   const bool centered) const;

  void RenderAxis(ChartRenderer &chart,
                   const TraceVariableHistory& var) const;

  void render_filled_posneg(ChartRenderer &chart,
                            const TraceVariableHistory& var) const;
};
