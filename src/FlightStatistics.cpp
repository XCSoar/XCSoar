/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "FlightStatistics.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Math/Constants.h"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Units.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Wind/WindStore.hpp"
#include "Language.hpp"
#include "Atmosphere.h"
#include "SettingsComputer.hpp"
#include "SettingsMap.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "ChartProjection.hpp"
#include "RenderTask.hpp"
#include "RenderTaskPoint.hpp"
#include "RenderObservationZone.hpp"
#include "Screen/Chart.hpp"
#include "Task/Visitors/TaskVisitor.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/Airspaces.hpp"

#include <algorithm>

#include <stdio.h>

using std::min;
using std::max;

void FlightStatistics::Reset() {
  ScopeLock lock(mutexStats);

  ThermalAverage.Reset();
  Altitude.Reset();
  Altitude_Base.Reset();
  Altitude_Ceiling.Reset();
  Task_Speed.Reset();
  Altitude_Terrain.Reset();
}

/**
 * Utility class to draw task leg entry lines
 */
class ChartLegHelper:
  public TaskPointConstVisitor
{
public:
  ChartLegHelper(Chart& chart, const fixed start_time):
    m_chart(chart),
    m_start_time(start_time)
    {
    };

  void Visit(const UnorderedTaskPoint& tp) {}
  void Visit(const StartPoint& tp) {
    if (tp.has_exited())
      draw(tp);
  }
  void Visit(const FinishPoint& tp) {
    if (tp.has_entered())
      draw(tp);
  }
  void Visit(const AATPoint& tp) {
    if (tp.has_entered())
      draw(tp);
  }
  void Visit(const ASTPoint& tp) {
    if (tp.has_entered())
      draw(tp);
  }

private:
  void draw(const OrderedTaskPoint& tp) {
    fixed x = (tp.get_state_entered().Time - m_start_time) / 3600;
    if (x >= fixed_zero)
      m_chart.DrawLine(x, m_chart.getYmin(), x, m_chart.getYmax(),
                       Chart::STYLE_REDTHICK);
  }
  Chart& m_chart;
  const fixed m_start_time;
};

static void DrawLegs(Chart& chart,
                     const TaskManager &task_manager,
                     const NMEA_INFO& basic,
                     const DERIVED_INFO& calculated,
                     const bool task_relative)
{
  if (!calculated.common_stats.task_started)
    return;

  const fixed start_time = task_relative
    ? basic.Time - calculated.common_stats.task_time_elapsed
    : basic.flight.TakeOffTime;

  const OrderedTask &task = task_manager.get_ordered_task();

  ChartLegHelper leg_visitor(chart, start_time);
  task.tp_CAccept(leg_visitor);
}

void
FlightStatistics::RenderBarograph(Canvas &canvas, const RECT rc,
                                  const NMEA_INFO &nmea_info,
                                  const DERIVED_INFO &derived_info,
                                  const ProtectedTaskManager *_task) const
{
  Chart chart(canvas, rc);

  if (Altitude.sum_n < 2) {
    chart.DrawNoData();
    return;
  }

  chart.ScaleXFromData(Altitude);
  chart.ScaleYFromData(Altitude);
  chart.ScaleYFromValue(fixed_zero);
  chart.ScaleXFromValue(Altitude.x_min + fixed_one); // in case no data
  chart.ScaleXFromValue(Altitude.x_min);

  if (_task != NULL) {
    ProtectedTaskManager::Lease task(*_task);
    DrawLegs(chart, task, nmea_info, derived_info, false);
  }

  Brush hbHorizonGround(Chart::GROUND_COLOUR);

  canvas.null_pen();
  canvas.select(hbHorizonGround);

  chart.DrawFilledLineGraph(Altitude_Terrain);
  canvas.white_pen();
  canvas.white_brush();

  chart.DrawXGrid(fixed_half, Altitude.x_min, Chart::STYLE_THINDASHPAPER,
                  fixed_half, true);
  chart.DrawYGrid(Units::ToSysAltitude(fixed(1000)),
                  fixed_zero, Chart::STYLE_THINDASHPAPER, fixed(1000), true);
  chart.DrawLineGraph(Altitude, Chart::STYLE_MEDIUMBLACK);

  chart.DrawTrend(Altitude_Base, Chart::STYLE_BLUETHIN);
  chart.DrawTrend(Altitude_Ceiling, Chart::STYLE_BLUETHIN);

  chart.DrawXLabel(_T("t (hr)"));
  chart.DrawYLabel(_T("h"));
}

