/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Units.hpp"
#include "Math/FastRotation.hpp"
#include "Math/Screen.hpp"

#include <assert.h>

static const Color hcWarning(0xFF, 0xA2, 0x00);
static const Color hcAlarm(0xFF, 0x00, 0x00);
static const Color hcStandard(0x00, 0x00, 0x00);
static const Color hcPassive(0x99, 0x99, 0x99);
static const Color hcSelection(0x00, 0x00, 0xFF);
static const Color hcTeam(0x74, 0xFF, 0x00);
static const Color hcBackground(0xFF, 0xFF, 0xFF);
static const Color hcRadar(0x55, 0x55, 0x55);

FlarmTrafficWindow::FlarmTrafficWindow()
  :zoom(2),
   selection(-1), warning(-1),
   direction(Angle::radians(fixed_zero)),
   side_display_type(1)
{
  memset(&data, 0, sizeof(data));
}

bool
FlarmTrafficWindow::WarningMode() const
{
  assert(warning < FLARM_STATE::FLARM_MAX_TRAFFIC);
  assert(warning < 0 || data.FLARM_Traffic[warning].defined());
  assert(warning < 0 || data.FLARM_Traffic[warning].HasAlarm());

  return warning >= 0;
}

bool
FlarmTrafficWindow::on_create()
{
  PaintWindow::on_create();

  hbWarning.set(hcWarning);
  hbAlarm.set(hcAlarm);
  hbSelection.set(hcSelection);
  hbTeam.set(hcTeam);

  hpWarning.set(Layout::FastScale(2), hcWarning);
  hpAlarm.set(Layout::FastScale(2), hcAlarm);
  hpStandard.set(Layout::FastScale(2), hcStandard);
  hpPassive.set(Layout::FastScale(2), hcPassive);
  hpSelection.set(Layout::FastScale(2), hcSelection);

  hpPlane.set(Layout::FastScale(2), hcRadar);
  hpRadar.set(1, hcRadar);

  return true;
}

bool
FlarmTrafficWindow::on_resize(unsigned width, unsigned height)
{
  PaintWindow::on_resize(width, height);

  // Calculate Radar size
  int size = min(height, width);
  radar_size.cx = size - Layout::FastScale(20);
  radar_size.cy = size - Layout::FastScale(20);
  radar_mid.x = width / 2;
  radar_mid.y = height / 2;

  return true;
}

double
FlarmTrafficWindow::GetZoomDistance(unsigned zoom)
{
  switch (zoom) {
  case 0:
    return 500;
  case 1:
    return 1000;
  case 3:
    return 5000;
  case 4:
    return 10000;
  case 2:
  default:
    return 2000;
  }
}

void
FlarmTrafficWindow::GetZoomDistanceString(TCHAR* str1, TCHAR* str2,
                                          unsigned size) const
{
  double z = GetZoomDistance(zoom);
  double z_half = z * 0.5;

  Units::FormatUserDistance(z, str1, size);
  Units::FormatUserDistance(z_half, str2, size);
}

void
FlarmTrafficWindow::SetTarget(int i)
{
  assert(i < FLARM_STATE::FLARM_MAX_TRAFFIC);
  assert(i < 0 || data.FLARM_Traffic[i].defined());

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
  assert(selection < FLARM_STATE::FLARM_MAX_TRAFFIC);

  const FLARM_TRAFFIC *traffic;
  if (selection >= 0)
    traffic = data.NextTraffic(&data.FLARM_Traffic[selection]);
  else
    traffic = NULL;

  if (traffic == NULL)
    traffic = data.FirstTraffic();

  SetTarget(traffic != NULL ? data.TrafficIndex(traffic) : -1);
}

/**
 * Tries to select the previous target, if impossible selection = -1
 */
void
FlarmTrafficWindow::PrevTarget()
{
  assert(selection < FLARM_STATE::FLARM_MAX_TRAFFIC);

  const FLARM_TRAFFIC *traffic;
  if (selection >= 0)
    traffic = data.PreviousTraffic(&data.FLARM_Traffic[selection]);
  else
    traffic = NULL;

  if (traffic == NULL)
    traffic = data.LastTraffic();

  SetTarget(traffic != NULL ? data.TrafficIndex(traffic) : -1);
}

/**
 * Checks whether the selection is still on the valid target and if not tries
 * to select the next one
 */
void
FlarmTrafficWindow::UpdateSelector()
{
  if (!data.FLARM_Available)
    SetTarget(-1);
  else if (selection < 0 || !data.FLARM_Traffic[selection].defined())
    NextTarget();
}

