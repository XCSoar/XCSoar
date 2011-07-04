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

/**
 * @file
 * The FLARM Traffic dialog displaying a radar screen with the moving
 * FLARM targets in track up orientation. The target can be selected and basic
 * information is given on the selected target. When a warning/alarm is present
 * the target with the highest alarm level is automatically selected and
 * highlighted in orange or red (depending on the level)
 */

#include "Dialogs/Traffic.hpp"
#include "Dialogs/Internal.hpp"
#include "Screen.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Form/CheckBox.hpp"
#include "MainWindow.hpp"
#include "Profile/Profile.hpp"
#include "Compiler.h"
#include "FLARM/Friends.hpp"
#include "Gauge/FlarmTrafficLook.hpp"
#include "Gauge/FlarmTrafficWindow.hpp"
#include "Language/Language.hpp"
#include "GestureManager.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Screen/Graphics.hpp"

/**
 * A Window which renders FLARM traffic, with user interaction.
 */
class FlarmTrafficControl : public FlarmTrafficWindow {
protected:
  bool enable_auto_zoom;
  unsigned zoom;
  Angle task_direction;
  GestureManager gestures;

public:
  FlarmTrafficControl(const FlarmTrafficLook &look)
    :FlarmTrafficWindow(look, Layout::Scale(10)),
     enable_auto_zoom(true),
     zoom(2),
     task_direction(Angle::degrees(fixed_minus_one)) {}

protected:
  void CalcAutoZoom();

public:
  void Update(Angle new_direction, const FLARM_STATE &new_data,
              const SETTINGS_TEAMCODE &new_settings);
  void UpdateTaskDirection(bool show_task_direction, Angle bearing);

  bool GetNorthUp() const {
    return enable_north_up;
  }

  void SetNorthUp(bool enabled);

  bool GetAutoZoom() const {
    return enable_auto_zoom;
  }

  static unsigned GetZoomDistance(unsigned zoom);

  void SetZoom(unsigned _zoom) {
    zoom = _zoom;
    SetDistance(fixed(GetZoomDistance(_zoom)));
  }

  void SetAutoZoom(bool enabled);

  void ZoomOut();
  void ZoomIn();

protected:
  void PaintTrafficInfo(Canvas &canvas) const;
  void PaintTaskDirection(Canvas &canvas) const;

protected:
  virtual bool on_create();
  virtual void on_paint(Canvas &canvas);
  virtual bool on_mouse_move(int x, int y, unsigned keys);
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);
  bool on_mouse_gesture(const TCHAR* gesture);
};

static WndForm *wf = NULL;
static FlarmTrafficControl *wdf;
static CheckBox *auto_zoom, *north_up;

bool
FlarmTrafficControl::on_create()
{
  FlarmTrafficWindow::on_create();

  Profile::Get(szProfileFlarmSideData, side_display_type);
  Profile::Get(szProfileFlarmAutoZoom, enable_auto_zoom);
  Profile::Get(szProfileFlarmNorthUp, enable_north_up);

  return true;
}

unsigned
FlarmTrafficControl::GetZoomDistance(unsigned zoom)
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
FlarmTrafficControl::SetNorthUp(bool enabled)
{
  enable_north_up = enabled;
  Profile::Set(szProfileFlarmNorthUp, enabled);
  north_up->set_checked(enabled);
}

void
FlarmTrafficControl::SetAutoZoom(bool enabled)
{
  enable_auto_zoom = enabled;
  Profile::Set(szProfileFlarmAutoZoom, enabled);
  auto_zoom->set_checked(enabled);
}

void
FlarmTrafficControl::CalcAutoZoom()
{
  bool warning_mode = WarningMode();
  fixed zoom_dist = fixed_zero;

  for (unsigned i = 0; i < data.traffic.size(); i++) {
    if (warning_mode && !data.traffic[i].HasAlarm())
      continue;

    zoom_dist = max(data.traffic[i].distance, zoom_dist);
  }

  for (unsigned i = 0; i <= 4; i++) {
    if (i == 4 || fixed(GetZoomDistance(i)) >= zoom_dist) {
      SetZoom(i);
      break;
    }
  }
}