void
FlightStatistics::RenderSpeed(Canvas &canvas, const RECT rc,
                              const NMEA_INFO &nmea_info,
                              const DERIVED_INFO &derived_info,
                              const TaskManager &task) const
{
  Chart chart(canvas, rc);

  if ((Task_Speed.sum_n < 2) || !task.check_ordered_task()) {
    chart.DrawNoData();
    return;
  }

  chart.ScaleXFromData(Task_Speed);
  chart.ScaleYFromData(Task_Speed);
  chart.ScaleYFromValue(fixed_zero);
  chart.ScaleXFromValue(Task_Speed.x_min + fixed_one); // in case no data
  chart.ScaleXFromValue(Task_Speed.x_min);

  DrawLegs(chart, task, nmea_info, derived_info, true);

  chart.DrawXGrid(fixed_half, Task_Speed.x_min,
                  Chart::STYLE_THINDASHPAPER, fixed_half, true);
  chart.DrawYGrid(Units::ToSysUnit(fixed_ten, Units::TaskSpeedUnit),
                  fixed_zero, Chart::STYLE_THINDASHPAPER, fixed(10), true);
  chart.DrawLineGraph(Task_Speed, Chart::STYLE_MEDIUMBLACK);
  chart.DrawTrend(Task_Speed, Chart::STYLE_BLUETHIN);

  chart.DrawXLabel(_T("t (hr)"));
  chart.DrawYLabel(_T("V"));
}

void
FlightStatistics::RenderClimb(Canvas &canvas, const RECT rc,
                              const GlidePolar& glide_polar) const
{
  Chart chart(canvas, rc);

  if (ThermalAverage.sum_n < 1) {
    chart.DrawNoData();
    return;
  }

  fixed MACCREADY = glide_polar.get_mc();

  chart.ScaleYFromData(ThermalAverage);
  chart.ScaleYFromValue(MACCREADY + fixed_half);
  chart.ScaleYFromValue(fixed_zero);

  chart.ScaleXFromValue(fixed_minus_one);
  chart.ScaleXFromValue(fixed(ThermalAverage.sum_n));

  chart.DrawYGrid(Units::ToSysVSpeed(fixed_one), fixed_zero,
                  Chart::STYLE_THINDASHPAPER, fixed_one, true);
  chart.DrawBarChart(ThermalAverage);

  chart.DrawLine(fixed_zero, MACCREADY, fixed(ThermalAverage.sum_n), MACCREADY,
                 Chart::STYLE_REDTHICK);

  chart.DrawLabel(_T("MC"),
                  max(fixed_half, fixed(ThermalAverage.sum_n) - fixed_one),
                  MACCREADY);

  chart.DrawTrendN(ThermalAverage, Chart::STYLE_BLUETHIN);

  chart.DrawXLabel(_T("n"));
  chart.DrawYLabel(_T("w"));
}

