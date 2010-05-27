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

/**
 * @file
 * The FLARM Traffic dialog displaying a radar screen with the moving
 * FLARM targets in track up orientation. The target can be selected and basic
 * information is given on the selected target. When a warning/alarm is present
 * the target with the highest alarm level is automatically selected and
 * highlighted in orange or red (depending on the level)
 */

#include "Dialogs/Internal.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Math/FastRotation.hpp"
#include "Math/Screen.hpp"
#include "MainWindow.hpp"
#include "Profile.hpp"
#include "Compiler.h"

static const Color hcWarning(0xFF, 0xA2, 0x00);
static const Color hcAlarm(0xFF, 0x00, 0x00);
static const Color hcStandard(0x00, 0x00, 0x00);
static const Color hcPassive(0x99, 0x99, 0x99);
static const Color hcSelection(0x00, 0x00, 0xFF);
static const Color hcTeam(0x74, 0xFF, 0x00);
static const Color hcBackground(0xFF, 0xFF, 0xFF);
static const Color hcRadar(0x55, 0x55, 0x55);

/**
 * A Window which renders FLARM traffic.
 */
class FlarmTrafficWindow : public PaintWindow {
protected:
  unsigned zoom;
  int selection;
  int warning;
  POINT radar_mid;
  SIZE radar_size;
  POINT sc[FLARM_STATE::FLARM_MAX_TRAFFIC];

  Angle direction;
  FLARM_STATE data;

public:
  int side_display_type;

public:
  FlarmTrafficWindow();

public:
  bool WarningMode() const;

  const FLARM_TRAFFIC *GetTarget() const {
    return selection >= 0 && data.FLARM_Traffic[selection].defined()
      ? &data.FLARM_Traffic[selection]
      : NULL;
  }

  void SetTarget(int i);
  void NextTarget();
  void PrevTarget();
  void SelectNearTarget(int x, int y);

protected:
  static double GetZoomDistance(unsigned zoom);

  void GetZoomDistanceString(TCHAR* str1, TCHAR* str2, unsigned size) const;
  double RangeScale(double d) const;

  void UpdateSelector();
  void UpdateWarnings();
  void Update(Angle new_direction, const FLARM_STATE &new_data);
  void PaintTrafficInfo(Canvas &canvas) const;
  void PaintRadarNoTraffic(Canvas &canvas) const;
  void PaintRadarTarget(Canvas &canvas, const FLARM_TRAFFIC &traffic,
                        unsigned i);
  void PaintRadarTraffic(Canvas &canvas);
  void PaintRadarPlane(Canvas &canvas) const;
  void PaintRadarBackground(Canvas &canvas) const;

protected:
  virtual bool on_resize(unsigned width, unsigned height);
  virtual void on_paint(Canvas &canvas);
};