void
FlarmTrafficControl::Update(Angle new_direction, const FLARM_STATE &new_data,
                            const SETTINGS_TEAMCODE &new_settings)
{
  FlarmTrafficWindow::Update(new_direction, new_data, new_settings);

  if (enable_auto_zoom || WarningMode())
    CalcAutoZoom();
}

void
FlarmTrafficControl::UpdateTaskDirection(bool show_task_direction, Angle bearing)
{
  if (!show_task_direction)
    task_direction = Angle::degrees(fixed_minus_one);
  else
    task_direction = bearing.as_bearing();
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
    SetZoom(zoom + 1);

  SetAutoZoom(false);
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
    SetZoom(zoom - 1);

  SetAutoZoom(false);
}

/**
 * Paints an arrow into the direction of the current task leg
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficControl::PaintTaskDirection(Canvas &canvas) const
{
  if (negative(task_direction.value_degrees()))
    return;

  canvas.select(look.hpRadar);
  canvas.hollow_brush();

  RasterPoint triangle[4];
  triangle[0].x = 0;
  triangle[0].y = -radius / Layout::FastScale(1) + 15;
  triangle[1].x = 7;
  triangle[1].y = triangle[0].y + 30;
  triangle[2].x = -triangle[1].x;
  triangle[2].y = triangle[1].y;
  triangle[3].x = triangle[0].x;
  triangle[3].y = triangle[0].y;

  PolygonRotateShift(triangle, 4, radar_mid.x, radar_mid.y,
                     task_direction - (enable_north_up ?
                                       Angle::native(fixed_zero) : heading));

  // Draw the arrow
  canvas.polygon(triangle, 4);
}

/**
 * Paints the basic info for the selected target on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficControl::PaintTrafficInfo(Canvas &canvas) const
{
  // Don't paint numbers if no plane selected
  if (selection == -1 && !WarningMode())
    return;

  // Shortcut to the selected traffic
  FLARM_TRAFFIC traffic = data.traffic[WarningMode() ? warning : selection];
  assert(traffic.IsDefined());

  // Temporary string
  TCHAR tmp[20];
  // Temporary string size
  PixelSize sz;

  PixelRect rc;
  rc.left = padding;
  rc.top = padding;
  rc.right = canvas.get_width() - padding;
  rc.bottom = canvas.get_height() - padding;

  // Set the text color and background
  switch (traffic.alarm_level) {
  case 1:
    canvas.set_text_color(look.hcWarning);
    break;
  case 2:
  case 3:
    canvas.set_text_color(look.hcAlarm);
    break;
  case 4:
  case 0:
  default:
    canvas.set_text_color(look.hcStandard);
    break;
  }
  canvas.background_transparent();

  // Climb Rate
  if (!WarningMode() && traffic.climb_rate_avg30s_available) {
    Units::FormatUserVSpeed(traffic.climb_rate_avg30s, tmp, 20);
    canvas.select(look.hfInfoValues);
    sz = canvas.text_size(tmp);
    canvas.text(rc.right - sz.cx, rc.top + look.hfInfoLabels.get_height(), tmp);

    canvas.select(look.hfInfoLabels);
    sz = canvas.text_size(_("Vario"));
    canvas.text(rc.right - sz.cx, rc.top, _("Vario"));
  }

  // Distance
  Units::FormatUserDistance(traffic.distance, tmp, 20);
  canvas.select(look.hfInfoValues);
  sz = canvas.text_size(tmp);
  canvas.text(rc.left, rc.bottom - sz.cy, tmp);

  canvas.select(look.hfInfoLabels);
  canvas.text(rc.left,
              rc.bottom - look.hfInfoValues.get_height() - look.hfInfoLabels.get_height(),
              _("Distance"));

  // Relative Height
  Units::FormatUserArrival(traffic.relative_altitude, tmp, 20);
  canvas.select(look.hfInfoValues);
  sz = canvas.text_size(tmp);
  canvas.text(rc.right - sz.cx, rc.bottom - sz.cy, tmp);

  canvas.select(look.hfInfoLabels);
  sz = canvas.text_size(_("Rel. Alt."));
  canvas.text(rc.right - sz.cx,
              rc.bottom - look.hfInfoValues.get_height() - look.hfInfoLabels.get_height(),
              _("Rel. Alt."));

  // ID / Name
  unsigned font_size;
  if (traffic.HasName()) {
    canvas.select(look.hfCallSign);
    font_size = look.hfCallSign.get_height();

    if (!traffic.HasAlarm())
      canvas.set_text_color(look.hcSelection);

    _tcscpy(tmp, traffic.name);
  } else {
    font_size = look.hfInfoLabels.get_height();
    traffic.id.format(tmp);
  }

  if (!WarningMode()) {
    // Team color dot
    FlarmFriends::Color team_color = FlarmFriends::GetFriendColor(traffic.id);

    // If no color found but target is teammate
    if (team_color == FlarmFriends::NONE &&
        settings.TeamFlarmTracking &&
        traffic.id == settings.TeamFlarmIdTarget)
      // .. use yellow color
      team_color = FlarmFriends::YELLOW;

    // If team color found -> draw a colored circle around the target
    if (team_color != FlarmFriends::NONE) {
      switch (team_color) {
      case FlarmFriends::GREEN:
        canvas.select(look.hbTeamGreen);
        break;
      case FlarmFriends::BLUE:
        canvas.select(look.hbTeamBlue);
        break;
      case FlarmFriends::YELLOW:
        canvas.select(look.hbTeamYellow);
        break;
      case FlarmFriends::MAGENTA:
        canvas.select(look.hbTeamMagenta);
        break;
      default:
        break;
      }

      canvas.null_pen();
      canvas.circle(rc.left + Layout::FastScale(7), rc.top + (font_size / 2),
                    Layout::FastScale(7));

      rc.left += Layout::FastScale(16);
    }
  }

  canvas.text(rc.left, rc.top, tmp);
}

void
FlarmTrafficControl::on_paint(Canvas &canvas)
{
  canvas.clear_white();

  PaintTaskDirection(canvas);
  FlarmTrafficWindow::Paint(canvas);
  PaintTrafficInfo(canvas);
}

static void
OpenDetails()
{
  // If warning is displayed -> prevent from opening details dialog
  if (wdf->WarningMode())
    return;

  // Don't open the details dialog if no plane selected
  const FLARM_TRAFFIC *traffic = wdf->GetTarget();
  if (traffic == NULL)
    return;

  // Show the details dialog
  dlgFlarmTrafficDetailsShowModal(traffic->id);
}

/**
 * This event handler is called when the "Details" button is pressed
 */