void
FlightStatistics::RenderGlidePolar(Canvas &canvas, const RECT rc,
                                   const DERIVED_INFO &derived, 
                                   const SETTINGS_COMPUTER &settings_computer,
                                   const GlidePolar& glide_polar) const
{
  int i;
  Chart chart(canvas, rc);
  Pen blue_pen(2, Color::BLUE);

  chart.ScaleYFromValue(fixed_zero);
  chart.ScaleYFromValue(-glide_polar.get_Smax() * fixed(1.1));
  chart.ScaleXFromValue(glide_polar.get_Vmin() * fixed(0.8));
  chart.ScaleXFromValue(glide_polar.get_Vmax() + fixed_two);

  chart.DrawXGrid(Units::ToSysSpeed(fixed_ten), fixed_zero,
                  Chart::STYLE_THINDASHPAPER, fixed_ten, true);
  chart.DrawYGrid(Units::ToSysVSpeed(fixed_one),
                  fixed_zero, Chart::STYLE_THINDASHPAPER, fixed_one, true);

  fixed sinkrate0, sinkrate1;
  fixed v0 = fixed_zero, v1;
  bool v0valid = false;
  int i0 = 0;

  for (i = glide_polar.get_Vmin(); i <= (int)glide_polar.get_Vmax(); ++i) {
    sinkrate0 = -glide_polar.SinkRate(fixed(i));
    sinkrate1 = -glide_polar.SinkRate(fixed(i + 1));
    chart.DrawLine(fixed(i), sinkrate0, fixed(i + 1), sinkrate1,
                   Chart::STYLE_MEDIUMBLACK);

    if (derived.AverageClimbRateN[i] > 0) {
      v1 = derived.AverageClimbRate[i] / derived.AverageClimbRateN[i];

      if (v0valid)
        chart.DrawLine(fixed(i0), v0, fixed(i), v1, blue_pen);

      v0 = v1;
      i0 = i;
      v0valid = true;
    }
  }

  fixed MACCREADY = glide_polar.get_mc();
  fixed sb = -glide_polar.get_SbestLD();
  fixed ff = (sb - MACCREADY) / glide_polar.get_VbestLD();

  chart.DrawLine(fixed_zero, MACCREADY, glide_polar.get_Vmax(),
                 MACCREADY + ff * glide_polar.get_Vmax(), Chart::STYLE_REDTHICK);

  chart.DrawXLabel(_T("V"));
  chart.DrawYLabel(_T("w"));

  TCHAR text[80];
  canvas.background_transparent();

  _stprintf(text, _T("%s: %d kg"), _("Mass"),
            (int)glide_polar.get_all_up_weight());
  canvas.text(rc.left + IBLSCALE(30), rc.bottom - IBLSCALE(55), text);

  fixed wl = glide_polar.get_wing_loading();
  if ( wl != fixed_zero )
  {
    _stprintf(text, _T("%s: %.1f kg/m2"), _("Wing loading"), (double)wl);

    canvas.text(rc.left + IBLSCALE(30), rc.bottom - IBLSCALE(40), text);
  }
}

static void
DrawTrace(Chart &chart, const ChartProjection& proj,
          const TracePointVector& trace, Chart::Style style)
{
  RasterPoint last;
  for (TracePointVector::const_iterator it = trace.begin();
       it != trace.end(); ++it) {
    RasterPoint sc = proj.GeoToScreen(it->get_location());
    if (it != trace.begin())
      chart.StyleLine(sc, last, style);

    last = sc;
  }
}

void
FlightStatistics::RenderOLC(Canvas &canvas, const RECT rc,
                            const NMEA_INFO &nmea_info, 
                            const SETTINGS_COMPUTER &settings_computer,
                            const SETTINGS_MAP &settings_map, 
                            const TracePointVector& olc,
                            const TracePointVector& trace) const
{
  // note: braces used here just to delineate separate main steps of
  // this function.  It's useful to ensure things are done in the right
  // order rather than having a monolithic block of code.

  Chart chart(canvas, rc);

  if (trace.size() < 2) {
    chart.DrawNoData();
    return;
  }

  ChartProjection proj(rc, olc, nmea_info.Location);

  RasterPoint aircraft_pos = proj.GeoToScreen(nmea_info.Location);
  Graphics::DrawAircraft(canvas, nmea_info.Heading, aircraft_pos);

  DrawTrace(chart, proj, trace, Chart::STYLE_MEDIUMBLACK);
  DrawTrace(chart, proj, olc, Chart::STYLE_REDTHICK);
}


void
FlightStatistics::RenderTask(Canvas &canvas, const RECT rc,
                             const NMEA_INFO &nmea_info, 
                             const SETTINGS_COMPUTER &settings_computer,
                             const SETTINGS_MAP &settings_map, 
                             const TaskManager &task_manager) const
{
  Chart chart(canvas, rc);

  const OrderedTask &task = task_manager.get_ordered_task();

  if (!task.check_task()) {
    chart.DrawNoData();
    return;
  }

  ChartProjection proj(rc, task, nmea_info.Location);

  RenderObservationZone ozv;
  RenderTaskPoint tpv(canvas, NULL, proj, settings_map,
                      task.get_task_projection(),
                      ozv, false, nmea_info.Location);
  ::RenderTask dv(tpv, proj.GetScreenBounds());
  dv.Visit(task);

  TracePointVector trace = task_manager.get_trace_points();
  DrawTrace(chart, proj, trace, Chart::STYLE_MEDIUMBLACK);

  RasterPoint aircraft_pos = proj.GeoToScreen(nmea_info.Location);
  Graphics::DrawAircraft(canvas, nmea_info.Heading, aircraft_pos);
}