FlarmTrafficWindow::FlarmTrafficWindow()
  :zoom(2),
   selection(-1), warning(-1),
   direction(Angle::radians(fixed_zero)),
   side_display_type(1)
{
  memset(&data, 0, sizeof(data));
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

/**
 * A Window which renders FLARM traffic, with user interaction.
 */
class FlarmTrafficControl : public FlarmTrafficWindow {
protected:
  bool enable_auto_zoom;

public:
  FlarmTrafficControl()
    :enable_auto_zoom(true) {}

protected:
  void CalcAutoZoom();

public:
  void Update(Angle new_direction, const FLARM_STATE &new_data);

  bool GetAutoZoom() const {
    return enable_auto_zoom;
  }

  void SetAutoZoom(bool enabled);
  void ToggleAutoZoom() {
    SetAutoZoom(!enable_auto_zoom);
  }

  void ZoomOut();
  void ZoomIn();

protected:
  virtual bool on_create();
  virtual bool on_mouse_down(int x, int y);
};

static WndForm *wf = NULL;
static FlarmTrafficControl *wdf;

bool
FlarmTrafficControl::on_create()
{
  FlarmTrafficWindow::on_create();

  Profile::Get(szProfileFlarmSideData, side_display_type);
  Profile::Get(szProfileFlarmAutoZoom, enable_auto_zoom);

  return true;
}

bool
FlarmTrafficWindow::WarningMode() const
{
  if (warning < 0 || warning >= FLARM_STATE::FLARM_MAX_TRAFFIC)
    return false;

  if (data.FLARM_Traffic[warning].defined())
    return true;

  return false;
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
  for (int i = selection + 1; i < FLARM_STATE::FLARM_MAX_TRAFFIC; i++) {
    if (data.FLARM_Traffic[i].defined()) {
      SetTarget(i);
      return;
    }
  }
  for (int i = 0; i < selection; i++) {
    if (data.FLARM_Traffic[i].defined()) {
      SetTarget(i);
      return;
    }
  }

  SetTarget(-1);
}

/**
 * Tries to select the previous target, if impossible selection = -1
 */
void
FlarmTrafficWindow::PrevTarget()
{
  for (int i = selection - 1; i >= 0; i--) {
    if (data.FLARM_Traffic[i].defined()) {
      SetTarget(i);
      return;
    }
  }
  for (int i = FLARM_STATE::FLARM_MAX_TRAFFIC - 1; i > selection; i--) {
    if (data.FLARM_Traffic[i].defined()) {
      SetTarget(i);
      return;
    }
  }

  SetTarget(-1);
}

/**
 * Checks whether the selection is still on the valid target and if not tries
 * to select the next one
 */
void
FlarmTrafficWindow::UpdateSelector()
{
  if (!data.FLARM_Available || data.GetActiveTrafficCount() == 0) {
    SetTarget(-1);
    return;
  }

  if (selection == -1 || !data.FLARM_Traffic[selection].defined())
    NextTarget();
}

/**
 * Iterates through the traffic array, finds the target with the highest
 * alarm level and saves it to "warning".
 */
void
FlarmTrafficWindow::UpdateWarnings()
{
  bool found = false;

  for (unsigned i = 0; i < FLARM_STATE::FLARM_MAX_TRAFFIC; ++i) {
    // if Traffic[i] not defined -> goto next one
    if (!data.FLARM_Traffic[i].defined())
      continue;

    // if current target has no alarm -> goto next one
    if (!data.FLARM_Traffic[i].HasAlarm())
      continue;

    // remember that a warning exists
    found = true;
    // if it did not before -> save the id and goto next one
    if (!WarningMode()) {
      warning = i;
      continue;
    }

    // if it did before and the other level was higher -> just goto next one
    if (data.FLARM_Traffic[warning].AlarmLevel >
        data.FLARM_Traffic[i].AlarmLevel) {
      continue;
    }

    // if the other level was lower -> save the id and goto next one
    if (data.FLARM_Traffic[warning].AlarmLevel <
        data.FLARM_Traffic[i].AlarmLevel) {
      warning = i;
      continue;
    }

    // if the levels match -> let the distance decide (smaller distance wins)
    double dist_w = sqrt(
        data.FLARM_Traffic[warning].RelativeAltitude *
        data.FLARM_Traffic[warning].RelativeAltitude +
        data.FLARM_Traffic[warning].RelativeEast *
        data.FLARM_Traffic[warning].RelativeEast +
        data.FLARM_Traffic[warning].RelativeNorth *
        data.FLARM_Traffic[warning].RelativeNorth);
    double dist_i = sqrt(
        data.FLARM_Traffic[i].RelativeAltitude *
        data.FLARM_Traffic[i].RelativeAltitude +
        data.FLARM_Traffic[i].RelativeEast *
        data.FLARM_Traffic[i].RelativeEast +
        data.FLARM_Traffic[i].RelativeNorth *
        data.FLARM_Traffic[i].RelativeNorth);

    if (dist_w > dist_i) {
      warning = i;
    }
  }

  // If no warning was found -> set warning to -1
  if (!found)
    warning = -1;
}

void
FlarmTrafficControl::SetAutoZoom(bool enabled)
{
  enable_auto_zoom = enabled;
  Profile::Set(szProfileFlarmAutoZoom, enabled);
  ((WndButton *)wf->FindByName(_T("cmdAutoZoom")))->
      SetForeColor(enable_auto_zoom ? Color::BLUE : Color::BLACK);
}

void
FlarmTrafficControl::CalcAutoZoom()
{
  bool warning_mode = WarningMode();
  double zoom_dist = 0;

  for (unsigned i = 0; i < FLARM_STATE::FLARM_MAX_TRAFFIC; i++) {
    if (warning_mode && !data.FLARM_Traffic[i].HasAlarm())
      continue;

    double dist = data.FLARM_Traffic[i].RelativeNorth
                * data.FLARM_Traffic[i].RelativeNorth
                + data.FLARM_Traffic[i].RelativeEast
                * data.FLARM_Traffic[i].RelativeEast;

    zoom_dist = max(dist, zoom_dist);
  }

  for (unsigned i = 0; i <= 4; i++) {
    if (i == 4 || (GetZoomDistance(i) * GetZoomDistance(i)) >= zoom_dist) {
      zoom = i;
      break;
    }
  }
}

/**
 * This should be called when the radar needs to be repainted
 */
void
FlarmTrafficWindow::Update(Angle new_direction, const FLARM_STATE &new_data)
{
  direction = new_direction;
  data = new_data;

  UpdateSelector();
  UpdateWarnings();

  invalidate();
}

void
FlarmTrafficControl::Update(Angle new_direction, const FLARM_STATE &new_data)
{
  FlarmTrafficWindow::Update(new_direction, new_data);

  if (enable_auto_zoom || WarningMode())
    CalcAutoZoom();
}

/**
 * Zoom out one step
 */
void
FlarmTrafficControl::ZoomOut()
{
  if (WarningMode())
    return;

  if (zoom < 4)
    zoom++;

  SetAutoZoom(false);
  invalidate();
}

/**
 * Zoom in one step
 */
void
FlarmTrafficControl::ZoomIn()
{
  if (WarningMode())
    return;

  if (zoom > 0)
    zoom--;

  SetAutoZoom(false);
  invalidate();
}

/**
 * This event handler is called when the "Details" button is pressed
 */
static void
OnDetailsClicked(gcc_unused WndButton &button)
{

  // If warning is displayed -> prevent from opening details dialog
  if (wdf->WarningMode())
    return;

  // Don't open the details dialog if no plane selected
  const FLARM_TRAFFIC *traffic = wdf->GetTarget();
  if (traffic == NULL)
    return;

  // Show the details dialog
  dlgFlarmTrafficDetailsShowModal(traffic->ID);
}

/**
 * This event handler is called when the "ZoomIn (+)" button is pressed
 */
static void
OnZoomInClicked(gcc_unused WndButton &button)
{
  wdf->ZoomIn();
}

/**
 * This event handler is called when the "ZoomOut (-)" button is pressed
 */
static void
OnZoomOutClicked(gcc_unused WndButton &button)
{
  wdf->ZoomOut();
}

/**
 * This event handler is called when the "Prev (<)" button is pressed
 */
static void
OnPrevClicked(gcc_unused WndButton &button)
{
  // If warning is displayed -> prevent selector movement
  if (wdf->WarningMode())
    return;

  wdf->PrevTarget();
}

/**
 * This event handler is called when the "Next (>)" button is pressed
 */
static void
OnNextClicked(gcc_unused WndButton &button)
{
  // If warning is displayed -> prevent selector movement
  if (wdf->WarningMode())
    return;

  wdf->NextTarget();
}

/**
 * This event handler is called when the "Close" button is pressed
 */
static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

/**
 * This event handler is called when the "Avg/Alt" button is pressed
 */
static void
OnSwitchDataClicked(gcc_unused WndButton &button)
{
  wdf->side_display_type++;
  if (wdf->side_display_type > 2)
    wdf->side_display_type = 1;

  Profile::Set(szProfileFlarmSideData, wdf->side_display_type);
}

/**
 * This event handler is called when the "AutoZoom" button is pressed
 */
static void
OnAutoZoomClicked(gcc_unused WndButton &button)
{
  wdf->ToggleAutoZoom();
}

/**
 * This event handler is called when a key is pressed
 * @param key_code The key code of the pressed key
 * @return True if the event was handled, False otherwise
 */
static bool
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  (void)Sender;

  switch (key_code) {
  case VK_UP:
    if (!has_pointer())
      break;

    wdf->ZoomIn();
    return true;
  case VK_DOWN:
    if (!has_pointer())
      break;

    wdf->ZoomOut();
    return true;
  case VK_LEFT:
  case '6':
    wdf->PrevTarget();
    return true;
  case VK_RIGHT:
  case '7':
    wdf->NextTarget();
    return true;
  }

  return false;
}

