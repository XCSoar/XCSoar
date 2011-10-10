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

#include "FlarmTrafficWindow.hpp"
#include "FLARM/Traffic.hpp"
#include "FLARM/Friends.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Math/Screen.hpp"
#include "Language/Language.hpp"
#include "Screen/Graphics.hpp"
#include "Util/Macros.hpp"

#include <assert.h>
#include <stdio.h>

FlarmTrafficWindow::FlarmTrafficWindow(const FlarmTrafficLook &_look,
                                       unsigned _padding, bool _small)
  :look(_look),
   distance(2000),
   selection(-1), warning(-1),
   padding(_padding),
   small(_small),
   enable_north_up(false),
   heading(Angle::Radians(fixed_zero)),
   side_display_type(1)
{
  data.Clear();
}

bool
FlarmTrafficWindow::WarningMode() const
{
  assert(warning < (int)data.traffic.size());
  assert(warning < 0 || data.traffic[warning].IsDefined());
  assert(warning < 0 || data.traffic[warning].HasAlarm());

  return warning >= 0;
}

bool
FlarmTrafficWindow::on_resize(unsigned width, unsigned height)
{
  PaintWindow::on_resize(width, height);

  // Calculate Radar size
  radius = min(height, width) / 2 - padding;
  radar_mid.x = width / 2;
  radar_mid.y = height / 2;

  return true;
}

void
FlarmTrafficWindow::SetTarget(int i)
{
  assert(i < (int)data.traffic.size());
  assert(i < 0 || data.traffic[i].IsDefined());

  if (selection == i)
    return;

  selection = i;
  invalidate();
}

/**
 * Tries to select the next target, if impossible selection = -1
 */
void
FlarmTrafficWindow::NextTarget()
{
  // If warning is displayed -> prevent selector movement
  if (WarningMode())
    return;

  assert(selection < (int)data.traffic.size());

  const FlarmTraffic *traffic;
  if (selection >= 0)
    traffic = data.NextTraffic(&data.traffic[selection]);
  else
    traffic = NULL;

  if (traffic == NULL)
    traffic = data.FirstTraffic();

  SetTarget(traffic);
}

/**
 * Tries to select the previous target, if impossible selection = -1
 */
void
FlarmTrafficWindow::PrevTarget()
{
  // If warning is displayed -> prevent selector movement
  if (WarningMode())
    return;

  assert(selection < (int)data.traffic.size());

  const FlarmTraffic *traffic;
  if (selection >= 0)
    traffic = data.PreviousTraffic(&data.traffic[selection]);
  else
    traffic = NULL;

  if (traffic == NULL)
    traffic = data.LastTraffic();

  SetTarget(traffic);
}

/**
 * Checks whether the selection is still on the valid target and if not tries
 * to select the next one
 */
void
FlarmTrafficWindow::UpdateSelector(const FlarmId id, const RasterPoint pt)
{
  // Update #selection
  if (!id.defined())
    SetTarget(-1);
  else
    SetTarget(id);

  // If we don't have a valid selection and we can't find
  // a target close to to the RasterPoint we select the next one
  // on the internal list
  if (selection < 0 && (
      pt.x < 0 || pt.y < 0 ||
      !SelectNearTarget(pt.x, pt.y, radius * 2)) )
    NextTarget();
}

/**
 * Iterates through the traffic array, finds the target with the highest
 * alarm level and saves it to "warning".
 */
void
FlarmTrafficWindow::UpdateWarnings()
{
  const FlarmTraffic *alert = data.FindMaximumAlert();
  warning = alert != NULL
    ? (int)data.TrafficIndex(alert)
    : - 1;
}

/**
 * This should be called when the radar needs to be repainted
 */
void
FlarmTrafficWindow::Update(Angle new_direction, const FlarmState &new_data,
                           const SETTINGS_TEAMCODE &new_settings)
{
  FlarmId selection_id;
  RasterPoint pt;
  if (!small && data.available && selection >= 0) {
    selection_id = data.traffic[selection].id;
    pt = sc[selection];
  } else {
    selection_id.clear();
    pt.x = -100;
    pt.y = -100;
  }

  heading = new_direction;
  fr.SetAngle(-heading);
  fir.SetAngle(heading);
  data = new_data;
  settings = new_settings;

  UpdateWarnings();
  UpdateSelector(selection_id, pt);

  invalidate();
}