void
FlightStatistics::RenderTemperature(Canvas &canvas, const RECT rc) const
{
  Chart chart(canvas, rc);

  int i;
  int hmin = 10000;
  int hmax = -10000;
  fixed tmin = fixed(CuSonde::maxGroundTemperature);
  fixed tmax = fixed(CuSonde::maxGroundTemperature);

  // find range for scaling of graph
  for (i = 0; i < CUSONDE_NUMLEVELS - 1; i++) {
    if (CuSonde::cslevels[i].nmeasurements) {
      hmin = min(hmin, i);
      hmax = max(hmax, i);

      tmin = min(tmin, fixed(min(CuSonde::cslevels[i].tempDry,
                                 min(CuSonde::cslevels[i].airTemp,
                                     CuSonde::cslevels[i].dewpoint))));
      tmax = max(tmax, fixed(max(CuSonde::cslevels[i].tempDry,
                                 max(CuSonde::cslevels[i].airTemp,
                                     CuSonde::cslevels[i].dewpoint))));
    }
  }

  if (hmin >= hmax) {
    chart.DrawNoData();
    return;
  }

  chart.ScaleYFromValue(fixed(hmin));
  chart.ScaleYFromValue(fixed(hmax));
  chart.ScaleXFromValue(tmin);
  chart.ScaleXFromValue(tmax);

  bool labelDry = false;
  bool labelAir = false;
  bool labelDew = false;

  int ipos = 0;

  for (i = 0; i < CUSONDE_NUMLEVELS - 1; i++) {
    if (CuSonde::cslevels[i].nmeasurements
        && CuSonde::cslevels[i + 1].nmeasurements) {

      ipos++;

      chart.DrawLine(fixed(CuSonde::cslevels[i].tempDry), fixed(i),
                     fixed(CuSonde::cslevels[i + 1].tempDry), fixed(i + 1),
                     Chart::STYLE_REDTHICK);

      chart.DrawLine(fixed(CuSonde::cslevels[i].airTemp), fixed(i),
                     fixed(CuSonde::cslevels[i + 1].airTemp), fixed(i + 1),
                     Chart::STYLE_MEDIUMBLACK);

      chart.DrawLine(fixed(CuSonde::cslevels[i].dewpoint), fixed(i),
                     fixed(CuSonde::cslevels[i + 1].dewpoint), fixed(i + 1),
                     Chart::STYLE_BLUETHIN);

      if (ipos > 2) {
        if (!labelDry) {
          chart.DrawLabel(_T("DALR"),
                          fixed(CuSonde::cslevels[i + 1].tempDry), fixed(i));
          labelDry = true;
        } else if (!labelAir) {
          chart.DrawLabel(_T("Air"),
                          fixed(CuSonde::cslevels[i + 1].airTemp), fixed(i));
          labelAir = true;
        } else if (!labelDew) {
          chart.DrawLabel(_T("Dew"),
                          fixed(CuSonde::cslevels[i + 1].dewpoint), fixed(i));
          labelDew = true;
        }
      }
    }
  }

  chart.DrawXLabel(_T("T")_T(DEG));
  chart.DrawYLabel(_T("h"));
}