/**
 * This event handler is called when the timer is activated and triggers the
 * repainting of the radar
 */
static int
OnTimerNotify(WindowControl * Sender)
{
  (void)Sender;
  wdf->Update(XCSoarInterface::Basic().TrackBearing,
              XCSoarInterface::Basic().flarm);
  return 0;
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
  if (selection == -1 ||
      !data.FLARM_Traffic[selection].defined())
    return;

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
      if (XCSoarInterface::SettingsComputer().TeamFlarmTracking &&
          traffic.ID == XCSoarInterface::SettingsComputer().TeamFlarmIdTarget)
        canvas.set_text_color(hcTeam);
      else
        canvas.set_text_color(hcSelection);
    }
    _tcscpy(tmp, traffic.Name);
  } else {
    _stprintf(tmp, _T("%lX"), traffic.ID);
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
  static TCHAR str[] = _T("No Traffic");
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
  static const Brush hbWarning(hcWarning);
  static const Brush hbAlarm(hcAlarm);
  static const Brush hbSelection(hcSelection);
  static const Brush hbTeam(hcTeam);

  static const Pen hpWarning(Layout::FastScale(2), hcWarning);
  static const Pen hpAlarm(Layout::FastScale(2), hcAlarm);
  static const Pen hpStandard(Layout::FastScale(2), hcStandard);
  static const Pen hpPassive(Layout::FastScale(2), hcPassive);
  static const Pen hpSelection(Layout::FastScale(2), hcSelection);

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
      if (XCSoarInterface::SettingsComputer().TeamFlarmTracking &&
          traffic.ID == XCSoarInterface::SettingsComputer().TeamFlarmIdTarget) {
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

    // If FLARM target does not exist -> next one
    if (!traffic.defined())
      continue;

    if (!traffic.HasAlarm() && static_cast<unsigned> (selection) != i)
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

    // If FLARM target does not exist -> next one
    if (!traffic.defined())
      continue;

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
  static const Pen hpPlane(Layout::FastScale(2), hcRadar);

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
  static const Pen hpRadar(1, hcRadar);

  canvas.hollow_brush();
  canvas.select(hpRadar);
  canvas.set_text_color(hcRadar);

  // Paint circles
  canvas.circle(radar_mid.x, radar_mid.y, radar_size.cx * 0.5);
  canvas.circle(radar_mid.x, radar_mid.y, radar_size.cx * 0.25);

  // Paint zoom strings
  static TCHAR str1[10], str2[10];
  GetZoomDistanceString(str1, str2, 10);
  static SIZE sz1, sz2;
  canvas.select(MapWindowFont);
  canvas.background_opaque();
  sz1 = canvas.text_size(str1);
  canvas.text(radar_mid.x - sz1.cx / 2,
              radar_mid.y + radar_size.cx * 0.5 - sz1.cy * 0.75, str1);
  sz2 = canvas.text_size(str2);
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

  invalidate();
}

bool
FlarmTrafficControl::on_mouse_down(int x, int y)
{
  if (!data.FLARM_Traffic[warning].defined())
    SelectNearTarget(x, y);

  return true;
}

static Window *
OnCreateFlarmTrafficControl(ContainerWindow &parent, int left, int top,
                            unsigned width, unsigned height,
                            const WindowStyle style)
{
  wdf = new FlarmTrafficControl();
  wdf->set(parent, left, top, width, height, style);

  return wdf;
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnCreateFlarmTrafficControl),
  DeclareCallBackEntry(OnTimerNotify),
  DeclareCallBackEntry(NULL)
};