static void
OnDetailsClicked(gcc_unused WndButton &button)
{
  OpenDetails();
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
  wdf->PrevTarget();
}

/**
 * This event handler is called when the "Next (>)" button is pressed
 */
static void
OnNextClicked(gcc_unused WndButton &button)
{
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

static void
SwitchData()
{
  wdf->side_display_type++;
  if (wdf->side_display_type > 2)
    wdf->side_display_type = 1;

  Profile::Set(szProfileFlarmSideData, wdf->side_display_type);
}

/**
 * This event handler is called when the "Avg/Alt" button is pressed
 */
static void
OnSwitchDataClicked(gcc_unused WndButton &button)
{
  SwitchData();
}

static void
OnAutoZoom(CheckBoxControl &control)
{
  wdf->SetAutoZoom(control.get_checked());
}

static void
OnNorthUp(CheckBoxControl &control)
{
  wdf->SetNorthUp(control.get_checked());
}

/**
 * This event handler is called when a key is pressed
 * @param key_code The key code of the pressed key
 * @return True if the event was handled, False otherwise
 */
static bool
FormKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
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
#ifdef GNAV
  case '6':
#endif
    wdf->PrevTarget();
    return true;
  case VK_RIGHT:
#ifdef GNAV
  case '7':
#endif
    wdf->NextTarget();
    return true;
  }

  return false;
}