void
FlightStatistics::RenderWind(Canvas &canvas, const RECT rc,
                             const NMEA_INFO &nmea_info,
                             const WindStore &wind_store) const
{
  int numsteps = 10;
  int i;
  fixed h;
  Vector wind;
  bool found = true;
  fixed mag;

  LeastSquares windstats_mag;
  Chart chart(canvas, rc);

  if (Altitude_Ceiling.y_max - Altitude_Ceiling.y_min <= fixed_ten) {
    chart.DrawNoData();
    return;
  }

  for (i = 0; i < numsteps; i++) {
    h = fixed(Altitude_Ceiling.y_max - Altitude_Base.y_min) * i /
        (numsteps - 1) + fixed(Altitude_Base.y_min);

    wind = wind_store.GetWind(nmea_info.Time, h, &found);
    mag = hypot(wind.x, wind.y);

    windstats_mag.LeastSquaresUpdate(mag, h);
  }

  chart.ScaleXFromData(windstats_mag);
  chart.ScaleXFromValue(fixed_zero);
  chart.ScaleXFromValue(fixed_ten);

  chart.ScaleYFromData(windstats_mag);

  chart.DrawXGrid(Units::ToSysSpeed(fixed(5)), fixed_zero,
                  Chart::STYLE_THINDASHPAPER, fixed(5), true);
  chart.DrawYGrid(Units::ToSysAltitude(fixed(1000)),
                  fixed_zero,
                  Chart::STYLE_THINDASHPAPER, fixed(1000), true);
  chart.DrawLineGraph(windstats_mag, Chart::STYLE_MEDIUMBLACK);

#define WINDVECTORMAG 25

  numsteps = (int)((rc.bottom - rc.top) / WINDVECTORMAG) - 1;

  // draw direction vectors
  fixed hfact;
  for (i = 0; i < numsteps; i++) {
    hfact = fixed(i + 1) / (numsteps + 1);
    h = fixed(Altitude_Ceiling.y_max - Altitude_Base.y_min) * hfact +
        fixed(Altitude_Base.y_min);

    wind = wind_store.GetWind(nmea_info.Time, h, &found);
    if (windstats_mag.x_max == fixed_zero)
      windstats_mag.x_max = fixed_one; // prevent /0 problems
    wind.x /= fixed(windstats_mag.x_max);
    wind.y /= fixed(windstats_mag.x_max);
    mag = hypot(wind.x, wind.y);
    if (negative(mag))
      continue;

    Angle angle = Angle::radians(atan2(-wind.x, wind.y));

    chart.DrawArrow((chart.getXmin() + chart.getXmax()) / 2, h,
                    mag * WINDVECTORMAG, angle, Chart::STYLE_MEDIUMBLACK);
  }

  chart.DrawXLabel(_T("w"));
  chart.DrawYLabel(_T("h"));
}

class AirspaceIntersectionVisitorSlice: public AirspaceIntersectionVisitor
{
public:
  AirspaceIntersectionVisitorSlice(Canvas &canvas, Chart &chart,
                                   const SETTINGS_MAP &settings,
                                   const GeoPoint start,
                                   const AIRCRAFT_STATE& state) :
    m_canvas(canvas), m_chart(chart), m_settings(settings), m_start(start), m_state(state)
  {
  }

  void
  Render(const AbstractAirspace& as, int type)
  {
    if (m_intersections.empty())
      return;

    if (m_settings.bAirspaceBlackOutline)
      m_canvas.black_pen();
    else
      m_canvas.select(Graphics::hAirspacePens[type]);

#ifdef ENABLE_SDL
    Color color = Graphics::Colours[m_settings.iAirspaceColour[as.get_type()]];
#ifdef ENABLE_OPENGL
    color = color.with_alpha(48);
#endif
    m_canvas.select(Brush(color));
#else
    m_canvas.select(Graphics::GetAirspaceBrushByClass(type, m_settings));
    m_canvas.set_text_color(Graphics::GetAirspaceColourByClass(type, m_settings));
#endif

    RECT rcd;
    rcd.top = m_chart.screenY(as.get_top_altitude(m_state));
    if (as.is_base_terrain())
      rcd.bottom = m_chart.screenY(fixed_zero);
    else
      rcd.bottom = m_chart.screenY(as.get_base_altitude(m_state));
    
    for (AirspaceIntersectionVector::const_iterator it = m_intersections.begin();
         it != m_intersections.end(); ++it) {
      const GeoPoint p_start = it->first;
      const GeoPoint p_end = it->second;
      const fixed distance_start = m_start.distance(p_start);
      const fixed distance_end = m_start.distance(p_end);

      rcd.left = m_chart.screenX(distance_start);
      rcd.right = m_chart.screenX(distance_end);
      
      // only one edge found, next edge must be beyond screen
      if ((rcd.left == rcd.right) && (p_start == p_end)) {
        rcd.right = m_chart.screenX(m_chart.getXmax());
      }

      m_canvas.rectangle(rcd.left, rcd.top, rcd.right, rcd.bottom);
    }
  }

  void
  render(const AbstractAirspace& as)
  {
    int type = as.get_type();
    if (type >= 0)
      Render(as, type);
  }