/**
 * Returns the distance to the own plane in pixels
 * @param d Distance in meters to the own plane
 */
fixed
FlarmTrafficWindow::RangeScale(fixed d) const
{
  d = d / distance;
  return min(d, fixed_one) * radius;
}

/**
 * Paints a "No Traffic" sign on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::PaintRadarNoTraffic(Canvas &canvas) const
{
  if (small)
    return;

  const TCHAR* str = _("No Traffic");
  canvas.select(look.no_traffic_font);
  PixelSize ts = canvas.text_size(str);
  canvas.set_text_color(look.default_color);
  canvas.text(radar_mid.x - (ts.cx / 2), radar_mid.y - (radius / 2), str);
}

/**
 * Paints the traffic symbols on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::PaintRadarTarget(Canvas &canvas,
                                     const FlarmTraffic &traffic,
                                     unsigned i)
{
  // Save relative East/North
  fixed x = traffic.relative_east;
  fixed y = -traffic.relative_north;

  // Calculate the distance in pixels
  fixed scale = RangeScale(traffic.distance);

  // Don't display distracting, far away targets in WarningMode
  if (WarningMode() && !traffic.HasAlarm() && scale == fixed(radius))
    return;

  // x and y are not between 0 and 1 (distance will be handled via scale)
  if (!traffic.distance.IsZero()) {
    x /= traffic.distance;
    y /= traffic.distance;
  } else {
    x = fixed_zero;
    y = fixed_zero;
  }

  if (!enable_north_up) {
    // Rotate x and y to have a track up display
    FastRotation::Pair p = fr.Rotate(x, y);
    x = fixed(p.first);
    y = fixed(p.second);
  }

  // Calculate screen coordinates
  sc[i].x = radar_mid.x + iround(x * scale);
  sc[i].y = radar_mid.y + iround(y * scale);

  const Color *text_color;
  const Pen *target_pen, *circle_pen;
  const Brush *target_brush, *arrow_brush;
  bool hollow_brush = false;
  unsigned circles = 0;

  // Set the arrow color depending on alarm level
  switch (traffic.alarm_level) {
  case FlarmTraffic::ALARM_LOW:
    text_color = &look.default_color;
    target_pen = circle_pen = &look.warning_pen;
    target_brush = &look.warning_brush;
    arrow_brush = &look.default_brush;
    circles = 1;
    break;
  case FlarmTraffic::ALARM_IMPORTANT:
  case FlarmTraffic::ALARM_URGENT:
    text_color = &look.default_color;
    target_pen = circle_pen = &look.alarm_pen;
    target_brush = &look.alarm_brush;
    arrow_brush = &look.default_brush;
    circles = 2;
    break;
  case FlarmTraffic::ALARM_NONE:
    if (WarningMode()) {
      text_color = &look.passive_color;
      target_pen = &look.passive_pen;
      arrow_brush = &look.passive_brush;
      hollow_brush = true;
    } else {
      // Search for team color
      FlarmFriends::Color team_color = FlarmFriends::GetFriendColor(traffic.id);

      // If no color found but target is teammate
      if (team_color == FlarmFriends::NONE &&
          settings.TeamFlarmTracking &&
          traffic.id == settings.TeamFlarmIdTarget)
        // .. use yellow color
        team_color = FlarmFriends::GREEN;

      // If team color found -> draw a colored circle around the target
      if (team_color != FlarmFriends::NONE) {
        switch (team_color) {
        case FlarmFriends::GREEN:
          circle_pen = &look.team_pen_green;
          break;
        case FlarmFriends::BLUE:
          circle_pen = &look.team_pen_blue;
          break;
        case FlarmFriends::YELLOW:
          circle_pen = &look.team_pen_yellow;
          break;
        case FlarmFriends::MAGENTA:
          circle_pen = &look.team_pen_magenta;
          break;
        default:
          break;
        }

        circles = 1;
      }

      if (!small && static_cast<unsigned> (selection) == i) {
        text_color = &look.selection_color;
        target_brush = arrow_brush = &look.selection_brush;
        target_pen = &look.selection_pen;
      } else {
        hollow_brush = true;
        if (traffic.IsPassive()) {
          text_color = &look.passive_color;
          target_pen = &look.passive_pen;
          arrow_brush = &look.passive_brush;
        } else {
          text_color = &look.default_color;
          target_pen = &look.default_pen;
          arrow_brush = &look.default_brush;
        }
      }
    }
    break;
  }

  if (circles > 0) {
    canvas.hollow_brush();
    canvas.select(*circle_pen);
    canvas.circle(sc[i].x, sc[i].y, Layout::FastScale(small ? 8 : 16));
    if (circles == 2)
      canvas.circle(sc[i].x, sc[i].y, Layout::FastScale(small ? 10 : 19));
  }

  // Create an arrow polygon
  RasterPoint Arrow[5];
  if (small) {
    Arrow[0].x = -3;
    Arrow[0].y = 4;
    Arrow[1].x = 0;
    Arrow[1].y = -5;
    Arrow[2].x = 3;
    Arrow[2].y = 4;
    Arrow[3].x = 0;
    Arrow[3].y = 2;
    Arrow[4].x = -3;
    Arrow[4].y = 4;
  } else {
    Arrow[0].x = -6;
    Arrow[0].y = 8;
    Arrow[1].x = 0;
    Arrow[1].y = -10;
    Arrow[2].x = 6;
    Arrow[2].y = 8;
    Arrow[3].x = 0;
    Arrow[3].y = 5;
    Arrow[4].x = -6;
    Arrow[4].y = 8;
  }

  // Rotate and shift the arrow
  PolygonRotateShift(Arrow, 5, sc[i].x, sc[i].y,
                     traffic.track - (enable_north_up ?
                                             Angle::Zero() : heading));

  // Select pen and brush
  canvas.select(*target_pen);
  if (hollow_brush)
    canvas.hollow_brush();
  else
    canvas.select(*target_brush);

  // Draw the polygon
  canvas.polygon(Arrow, 5);

  if (small) {
    if (WarningMode() && !traffic.HasAlarm())
      return;

    const short relalt =
        iround(Units::ToUserAltitude(traffic.relative_altitude) / 100);

    // if (relative altitude is other than zero)
    if (relalt == 0)
      return;

    // Write the relativ altitude devided by 100 to the Buffer
    TCHAR buffer[10];
    _stprintf(buffer, _T("%d"), abs(relalt));

    // Select font
    canvas.background_transparent();
    canvas.select(look.side_info_font);
    canvas.set_text_color(*text_color);

    // Calculate size of the output string
    PixelSize tsize = canvas.text_size(buffer);

    int dist = Layout::FastScale(traffic.HasAlarm() ? 12 : 8);

    // Draw string
    canvas.text(sc[i].x + dist, sc[i].y - tsize.cy / 2, buffer);

    // Set target_brush for the up/down arrow
    canvas.select(*arrow_brush);
    canvas.null_pen();

    // Prepare the triangular polygon
    RasterPoint triangle[4];
    triangle[0].x = 0;
    triangle[0].y = -4;
    triangle[1].x = 3;
    triangle[1].y = 0;
    triangle[2].x = -3;
    triangle[2].y = 0;

    // Flip = -1 for arrow pointing downwards
    short flip = 1;
    if (relalt < 0)
      flip = -1;

    // Shift the arrow to the right position
    for (int j = 0; j < 3; j++) {
      triangle[j].x = Layout::FastScale(triangle[j].x);
      triangle[j].y = Layout::FastScale(triangle[j].y);

      triangle[j].x = sc[i].x + dist + triangle[j].x + tsize.cx / 2;
      triangle[j].y = sc[i].y + flip * (triangle[j].y  - tsize.cy / 2);
    }
    triangle[3].x = triangle[0].x;
    triangle[3].y = triangle[0].y;

    // Draw the arrow
    canvas.TriangleFan(triangle, 4);

    return;
  }

  // if warning exists -> don't draw vertical speeds
  if (WarningMode())
    return;

  // if vertical speed to small or negative -> skip this one
  if (side_display_type == 1 &&
      (!traffic.climb_rate_avg30s_available ||
       traffic.climb_rate_avg30s < fixed(0.5) ||
       (traffic.type != FlarmTraffic::acGlider &&
        traffic.type != FlarmTraffic::acHangGlider &&
        traffic.type != FlarmTraffic::acParaGlider)))
      return;

  // Select font
  canvas.background_transparent();
  canvas.select(look.side_info_font);

  // Format string
  TCHAR tmp[10];

  if (side_display_type == 1)
    Units::FormatUserVSpeed(traffic.climb_rate_avg30s, tmp, 10, false);
  else
    Units::FormatUserArrival(traffic.relative_altitude, tmp, 10, true);

  PixelSize sz = canvas.text_size(tmp);

  // Draw vertical speed shadow
  canvas.set_text_color(COLOR_WHITE);
  canvas.text(sc[i].x + Layout::FastScale(11) + 1,
              sc[i].y - sz.cy / 2 + 1, tmp);
  canvas.text(sc[i].x + Layout::FastScale(11) - 1,
              sc[i].y - sz.cy / 2 - 1, tmp);

  // Select color
  canvas.set_text_color(*text_color);

  // Draw vertical speed
  canvas.text(sc[i].x + Layout::FastScale(11), sc[i].y - sz.cy / 2, tmp);
}

/**
 * Paints the traffic symbols on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::PaintRadarTraffic(Canvas &canvas)
{
  if (!data.available || data.GetActiveTrafficCount() == 0) {
    PaintRadarNoTraffic(canvas);
    return;
  }

  // Iterate through the traffic (normal traffic)
  for (unsigned i = 0; i < data.traffic.size(); ++i) {
    const FlarmTraffic &traffic = data.traffic[i];

    if (!traffic.HasAlarm() &&
        static_cast<unsigned> (selection) != i)
      PaintRadarTarget(canvas, traffic, i);
  }

  if (selection >= 0) {
    const FlarmTraffic &traffic = data.traffic[selection];

    if (!traffic.HasAlarm())
      PaintRadarTarget(canvas, traffic, selection);
  }

  if (!WarningMode())
    return;

  // Iterate through the traffic (alarm traffic)
  for (unsigned i = 0; i < data.traffic.size(); ++i) {
    const FlarmTraffic &traffic = data.traffic[i];

    if (traffic.HasAlarm())
      PaintRadarTarget(canvas, traffic, i);
  }
}

/**
 * Paint a plane symbol in the middle of the radar on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::PaintRadarPlane(Canvas &canvas) const
{
  canvas.select(look.plane_pen);

  int x1, y1, x2, y2;

  x1 = Layout::FastScale(small ? 5 : 10);
  y1 = -Layout::FastScale(small ? 1 : 2);
  x2 = -Layout::FastScale(small ? 5 : 10);
  y2 = -Layout::FastScale(small ? 1 : 2);

  if (enable_north_up) {
    FastIntegerRotation::Pair p = fir.Rotate(x1, y1);
    x1 = p.first;
    y1 = p.second;
    p = fir.Rotate(x2, y2);
    x2 = p.first;
    y2 = p.second;
  }

  canvas.line(radar_mid.x + x1, radar_mid.y + y1,
              radar_mid.x + x2, radar_mid.y + y2);

  x1 = 0;
  y1 = -Layout::FastScale(small ? 3 : 6);
  x2 = 0;
  y2 = Layout::FastScale(small ? 3 : 6);

  if (enable_north_up) {
    FastIntegerRotation::Pair p = fir.Rotate(x1, y1);
    x1 = p.first;
    y1 = p.second;
    p = fir.Rotate(x2, y2);
    x2 = p.first;
    y2 = p.second;
  }

  canvas.line(radar_mid.x + x1, radar_mid.y + y1,
              radar_mid.x + x2, radar_mid.y + y2);

  x1 = Layout::FastScale(small ? 2 : 4);
  y1 = Layout::FastScale(small ? 2 : 4);
  x2 = -Layout::FastScale(small ? 2 : 4);
  y2 = Layout::FastScale(small ? 2 : 4);

  if (enable_north_up) {
    FastIntegerRotation::Pair p = fir.Rotate(x1, y1);
    x1 = p.first;
    y1 = p.second;
    p = fir.Rotate(x2, y2);
    x2 = p.first;
    y2 = p.second;
  }

  canvas.line(radar_mid.x + x1, radar_mid.y + y1,
              radar_mid.x + x2, radar_mid.y + y2);
}

/**
 * Paints the radar circle on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::PaintNorth(Canvas &canvas) const
{
  fixed x = fixed_zero, y = fixed_minus_one;
  if (!enable_north_up) {
    FastRotation::Pair p = fr.Rotate(x, y);
    x = p.first;
    y = p.second;
  }

  canvas.set_text_color(look.background_color);
  canvas.select(look.radar_pen);
  canvas.select(look.radar_brush);
  canvas.background_transparent();
  canvas.select(look.label_font);

  PixelSize s = canvas.text_size(_T("N"));
  canvas.circle(radar_mid.x + iround(x * radius),
                radar_mid.y + iround(y * radius), s.cy * 0.65);
  canvas.text(radar_mid.x + iround(x * radius) - s.cx / 2,
              radar_mid.y + iround(y * radius) - s.cy / 2, _T("N"));
}

/**
 * Paints the radar circle on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::PaintRadarBackground(Canvas &canvas) const
{
  canvas.hollow_brush();
  canvas.select(look.radar_pen);
  canvas.set_text_color(look.radar_color);

  // Paint circles
  canvas.circle(radar_mid.x, radar_mid.y, radius);
  canvas.circle(radar_mid.x, radar_mid.y, radius / 2);

  PaintRadarPlane(canvas);

  if (small)
    return;

  // Paint zoom strings
  canvas.select(look.label_font);
  canvas.background_opaque();
  canvas.set_background_color(look.background_color);

  TCHAR distance_string[10];
  Units::FormatUserDistance(distance, distance_string,
                            ARRAY_SIZE(distance_string));
  PixelSize s = canvas.text_size(distance_string);
  canvas.text(radar_mid.x - s.cx / 2,
              radar_mid.y + radius - s.cy * 0.75, distance_string);

  Units::FormatUserDistance(distance / 2, distance_string,
                            ARRAY_SIZE(distance_string));
  s = canvas.text_size(distance_string);
  canvas.text(radar_mid.x - s.cx / 2,
              radar_mid.y + radius / 2 - s.cy * 0.75, distance_string);

  canvas.background_transparent();

  PaintNorth(canvas);
}

/**
 * This function paints the TrafficRadar onto the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::Paint(Canvas &canvas)
{
  assert(selection < (int)data.traffic.size());
  assert(selection < 0 || data.traffic[selection].IsDefined());
  assert(warning < (int)data.traffic.size());
  assert(warning < 0 || data.traffic[warning].IsDefined());
  assert(warning < 0 || data.traffic[warning].HasAlarm());

  PaintRadarBackground(canvas);
  PaintRadarTraffic(canvas);
}

/**
 * This function is called when the Radar needs repainting.
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::on_paint(Canvas &canvas)
{
  canvas.clear_white();
  Paint(canvas);
}

bool
FlarmTrafficWindow::SelectNearTarget(int x, int y, int max_distance)
{
  int min_distance = 99999;
  int min_id = -1;

  for (unsigned i = 0; i < data.traffic.size(); ++i) {
    // If FLARM target does not exist -> next one
    if (!data.traffic[i].IsDefined())
      continue;

    int distance_sq = (x - sc[i].x) * (x - sc[i].x) +
                      (y - sc[i].y) * (y - sc[i].y);

    if (distance_sq > min_distance
        || distance_sq > max_distance * max_distance)
      continue;

    min_distance = distance_sq;
    min_id = i;
  }

  if (min_id >= 0)
    SetTarget(min_id);

  return min_id >= 0;
}