static void
SetButtonsEnabled(bool enabled)
{
  ((WndButton *)wf->FindByName(_T("cmdSwitchData")))->set_enabled(enabled);
  ((WndButton *)wf->FindByName(_T("cmdDetails")))->set_enabled(enabled);
  ((WndButton *)wf->FindByName(_T("cmdZoomIn")))->set_enabled(enabled);
  ((WndButton *)wf->FindByName(_T("cmdZoomOut")))->set_enabled(enabled);
  ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_enabled(enabled);
  ((WndButton *)wf->FindByName(_T("cmdNext")))->set_enabled(enabled);
}

static void
Update()
{
  if (XCSoarInterface::SettingsMap().AutoCloseFlarmDialog &&
      (!XCSoarInterface::Basic().flarm.available ||
       XCSoarInterface::Basic().flarm.GetActiveTrafficCount() == 0))
    wf->SetModalResult(mrOK);

  wdf->Update(XCSoarInterface::Basic().track,
              XCSoarInterface::Basic().flarm,
              XCSoarInterface::SettingsComputer());

  wdf->UpdateTaskDirection(XCSoarInterface::Calculated().task_stats.task_valid &&
                           XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining.defined(),
                           XCSoarInterface::Calculated().task_stats.
                           current_leg.solution_remaining.CruiseTrackBearing);

  SetButtonsEnabled(!wdf->WarningMode());
}

/**
 * This event handler is called when the timer is activated and triggers the
 * repainting of the radar
 */
static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  Update();
}

bool
FlarmTrafficControl::on_mouse_move(int x, int y, gcc_unused unsigned keys)
{
  gestures.Update(x, y);

  return true;
}

bool
FlarmTrafficControl::on_mouse_down(int x, int y)
{
  gestures.Start(x, y, Layout::Scale(20));

  return true;
}

bool
FlarmTrafficControl::on_mouse_up(int x, int y)
{
  const TCHAR *gesture = gestures.Finish();
  if (gesture && on_mouse_gesture(gesture))
    return true;

  if (!WarningMode())
    SelectNearTarget(x, y, Layout::Scale(15));

  return true;
}

bool
FlarmTrafficControl::on_mouse_gesture(const TCHAR* gesture)
{
  if (_tcscmp(gesture, _T("U")) == 0) {
    ZoomIn();
    return true;
  }
  if (_tcscmp(gesture, _T("D")) == 0) {
    ZoomOut();
    return true;
  }
  if (_tcscmp(gesture, _T("L")) == 0) {
    PrevTarget();
    return true;
  }
  if (_tcscmp(gesture, _T("R")) == 0) {
    NextTarget();
    return true;
  }
  if (_tcscmp(gesture, _T("UD")) == 0) {
    SetAutoZoom(true);
    return true;
  }
  if (_tcscmp(gesture, _T("DR")) == 0) {
    OpenDetails();
    return true;
  }
  if (_tcscmp(gesture, _T("RL")) == 0) {
    SwitchData();
    return true;
  }

  return false;
}

static Window *
OnCreateFlarmTrafficControl(ContainerWindow &parent, int left, int top,
                            unsigned width, unsigned height,
                            const WindowStyle style)
{
  wdf = new FlarmTrafficControl(Graphics::flarm_dialog);
  wdf->set(parent, left, top, width, height, style);

  return wdf;
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCreateFlarmTrafficControl),
  DeclareCallBackEntry(OnTimerNotify),
  DeclareCallBackEntry(OnAutoZoom),
  DeclareCallBackEntry(OnNorthUp),
  DeclareCallBackEntry(NULL)
};

/**
 * The function opens the FLARM Traffic dialog
 */
void
dlgFlarmTrafficShowModal()
{
  if (wf)
    return;

  // Load dialog from XML
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_FLARMTRAFFIC_L") :
                                      _T("IDR_XML_FLARMTRAFFIC"));
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

  // Update Radar and Selection for the first time
  Update();

  // Get the last chosen Side Data configuration
  auto_zoom = (CheckBox *)wf->FindByName(_T("AutoZoom"));
  auto_zoom->set_checked(wdf->GetAutoZoom());

  north_up = (CheckBox *)wf->FindByName(_T("NorthUp"));
  north_up->set_checked(wdf->GetNorthUp());

  // Show the dialog
  wf->ShowModal();

  // After dialog closed -> delete it
  delete wf;
  wf = NULL;
}