  void
  Visit(const AirspaceCircle& as)
  {
    render(as);
  }

  void
  Visit(const AirspacePolygon& as)
  {
    render(as);
  }

private:
  Canvas& m_canvas;
  Chart& m_chart;
  const SETTINGS_MAP& m_settings;
  const GeoPoint m_start;
  const AIRCRAFT_STATE& m_state;
};

void
FlightStatistics::RenderAirspace(Canvas &canvas, const RECT rc,
                                 const NMEA_INFO &nmea_info, 
                                 const DERIVED_INFO &derived,
                                 const SETTINGS_MAP &settings_map, 
                                 const Airspaces &airspace_database,
                                 RasterTerrain *terrain) const
{
  static const fixed range(50000);
  fixed hmin = max(fixed_zero, nmea_info.GPSAltitude - fixed(3300));
  fixed hmax = max(fixed(3300), nmea_info.GPSAltitude + fixed(1000));
  const GeoPoint p_start = nmea_info.Location;
  const GeoVector vec(range, nmea_info.TrackBearing);
  const GeoPoint p_end = vec.end_point(p_start);

  Chart chart(canvas, rc);
  chart.ResetScale();
  chart.ScaleXFromValue(fixed_zero);
  chart.ScaleXFromValue(range);
  chart.ScaleYFromValue(hmin);
  chart.ScaleYFromValue(hmax);

  // draw airspaces
  AirspaceIntersectionVisitorSlice ivisitor(canvas, chart, settings_map, p_start, ToAircraftState(nmea_info));
  airspace_database.visit_intersecting(p_start, vec, ivisitor);

  // draw terrain
  if (terrain != NULL) {
    RasterTerrain::Lease map(*terrain);

    RasterPoint points[2 + AIRSPACE_SCANSIZE_X];

    RasterPoint pf0, pf1;
    pf0.x = chart.screenX(range);
    pf0.y = chart.screenY(fixed_zero);
    points[0] = pf0;
    pf1.x = chart.screenX(fixed_zero);
    pf1.y = chart.screenY(fixed_zero);
    points[1] = pf1;

    unsigned i = 2;
    for (unsigned j = 0; j < AIRSPACE_SCANSIZE_X; ++j) {
      const fixed t_this = fixed(j) / (AIRSPACE_SCANSIZE_X - 1);
      const GeoPoint p_this = p_start + (p_end - p_start) * t_this;

      short h = map->GetField(p_this);
      if (RasterBuffer::is_special(h)) {
        if (RasterBuffer::is_water(h))
          /* water is at 0m MSL */
          /* XXX paint in blue? */
          h = 0;
        else
          /* skip "unknown" values */
          continue;
      }

      RasterPoint p;
      p.x = chart.screenX(t_this * range);
      p.y = chart.screenY(fixed(h));

      points[i++] = p;
    }

    if (i >= 4) {
      Brush a_brush(Chart::GROUND_COLOUR);

      canvas.null_pen();
      canvas.select(a_brush);
      canvas.polygon(&points[0], i);
    }
  }

  // draw aircraft trend line
  if (nmea_info.GroundSpeed > fixed(10)) {
    fixed t = range / nmea_info.GroundSpeed;
    chart.DrawLine(fixed_zero, nmea_info.GPSAltitude, range,
                   nmea_info.GPSAltitude + derived.Average30s * t,
                   Chart::STYLE_BLUETHIN);
  }

  // draw aircraft
  {
    int delta;
    canvas.white_pen();
    canvas.white_brush();

    RasterPoint line[4];
    line[0].x = chart.screenX(fixed_zero);
    line[0].y = chart.screenY(nmea_info.GPSAltitude);
    line[1].x = rc.left;
    line[1].y = line[0].y;
    delta = (line[0].x - line[1].x);
    line[2].x = line[1].x;
    line[2].y = line[0].y - delta / 2;
    line[3].x = (line[1].x + line[0].x) / 2;
    line[3].y = line[0].y;
    canvas.polygon(line, 4);
  }

  // draw grid
  {
    canvas.white_pen();
    canvas.white_brush();
    canvas.set_text_color(Color(0xff, 0xff, 0xff));

    chart.DrawXGrid(Units::ToSysDistance(fixed(5)), fixed_zero,
                    Chart::STYLE_THINDASHPAPER, fixed(5), true);
    chart.DrawYGrid(Units::ToSysDistance(fixed(1000)), fixed_zero,
                    Chart::STYLE_THINDASHPAPER, fixed(1000), true);

    chart.DrawXLabel(_T("D"));
    chart.DrawYLabel(_T("h"));
  }
}

