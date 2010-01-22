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

#include "Dialogs/Internal.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "Blackboard.hpp"
#include "UtilsFLARM.hpp"
#include "Math/Earth.hpp"
#include "Math/FastRotation.hpp"
#include "Math/Screen.hpp"
#include "MainWindow.hpp"

#include <assert.h>

#define FLARMMAXRANGE 2000

static WndForm *wf = NULL;
static WndOwnerDrawFrame *wdf = NULL;
static unsigned zoom = 0;
static int selection = -1;
static POINT radar_mid;
static SIZE radar_size;

static void
NextTarget()
{
  for (int i = selection + 1; i < FLARM_STATE::FLARM_MAX_TRAFFIC; i++) {
    if (XCSoarInterface::Basic().flarm.FLARM_Traffic[i].defined()) {
      selection = i;
      return;
    }
  }
  for (int i = 0; i < selection; i++) {
    if (XCSoarInterface::Basic().flarm.FLARM_Traffic[i].defined()) {
      selection = i;
      return;
    }
  }
  selection = -1;
}

static void
PrevTarget()
{
  for (int i = selection - 1; i >= 0; i--) {
    if (XCSoarInterface::Basic().flarm.FLARM_Traffic[i].defined()) {
      selection = i;
      return;
    }
  }
  for (int i = FLARM_STATE::FLARM_MAX_TRAFFIC - 1; i > selection; i--) {
    if (XCSoarInterface::Basic().flarm.FLARM_Traffic[i].defined()) {
      selection = i;
      return;
    }
  }
  selection = -1;
}

static void
UpdateSelector()
{
  if (!XCSoarInterface::Basic().flarm.FLARM_Available ||
      XCSoarInterface::Basic().flarm.GetActiveTrafficCount() == 0) {
    selection = -1;
    return;
  }

  if (selection == -1 ||
      !XCSoarInterface::Basic().flarm.FLARM_Traffic[selection].defined())
    NextTarget();
}

static void
Update()
{
  UpdateSelector();
  wdf->invalidate();
}

static void
OnDetailsClicked(WindowControl * Sender)
{
  (void)Sender;
  MessageBoxX(_T("This feature is not implemented yet."),
              _T(""), MB_OK);
}

static void
OnZoomInClicked(WindowControl * Sender)
{
  (void)Sender;
  MessageBoxX(_T("This feature is not implemented yet."),
              _T(""), MB_OK);
}

static void
OnZoomOutClicked(WindowControl * Sender)
{
  (void)Sender;
  MessageBoxX(_T("This feature is not implemented yet."),
              _T(""), MB_OK);
}

static void
OnPrevClicked(WindowControl * Sender)
{
  (void)Sender;
  PrevTarget();
  Update();
}

static void
OnNextClicked(WindowControl * Sender)
{
  (void)Sender;
  NextTarget();
  Update();
}

static void
OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static bool
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  (void)Sender;

  switch (key_code) {
  case VK_LEFT:
  case '6':
    PrevTarget();
    Update();
    return true;
  case VK_RIGHT:
  case '7':
    NextTarget();
    Update();
    return true;

  default:
    return false;
  }
}

static int
OnTimerNotify(WindowControl * Sender)
{
  (void)Sender;
  Update();
  return 0;
}

/**
 * Returns the distance scaled at a quadratic(?) scale
 * @param d Distance to the own plane
 */
static int
RangeScale(double d)
{
  double drad = max(0.0, 1.0 - d / FLARMMAXRANGE);
  return iround(radar_size.cx * 0.5 * (1.0 - drad * drad));
}

static void
PaintRadarNoTraffic(Canvas &canvas) {
  static TCHAR str[] = _T("No Traffic");
  SIZE ts = canvas.text_size(str);
  canvas.select(StatisticsFont);
  canvas.black_pen();
  canvas.select(wdf->GetBackBrush());
  canvas.text(radar_mid.x - (ts.cx / 2), radar_mid.y - (radar_size.cy / 4), str);
}