/**
 * Iterates through the traffic array, finds the target with the highest
 * alarm level and saves it to "warning".
 */
void
FlarmTrafficWindow::UpdateWarnings()
{
  const FLARM_TRAFFIC *alert = data.FindMaximumAlert();
  warning = alert != NULL
    ? data.TrafficIndex(alert)
    : - 1;
}

/**
 * This should be called when the radar needs to be repainted
 */
void
FlarmTrafficWindow::Update(Angle new_direction, const FLARM_STATE &new_data,
                           const SETTINGS_TEAMCODE &new_settings)
{
  direction = new_direction;
  data = new_data;
  settings = new_settings;

  UpdateSelector();
  UpdateWarnings();

  invalidate();
}

/**
 * Returns the distance to the own plane in pixels
 * @param d Distance in meters to the own plane
 */
double
FlarmTrafficWindow::RangeScale(double d) const
{
  d = d / GetZoomDistance(zoom);
  return min(d, 1.0) * radar_size.cx * 0.5;
}

/**
 * Paints the basic info for the selected target on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::PaintTrafficInfo(Canvas &canvas) const
{
  // Don't paint numbers if no plane selected
  if (selection == -1)
    return;

  assert(data.FLARM_Traffic[selection].defined());

  // Temporary string
  TCHAR tmp[20];
  // Temporary string size
  SIZE sz;
  // Shortcut to the selected traffic
  FLARM_TRAFFIC traffic;
  if (WarningMode())
    traffic = data.FLARM_Traffic[warning];
  else
    traffic = data.FLARM_Traffic[selection];

  RECT rc;
  rc.left = min(radar_mid.x - radar_size.cx * 0.5,
                radar_mid.y - radar_size.cy * 0.5);
  rc.top = rc.left;
  rc.right = 2 * radar_mid.x - rc.left;
  rc.bottom = 2 * radar_mid.y - rc.top;

  // Set the text color and background
  switch (traffic.AlarmLevel) {
  case 1:
    canvas.set_text_color(hcWarning);
    break;
  case 2:
  case 3:
    canvas.set_text_color(hcAlarm);
    break;
  case 4:
  case 0:
  default:
    canvas.set_text_color(hcStandard);
    break;
  }
  canvas.select(TitleSmallWindowFont);
  canvas.background_transparent();

  // Climb Rate
  if (!WarningMode()) {
#ifdef FLARM_AVERAGE
    Units::FormatUserVSpeed(traffic.Average30s, tmp, 20);
#else
    Units::FormatUserVSpeed(traffic.ClimbRate, tmp, 20);
#endif
    sz = canvas.text_size(tmp);
    canvas.text(rc.right - sz.cx, rc.top, tmp);
  }

  // Distance
  Units::FormatUserDistance(sqrt(traffic.RelativeEast * traffic.RelativeEast
      + traffic.RelativeNorth * traffic.RelativeNorth), tmp, 20);
  sz = canvas.text_size(tmp);
  canvas.text(rc.left, rc.bottom - sz.cy, tmp);

  // Relative Height
  Units::FormatUserArrival(traffic.RelativeAltitude, tmp, 20);
  sz = canvas.text_size(tmp);
  canvas.text(rc.right - sz.cx, rc.bottom - sz.cy, tmp);

  // ID / Name
  if (traffic.HasName()) {
    canvas.select(InfoWindowFont);
    if (!traffic.HasAlarm()) {
      if (settings.TeamFlarmTracking &&
          traffic.ID == settings.TeamFlarmIdTarget)
        canvas.set_text_color(hcTeam);
      else
        canvas.set_text_color(hcSelection);
    }
    _tcscpy(tmp, traffic.Name);
  } else {
    traffic.ID.format(tmp);
  }
  canvas.text(rc.left, rc.top, tmp);
}

/**
 * Paints a "No Traffic" sign on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::PaintRadarNoTraffic(Canvas &canvas) const
{
  static const TCHAR str[] = _T("No Traffic");
  canvas.select(StatisticsFont);
  SIZE ts = canvas.text_size(str);
  canvas.set_text_color(hcStandard);
  canvas.text(radar_mid.x - (ts.cx / 2), radar_mid.y - (radar_size.cy / 4), str);
}

/**
 * Paints the traffic symbols on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::PaintRadarTarget(Canvas &canvas,
                                     const FLARM_TRAFFIC &traffic,
                                     unsigned i)
{
  // Save relative East/North
  double x, y;
  x = traffic.RelativeEast;
  y = -traffic.RelativeNorth;

  // Calculate the distance in meters
  double d = sqrt(x * x + y * y);

  // Calculate the distance in pixels
  double scale = RangeScale(d);

  // Don't display distracting, far away targets in WarningMode
  if (WarningMode() && !traffic.HasAlarm() && scale == radar_size.cx * 0.5)
    return;

  // x and y are not between 0 and 1 (distance will be handled via scale)
  if (d > 0) {
    x /= d;
    y /= d;
  } else {
    x = 0;
    y = 0;
  }

  // Rotate x and y to have a track up display
  Angle DisplayAngle = -direction;
  // or use .Heading? (no, because heading is not reliable)
  const FastRotation r(DisplayAngle);
  FastRotation::Pair p = r.Rotate(x, y);
  x = p.first;
  y = p.second;

  // Calculate screen coordinates
  sc[i].x = radar_mid.x + iround(x * scale);
  sc[i].y = radar_mid.y + iround(y * scale);

  // Set the arrow color depending on alarm level
  switch (traffic.AlarmLevel) {
  case 1:
    canvas.hollow_brush();
    canvas.select(hpWarning);
    canvas.circle(sc[i].x, sc[i].y, Layout::FastScale(16));
    canvas.select(hbWarning);
    break;
  case 2:
  case 3:
    canvas.hollow_brush();
    canvas.select(hpAlarm);
    canvas.circle(sc[i].x, sc[i].y, Layout::FastScale(16));
    canvas.circle(sc[i].x, sc[i].y, Layout::FastScale(19));
    canvas.select(hbAlarm);
    break;
  case 0:
  case 4:
    if (WarningMode()) {
      canvas.hollow_brush();
      canvas.select(hpPassive);
    } else {
      if (static_cast<unsigned> (selection) == i) {
        canvas.select(hpSelection);
        canvas.select(hbSelection);
      } else {
        canvas.hollow_brush();
        canvas.select(hpStandard);
      }
      if (settings.TeamFlarmTracking &&
          traffic.ID == settings.TeamFlarmIdTarget) {
        canvas.select(hbTeam);
      }
    }
    break;
  }

  // Create an arrow polygon
  POINT Arrow[5];
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

  // Rotate and shift the arrow
  PolygonRotateShift(Arrow, 5, sc[i].x, sc[i].y,
                     traffic.TrackBearing + DisplayAngle);

  // Draw the polygon
  canvas.polygon(Arrow, 5);

  // if warning exists -> don't draw vertical speeds
  if (WarningMode())
    return;

#ifdef FLARM_AVERAGE
  if (side_display_type == 1) {
    // if vertical speed to small or negative -> skip this one
    if (traffic.Average30s < 0.5)
      return;

    // Select font
    canvas.background_transparent();
    canvas.select(MapWindowBoldFont);

    // Format string
    TCHAR tmp[10];
    Units::FormatUserVSpeed(traffic.Average30s, tmp, 10, false);
    SIZE sz = canvas.text_size(tmp);

    // Draw vertical speed shadow
    canvas.set_text_color(Color::WHITE);
    canvas.text(sc[i].x + Layout::FastScale(11) + 1,
                sc[i].y - sz.cy * 0.5 + 1, tmp);
    canvas.text(sc[i].x + Layout::FastScale(11) - 1,
                sc[i].y - sz.cy * 0.5 - 1, tmp);

    // Select color
    if (static_cast<unsigned> (selection) == i)
      canvas.set_text_color(hcSelection);
    else
      canvas.set_text_color(hcStandard);

    // Draw vertical speed
    canvas.text(sc[i].x + Layout::FastScale(11), sc[i].y - sz.cy * 0.5, tmp);
  } else if (side_display_type == 2) {
#endif
    // Select font
    canvas.background_transparent();
    canvas.select(MapWindowBoldFont);

    // Format string
    TCHAR tmp[10];
    Units::FormatUserArrival(traffic.RelativeAltitude, tmp, 10, true);
    SIZE sz = canvas.text_size(tmp);

    // Draw vertical speed shadow
    canvas.set_text_color(Color::WHITE);
    canvas.text(sc[i].x + Layout::FastScale(11) + 1,
                sc[i].y - sz.cy * 0.5 + 1, tmp);
    canvas.text(sc[i].x + Layout::FastScale(11) - 1,
                sc[i].y - sz.cy * 0.5 - 1, tmp);

    // Select color
    if (static_cast<unsigned> (selection) == i)
      canvas.set_text_color(hcSelection);
    else
      canvas.set_text_color(hcStandard);

    // Draw vertical speed
    canvas.text(sc[i].x + Layout::FastScale(11), sc[i].y - sz.cy * 0.5, tmp);
#ifdef FLARM_AVERAGE
  }
#endif
}

/**
 * Paints the traffic symbols on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::PaintRadarTraffic(Canvas &canvas)
{
  if (!data.FLARM_Available || data.GetActiveTrafficCount() == 0) {
    PaintRadarNoTraffic(canvas);
    return;
  }

  // Iterate through the traffic (normal traffic)
  for (unsigned i = 0; i < FLARM_STATE::FLARM_MAX_TRAFFIC; ++i) {
    const FLARM_TRAFFIC &traffic = data.FLARM_Traffic[i];

    if (traffic.defined() && !traffic.HasAlarm() &&
        static_cast<unsigned> (selection) != i)
      PaintRadarTarget(canvas, traffic, i);
  }

  if (selection >= 0 && selection < FLARM_STATE::FLARM_MAX_TRAFFIC) {
    const FLARM_TRAFFIC &traffic = data.FLARM_Traffic[selection];

    if (traffic.defined() && !traffic.HasAlarm())
      PaintRadarTarget(canvas, traffic, selection);
  }

  if (!WarningMode())
    return;

  // Iterate through the traffic (alarm traffic)
  for (unsigned i = 0; i < FLARM_STATE::FLARM_MAX_TRAFFIC; ++i) {
    const FLARM_TRAFFIC &traffic = data.FLARM_Traffic[i];

    if (traffic.defined() && traffic.HasAlarm())
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
  canvas.select(hpPlane);
  canvas.line(radar_mid.x + Layout::FastScale(10),
              radar_mid.y - Layout::FastScale(2),
              radar_mid.x - Layout::FastScale(10),
              radar_mid.y - Layout::FastScale(2));
  canvas.line(radar_mid.x,
              radar_mid.y - Layout::FastScale(6),
              radar_mid.x,
              radar_mid.y + Layout::FastScale(6));
  canvas.line(radar_mid.x + Layout::FastScale(4),
              radar_mid.y + Layout::FastScale(4),
              radar_mid.x - Layout::FastScale(4),
              radar_mid.y + Layout::FastScale(4));
}

/**
 * Paints the radar circle on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::PaintRadarBackground(Canvas &canvas) const
{
  canvas.hollow_brush();
  canvas.select(hpRadar);
  canvas.set_text_color(hcRadar);

  // Paint circles
  canvas.circle(radar_mid.x, radar_mid.y, radar_size.cx * 0.5);
  canvas.circle(radar_mid.x, radar_mid.y, radar_size.cx * 0.25);

  // Paint zoom strings
  TCHAR str1[10], str2[10];
  GetZoomDistanceString(str1, str2, 10);
  canvas.select(MapWindowFont);
  canvas.background_opaque();
  SIZE sz1 = canvas.text_size(str1);
  canvas.text(radar_mid.x - sz1.cx / 2,
              radar_mid.y + radar_size.cx * 0.5 - sz1.cy * 0.75, str1);
  SIZE sz2 = canvas.text_size(str2);
  canvas.text(radar_mid.x - sz2.cx / 2,
              radar_mid.y + radar_size.cx * 0.25 - sz2.cy * 0.75, str2);
  canvas.background_transparent();
}

/**
 * This function is called when the Radar needs repainting.
 * @param Sender WindowControl that send the "repaint" message
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficWindow::on_paint(Canvas &canvas)
{
  canvas.white_pen();
  canvas.white_brush();
  canvas.clear();

  PaintRadarBackground(canvas);
  PaintRadarPlane(canvas);
  PaintTrafficInfo(canvas);
  PaintRadarTraffic(canvas);
}

void
FlarmTrafficWindow::SelectNearTarget(int x, int y)
{
  int min_distance = 99999;
  int min_id = -1;

  for (unsigned i = 0; i < FLARM_STATE::FLARM_MAX_TRAFFIC; ++i) {
    // If FLARM target does not exist -> next one
    if (!data.FLARM_Traffic[i].defined())
      continue;

    int distance_sq = (x - sc[i].x) * (x - sc[i].x) +
                      (y - sc[i].y) * (y - sc[i].y);

    if (distance_sq > min_distance
        || distance_sq > Layout::FastScale(15) * Layout::FastScale(15))
      continue;

    min_distance = distance_sq;
    min_id = i;
  }

  if (min_id >= 0)
    SetTarget(min_id);
}