void
FlightStatistics::StartTask()
{
  ScopeLock lock(mutexStats);
  // JMW clear thermal climb average on task start
  ThermalAverage.Reset();
  Task_Speed.Reset();
}

void
FlightStatistics::AddAltitudeTerrain(const fixed tflight, const fixed terrainalt)
{
  ScopeLock lock(mutexStats);
  Altitude_Terrain.LeastSquaresUpdate(max(fixed_zero, tflight / 3600),
                                      terrainalt);
}

void
FlightStatistics::AddAltitude(const fixed tflight, const fixed alt)
{
  ScopeLock lock(mutexStats);
  Altitude.LeastSquaresUpdate(max(fixed_zero, tflight / 3600), alt);
}

fixed
FlightStatistics::AverageThermalAdjusted(const fixed mc_current,
                                         const bool circling)
{
  ScopeLock lock(mutexStats);

  fixed mc_stats;
  if (ThermalAverage.y_ave > fixed_zero) {
    if (mc_current > fixed_zero && circling)
      mc_stats = (ThermalAverage.sum_n * ThermalAverage.y_ave + mc_current) /
                 (ThermalAverage.sum_n + 1);
    else
      mc_stats = ThermalAverage.y_ave;
  } else {
    mc_stats = mc_current;
  }

  return mc_stats;
}

void
FlightStatistics::AddTaskSpeed(const fixed tflight, const fixed val)
{
  ScopeLock lock(mutexStats);
  Task_Speed.LeastSquaresUpdate(tflight / 3600, std::max(fixed_zero,val));
}

void
FlightStatistics::AddClimbBase(const fixed tflight, const fixed alt)
{
  ScopeLock lock(mutexStats);

  if (Altitude_Ceiling.sum_n > 0)
    // only update base if have already climbed, otherwise
    // we will catch the takeoff height as the base.
    Altitude_Base.LeastSquaresUpdate(max(fixed_zero, tflight) / 3600, alt);
}

void
FlightStatistics::AddClimbCeiling(const fixed tflight, const fixed alt)
{
  ScopeLock lock(mutexStats);
  Altitude_Ceiling.LeastSquaresUpdate(max(fixed_zero, tflight) / 3600, alt);
}

/**
 * Adds a thermal to the ThermalAverage calculator
 * @param v Average climb speed of the last thermal
 */
void
FlightStatistics::AddThermalAverage(const fixed v)
{
  ScopeLock lock(mutexStats);
  ThermalAverage.LeastSquaresUpdate(v);
}

void
FlightStatistics::CaptionBarograph(TCHAR *sTmp)
{
  ScopeLock lock(mutexStats);
  if (Altitude_Ceiling.sum_n < 2) {
    sTmp[0] = _T('\0');
  } else if (Altitude_Ceiling.sum_n < 4) {
    _stprintf(sTmp, _T("%s:\r\n  %.0f-%.0f %s"),
              _("Working band"),
              (double)Units::ToUserAltitude(fixed(Altitude_Base.y_ave)),
              (double)Units::ToUserAltitude(fixed(Altitude_Ceiling.y_ave)),
              Units::GetAltitudeName());
  } else {
    _stprintf(sTmp, _T("%s:\r\n  %.0f-%.0f %s\r\n\r\n%s:\r\n  %.0f %s/hr"),
              _("Working band"),
              (double)Units::ToUserAltitude(fixed(Altitude_Base.y_ave)),
              (double)Units::ToUserAltitude(fixed(Altitude_Ceiling.y_ave)),
              Units::GetAltitudeName(),
              _("Ceiling trend"),
              (double)Units::ToUserAltitude(fixed(Altitude_Ceiling.m)),
              Units::GetAltitudeName());
  }
}