/**
 * The function opens the FLARM Traffic dialog
 */
void
dlgFlarmTrafficShowModal()
{
  // Load dialog from XML
  if (Layout::landscape)
    wf = dlgLoadFromXML(CallBackTable, _T("dlgFlarmTraffic_L.xml"),
        XCSoarInterface::main_window, _T("IDR_XML_FLARMTRAFFIC_L"));
  else
    wf = dlgLoadFromXML(CallBackTable, _T("dlgFlarmTraffic.xml"),
        XCSoarInterface::main_window, _T("IDR_XML_FLARMTRAFFIC"));

  if (!wf)
    return;

  // Set dialog events
  wf->SetKeyDownNotify(FormKeyDown);
  wf->SetTimerNotify(OnTimerNotify);

  // Set button events
  ((WndButton *)wf->FindByName(_T("cmdDetails")))->
      SetOnClickNotify(OnDetailsClicked);
  ((WndButton *)wf->FindByName(_T("cmdZoomIn")))->
      SetOnClickNotify(OnZoomInClicked);
  ((WndButton *)wf->FindByName(_T("cmdZoomOut")))->
      SetOnClickNotify(OnZoomOutClicked);
  ((WndButton *)wf->FindByName(_T("cmdPrev")))->
      SetOnClickNotify(OnPrevClicked);
  ((WndButton *)wf->FindByName(_T("cmdNext")))->
      SetOnClickNotify(OnNextClicked);
  ((WndButton *)wf->FindByName(_T("cmdClose")))->
      SetOnClickNotify(OnCloseClicked);
  ((WndButton *)wf->FindByName(_T("cmdSwitchData")))->
      SetOnClickNotify(OnSwitchDataClicked);
  ((WndButton *)wf->FindByName(_T("cmdAutoZoom")))->
      SetOnClickNotify(OnAutoZoomClicked);

  // Update Radar and Selection for the first time
  wdf->Update(XCSoarInterface::Basic().TrackBearing,
              XCSoarInterface::Basic().flarm);

  // Get the last chosen Side Data configuration
  ((WndButton *)wf->FindByName(_T("cmdAutoZoom")))->
    SetForeColor(wdf->GetAutoZoom() ? Color::BLUE : Color::BLACK);

  // Show the dialog
  wf->ShowModal();

  // After dialog closed -> delete it
  delete wf;
}