static void
PaintRadarTraffic(Canvas &canvas) {
  if (!XCSoarInterface::Basic().flarm.FLARM_Available ||
      XCSoarInterface::Basic().flarm.GetActiveTrafficCount() == 0) {
    PaintRadarNoTraffic(canvas);
    return;
  }

  static Brush hbWarning(Color(0xFF, 0xA2, 0x00));
  static Brush hbAlarm(Color::RED);
  static Brush hbStandard(Color::BLACK);
  static Brush hbSelection(Color(0x12, 0xFF, 0x00));
  static Pen hpWarning(Layout::FastScale(2), Color(0xFF, 0xA2, 0x00));
  static Pen hpAlarm(Layout::FastScale(2), Color::RED);
  static Pen hpStandard(Layout::FastScale(2), Color::BLACK);
  static Pen hpSelection(Layout::FastScale(2), Color(0x12, 0xFF, 0x00));

  for (unsigned i = 0; i < FLARM_STATE::FLARM_MAX_TRAFFIC; ++i) {
    const FLARM_TRAFFIC &traffic = XCSoarInterface::Basic().flarm.FLARM_Traffic[i];

    if (!traffic.defined())
      continue;

    double x, y;
    x = traffic.RelativeEast;
    y = -traffic.RelativeNorth;
    double d = sqrt(x * x + y * y);
    if (d > 0) {
      x /= d;
      y /= d;
    } else {
      x = 0;
      y = 0;
    }
    double dh = traffic.RelativeAltitude;
    double slope = atan2(dh, d) * 2.0 / M_PI; // (-1,1)

    slope = max(-1.0, min(1.0, slope * 2)); // scale so 45 degrees or more=90

    fixed DisplayAngle = -XCSoarInterface::Basic().TrackBearing;
    // or use .Heading? (no, because heading is not reliable)
    const FastRotation r(DisplayAngle);
    FastRotation::Pair p = r.Rotate(x, y);
    x = p.first;
    y = p.second;

    double scale = RangeScale(d);

    // Calculate screen coordinates
    POINT sc;
    sc.x = radar_mid.x + iround(x * scale);
    sc.y = radar_mid.y + iround(y * scale);

    // Set the arrow color depending on alarm level
    switch (traffic.AlarmLevel) {
    case 1:
      canvas.hollow_brush();
      canvas.select(hpWarning);
      canvas.circle(sc.x, sc.y, Layout::FastScale(16));
      canvas.select(hbWarning);
      break;
    case 2:
    case 3:
      canvas.hollow_brush();
      canvas.select(hpAlarm);
      canvas.circle(sc.x, sc.y, Layout::FastScale(16));
      canvas.circle(sc.x, sc.y, Layout::FastScale(22));
      canvas.select(hbAlarm);
      break;
    case 0:
    case 4:
      canvas.select(hbStandard);
      canvas.select(hpStandard);
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
    PolygonRotateShift(Arrow, 5, sc.x, sc.y,
                       traffic.TrackBearing + DisplayAngle);

    // Draw the polygon
    canvas.polygon(Arrow, 5);

    // Paint selection circle
    canvas.hollow_brush();
    canvas.select(hpSelection);
    if (static_cast<unsigned>(selection) == i)
      canvas.circle(sc.x, sc.y, Layout::FastScale(19));
  }
}

static void
PaintRadarPlane(Canvas &canvas) {
  static Pen hpPlane(IBLSCALE(2), Color::GRAY);
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

static void
PaintRadarBackground(Canvas &canvas) {
  static Pen hpGray(Layout::FastScale(1), Color::GRAY);
  canvas.select(wdf->GetBackBrush());
  canvas.select(hpGray);
  canvas.circle(radar_mid.x, radar_mid.y, radar_size.cx * 0.5);
  canvas.circle(radar_mid.x, radar_mid.y, radar_size.cx * 0.25);
}

static void
OnRadarPaint(WindowControl *Sender, Canvas &canvas)
{
  (void)zoom;
  PaintRadarBackground(canvas);
  PaintRadarPlane(canvas);
  PaintRadarTraffic(canvas);
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnTimerNotify),
  DeclareCallBackEntry(NULL)
};

void
dlgFlarmTrafficShowModal(void)
{
  if (Layout::landscape)
    wf = dlgLoadFromXML(CallBackTable, _T("dlgFlarmTraffic_L.xml"),
        XCSoarInterface::main_window, _T("IDR_XML_FLARMTRAFFIC_L"));
  else
    wf = dlgLoadFromXML(CallBackTable, _T("dlgFlarmTraffic.xml"),
        XCSoarInterface::main_window, _T("IDR_XML_FLARMTRAFFIC"));

  if (!wf)
    return;

  wdf = ((WndOwnerDrawFrame *)wf->FindByName(_T("frmRadar")));
  wdf->SetOnPaintNotify(OnRadarPaint);

  int size = min(wdf->get_height(), wdf->get_width());
  radar_size.cx = size - Layout::FastScale(20);
  radar_size.cy = size - Layout::FastScale(20);
  radar_mid.x = wdf->get_width() / 2;
  radar_mid.y = wdf->get_height() / 2;

  wf->SetKeyDownNotify(FormKeyDown);
  wf->SetTimerNotify(OnTimerNotify);

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

  Update();

  wf->ShowModal();

  delete wf;
  wf = NULL;
}