void
FlightStatistics::CaptionClimb(TCHAR* sTmp)
{
  ScopeLock lock(mutexStats);
  if (ThermalAverage.sum_n == 0) {
    sTmp[0] = _T('\0');
  } else if (ThermalAverage.sum_n == 1) {
    _stprintf(sTmp, _T("%s:\r\n  %3.1f %s"),
              _("Av climb"),
              (double)Units::ToUserVSpeed(fixed(ThermalAverage.y_ave)),
              Units::GetVerticalSpeedName());
  } else {
    _stprintf(sTmp, _T("%s:\r\n  %3.1f %s\r\n\r\n%s:\r\n  %3.2f %s"),
              _("Av climb"),
              (double)Units::ToUserVSpeed(fixed(ThermalAverage.y_ave)),
              Units::GetVerticalSpeedName(),
              _("Climb trend"),
              (double)Units::ToUserVSpeed(fixed(ThermalAverage.m)),
              Units::GetVerticalSpeedName());
  }
}

void
FlightStatistics::CaptionPolar(TCHAR *sTmp, const GlidePolar& glide_polar) const
{
  _stprintf(sTmp, Layout::landscape ?
                  _T("%s:\r\n  %d\r\n  at %d %s\r\n\r\n%s:\r\n  %3.2f %s\r\n  at %d %s") :
                  _T("%s:\r\n  %d at %d %s\r\n%s:\r\n  %3.2f %s at %d %s"),
            _("Best LD"),
            (int)glide_polar.get_bestLD(),
            (int)Units::ToUserSpeed(glide_polar.get_VbestLD()),
            Units::GetSpeedName(),
            _("Min sink"),
            (double)Units::ToUserVSpeed(glide_polar.get_Smin()),
            Units::GetVerticalSpeedName(),
            (int)Units::ToUserSpeed(glide_polar.get_Vmin()),
            Units::GetSpeedName());
}

void
FlightStatistics::CaptionTempTrace(TCHAR *sTmp) const
{
  _stprintf(sTmp, _T("%s:\r\n  %5.0f %s\r\n\r\n%s:\r\n  %5.0f %s\r\n"),
            _("Thermal height"),
            (double)Units::ToUserAltitude(fixed(CuSonde::thermalHeight)),
            Units::GetAltitudeName(),
            _("Cloud base"),
            (double)Units::ToUserAltitude(fixed(CuSonde::cloudBase)),
            Units::GetAltitudeName());
}

void
FlightStatistics::CaptionTask(TCHAR *sTmp, const DERIVED_INFO &derived) const
{
  const CommonStats &common = derived.common_stats;
  fixed d_remaining = derived.task_stats.total.remaining.get_distance();

  if (!common.ordered_valid) {
    _tcscpy(sTmp, _("No task"));
  } else {
    TCHAR timetext1[100];
    TCHAR timetext2[100];
    if (common.ordered_has_targets) {
      Units::TimeToText(timetext1, (int)common.task_time_remaining);
      Units::TimeToText(timetext2, (int)common.aat_time_remaining);

      if (Layout::landscape) {
        _stprintf(sTmp,
            _T("%s:\r\n  %s\r\n%s:\r\n  %s\r\n%s:\r\n  %5.0f %s\r\n%s:\r\n  %5.0f %s\r\n"),
            _("Task to go"), timetext1, _("AAT to go"), timetext2,
            _("Distance to go"),
            (double)Units::ToUserDistance(d_remaining),
            Units::GetDistanceName(), _("Target speed"),
            (double)Units::ToUserUnit(common.aat_speed_remaining,
                                      Units::TaskSpeedUnit),
            Units::GetTaskSpeedName());
      } else {
        _stprintf(sTmp,
            _T("%s: %s\r\n%s: %s\r\n%s: %5.0f %s\r\n%s: %5.0f %s\r\n"),
            _("Task to go"), timetext1, _("AAT to go"), timetext2,
            _("Distance to go"),
            (double)Units::ToUserDistance(d_remaining),
            Units::GetDistanceName(),
            _("Target speed"),
            (double)Units::ToUserUnit(common.aat_speed_remaining,
                                      Units::TaskSpeedUnit),
            Units::GetTaskSpeedName());
      }
    } else {
      Units::TimeToText(timetext1, (int)common.task_time_remaining);
      _stprintf(sTmp, _T("%s: %s\r\n%s: %5.0f %s\r\n"),
                _("Task to go"), timetext1, _("Distance to go"),
                (double)Units::ToUserDistance(d_remaining),
                Units::GetDistanceName());
    }
  }
}
