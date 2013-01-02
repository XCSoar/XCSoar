/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Math/Screen.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Profile/Profile.hpp"
#include "Compiler.h"
#include "FLARM/Friends.hpp"
#include "Look/FlarmTrafficLook.hpp"
#include "Gauge/FlarmTrafficWindow.hpp"
#include "Language/Language.hpp"
#include "GestureManager.hpp"
#include "Formatter/UserUnits.hpp"
#include "Units/Units.hpp"
#include "Renderer/UnitSymbolRenderer.hpp"
#include "Interface.hpp"

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
     task_direction(Angle::Degrees(fixed_minus_one)) {}

protected:
  void CalcAutoZoom();

public:
  void Update(Angle new_direction, const TrafficList &new_data,
              const TeamCodeSettings &new_settings);
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
  void PaintClimbRate(Canvas &canvas, PixelRect rc, fixed climb_rate) const;
  void PaintDistance(Canvas &canvas, PixelRect rc, fixed distance) const;
  void PaintRelativeAltitude(Canvas &canvas, PixelRect rc,
                             fixed relative_altitude) const;
  void PaintID(Canvas &canvas, PixelRect rc, const FlarmTraffic &traffic) const;
  void PaintTaskDirection(Canvas &canvas) const;

protected:
  virtual void OnCreate();
  virtual void OnPaint(Canvas &canvas);
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys);
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y);
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y);
  bool OnMouseGesture(const TCHAR* gesture);
};

static WndForm *wf = NULL;
static FlarmTrafficControl *wdf;
static CheckBox *auto_zoom, *north_up;

void
FlarmTrafficControl::OnCreate()
{
  FlarmTrafficWindow::OnCreate();

  const TrafficSettings &settings = CommonInterface::GetUISettings().traffic;

  Profile::GetEnum(ProfileKeys::FlarmSideData, side_display_type);
  enable_auto_zoom = settings.auto_zoom;
  enable_north_up = settings.north_up;
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
  TrafficSettings &settings = CommonInterface::SetUISettings().traffic;
  settings.north_up = enable_north_up = enabled;
  Profile::Set(ProfileKeys::FlarmNorthUp, enabled);
  north_up->SetState(enabled);
}

void
FlarmTrafficControl::SetAutoZoom(bool enabled)
{
  TrafficSettings &settings = CommonInterface::SetUISettings().traffic;
  settings.auto_zoom = enable_auto_zoom = enabled;
  Profile::Set(ProfileKeys::FlarmAutoZoom, enabled);
  auto_zoom->SetState(enabled);
}

void
FlarmTrafficControl::CalcAutoZoom()
{
  bool warning_mode = WarningMode();
  RoughDistance zoom_dist = fixed_zero;

  for (auto it = data.list.begin(), end = data.list.end();
      it != end; ++it) {
    if (warning_mode && !it->HasAlarm())
      continue;

    zoom_dist = max(it->distance, zoom_dist);
  }

  fixed zoom_dist2 = zoom_dist;
  for (unsigned i = 0; i <= 4; i++) {
    if (i == 4 || fixed(GetZoomDistance(i)) >= zoom_dist2) {
      SetZoom(i);
      break;
    }
  }
}

void
FlarmTrafficControl::Update(Angle new_direction, const TrafficList &new_data,
                            const TeamCodeSettings &new_settings)
{
  FlarmTrafficWindow::Update(new_direction, new_data, new_settings);

  if (enable_auto_zoom || WarningMode())
    CalcAutoZoom();
}

void
FlarmTrafficControl::UpdateTaskDirection(bool show_task_direction, Angle bearing)
{
  if (!show_task_direction)
    task_direction = Angle::Degrees(fixed_minus_one);
  else
    task_direction = bearing.AsBearing();
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
  if (negative(task_direction.Degrees()))
    return;

  canvas.Select(look.radar_pen);
  canvas.SelectHollowBrush();

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
                                       Angle::Zero() : heading));

  // Draw the arrow
  canvas.DrawPolygon(triangle, 4);
}

void
FlarmTrafficControl::PaintClimbRate(Canvas &canvas, PixelRect rc,
                                    fixed climb_rate) const
{
  // Paint label
  canvas.Select(look.info_labels_font);
  PixelScalar label_width = canvas.CalcTextSize(_("Vario")).cx;
  canvas.text(rc.right - label_width, rc.top, _("Vario"));

  // Format climb rate
  TCHAR buffer[20];
  Unit unit = Units::GetUserVerticalSpeedUnit();
  FormatUserVerticalSpeed(climb_rate, buffer, false);

  // Calculate unit size
  canvas.Select(look.info_units_font);
  UPixelScalar unit_width = UnitSymbolRenderer::GetSize(canvas, unit).cx;
  UPixelScalar unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.info_units_font, unit);

  UPixelScalar space_width = unit_width / 3;

  // Calculate value size
  canvas.Select(look.info_values_font);
  UPixelScalar value_height = look.info_values_font.GetAscentHeight();
  UPixelScalar value_width = canvas.CalcTextSize(buffer).cx;

  // Calculate positions
  PixelScalar max_height = max(unit_height, value_height);
  PixelScalar y = rc.top + look.info_units_font.GetHeight() + max_height;

  // Paint value
  canvas.text(rc.right - unit_width - space_width - value_width, y - value_height, buffer);

  // Paint unit
  canvas.Select(look.info_units_font);
  UnitSymbolRenderer::Draw(
      canvas, { PixelScalar(rc.right - unit_width), PixelScalar(y - unit_height) }, unit, look.unit_fraction_pen);
}

void
FlarmTrafficControl::PaintDistance(Canvas &canvas, PixelRect rc,
                                    fixed distance) const
{
  // Format distance
  TCHAR buffer[20];
  Unit unit = FormatUserDistanceSmart(distance, buffer, false, fixed(1000));

  // Calculate unit size
  canvas.Select(look.info_units_font);
  UPixelScalar unit_width = UnitSymbolRenderer::GetSize(canvas, unit).cx;
  UPixelScalar unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.info_units_font, unit);

  UPixelScalar space_width = unit_width / 3;

  // Calculate value size
  canvas.Select(look.info_values_font);
  UPixelScalar value_height = look.info_values_font.GetAscentHeight();
  UPixelScalar value_width = canvas.CalcTextSize(buffer).cx;

  // Calculate positions
  PixelScalar max_height = max(unit_height, value_height);

  // Paint value
  canvas.text(rc.left, rc.bottom - value_height, buffer);

  // Paint unit
  canvas.Select(look.info_units_font);
  UnitSymbolRenderer::Draw(
      canvas, { PixelScalar(rc.left + value_width + space_width), PixelScalar(rc.bottom - unit_height) }, unit, look.unit_fraction_pen);


  // Paint label
  canvas.Select(look.info_labels_font);
  canvas.text(rc.left, rc.bottom - max_height - look.info_labels_font.GetHeight(), _("Distance"));
}

void
FlarmTrafficControl::PaintRelativeAltitude(Canvas &canvas, PixelRect rc,
                                           fixed relative_altitude) const
{
  // Format relative altitude
  TCHAR buffer[20];
  Unit unit = Units::GetUserAltitudeUnit();
  FormatRelativeUserAltitude(relative_altitude, buffer, false);

  // Calculate unit size
  canvas.Select(look.info_units_font);
  UPixelScalar unit_width = UnitSymbolRenderer::GetSize(canvas, unit).cx;
  UPixelScalar unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.info_units_font, unit);

  UPixelScalar space_width = unit_width / 3;

  // Calculate value size
  canvas.Select(look.info_values_font);
  UPixelScalar value_height = look.info_values_font.GetAscentHeight();
  UPixelScalar value_width = canvas.CalcTextSize(buffer).cx;

  // Calculate positions
  PixelScalar max_height = max(unit_height, value_height);

  // Paint value
  canvas.text(rc.right - unit_width - space_width - value_width, rc.bottom - value_height, buffer);

  // Paint unit
  canvas.Select(look.info_units_font);
  UnitSymbolRenderer::Draw(
      canvas, { PixelScalar(rc.right - unit_width), PixelScalar(rc.bottom - unit_height) }, unit, look.unit_fraction_pen);


  // Paint label
  canvas.Select(look.info_labels_font);
  PixelScalar label_width = canvas.CalcTextSize(_("Rel. Alt.")).cx;
  canvas.text(rc.right - label_width, rc.bottom - max_height - look.info_labels_font.GetHeight(), _("Rel. Alt."));
}

void
FlarmTrafficControl::PaintID(Canvas &canvas, PixelRect rc,
                             const FlarmTraffic &traffic) const
{
  TCHAR buffer[20];

  unsigned font_size;
  if (traffic.HasName()) {
    canvas.Select(look.call_sign_font);
    font_size = look.call_sign_font.GetHeight();

    _tcscpy(buffer, traffic.name);
  } else {
    canvas.Select(look.info_labels_font);
    font_size = look.info_labels_font.GetHeight();

    traffic.id.Format(buffer);
  }

  if (!WarningMode()) {
    // Team color dot
    FlarmFriends::Color team_color = FlarmFriends::GetFriendColor(traffic.id);

    // If no color found but target is teammate
    if (team_color == FlarmFriends::Color::NONE &&
        settings.team_flarm_tracking &&
        traffic.id == settings.team_flarm_id)
      // .. use green color
      team_color = FlarmFriends::Color::GREEN;

    // If team color found -> draw a colored circle in front of the name
    if (team_color != FlarmFriends::Color::NONE) {
      switch (team_color) {
      case FlarmFriends::Color::GREEN:
        canvas.Select(look.team_brush_green);
        break;
      case FlarmFriends::Color::BLUE:
        canvas.Select(look.team_brush_blue);
        break;
      case FlarmFriends::Color::YELLOW:
        canvas.Select(look.team_brush_yellow);
        break;
      case FlarmFriends::Color::MAGENTA:
        canvas.Select(look.team_brush_magenta);
        break;
      default:
        break;
      }

      canvas.SelectNullPen();
      canvas.DrawCircle(rc.left + Layout::FastScale(7), rc.top + (font_size / 2),
                    Layout::FastScale(7));

      rc.left += Layout::FastScale(16);
    }
  }

  canvas.text(rc.left, rc.top, buffer);
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
  FlarmTraffic traffic = data.list[WarningMode() ? warning : selection];
  assert(traffic.IsDefined());

  PixelRect rc;
  rc.left = padding;
  rc.top = padding;
  rc.right = canvas.get_width() - padding;
  rc.bottom = canvas.get_height() - padding;

  // Set the text color and background
  switch (traffic.alarm_level) {
  case FlarmTraffic::AlarmType::LOW:
  case FlarmTraffic::AlarmType::INFO_ALERT:
    canvas.SetTextColor(look.warning_color);
    break;
  case FlarmTraffic::AlarmType::IMPORTANT:
  case FlarmTraffic::AlarmType::URGENT:
    canvas.SetTextColor(look.alarm_color);
    break;
  case FlarmTraffic::AlarmType::NONE:
    canvas.SetTextColor(look.default_color);
    break;
  }
  canvas.SetBackgroundTransparent();

  // Climb Rate
  if (!WarningMode() && traffic.climb_rate_avg30s_available)
    PaintClimbRate(canvas, rc, traffic.climb_rate_avg30s);

  // Distance
  PaintDistance(canvas, rc, traffic.distance);

  // Relative Height
  PaintRelativeAltitude(canvas, rc, traffic.relative_altitude);

  // ID / Name
  if (!traffic.HasAlarm())
    canvas.SetTextColor(look.selection_color);

  PaintID(canvas, rc, traffic);
}

void
FlarmTrafficControl::OnPaint(Canvas &canvas)
{
  canvas.ClearWhite();

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
  const FlarmTraffic *traffic = wdf->GetTarget();
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
  if (wdf->side_display_type == FlarmTrafficWindow::SIDE_INFO_VARIO)
    wdf->side_display_type = FlarmTrafficWindow::SIDE_INFO_RELATIVE_ALTITUDE;
  else
    wdf->side_display_type = FlarmTrafficWindow::SIDE_INFO_VARIO;

  Profile::SetEnum(ProfileKeys::FlarmSideData, wdf->side_display_type);
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
  wdf->SetAutoZoom(control.GetState());
}

static void
OnNorthUp(CheckBoxControl &control)
{
  wdf->SetNorthUp(control.GetState());
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
    if (!HasPointer())
      break;

    wdf->ZoomIn();
    return true;
  case VK_DOWN:
    if (!HasPointer())
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
  SetFormControlEnabled(*wf, _T("cmdSwitchData"), enabled);
  SetFormControlEnabled(*wf, _T("cmdDetails"), enabled);
  SetFormControlEnabled(*wf, _T("cmdZoomIn"), enabled);
  SetFormControlEnabled(*wf, _T("cmdZoomOut"), enabled);
  SetFormControlEnabled(*wf, _T("cmdPrev"), enabled);
  SetFormControlEnabled(*wf, _T("cmdNext"), enabled);
}

static void
Update()
{
  if (XCSoarInterface::GetUISettings().traffic.auto_close_dialog &&
      XCSoarInterface::Basic().flarm.traffic.IsEmpty())
    wf->SetModalResult(mrOK);

  wdf->Update(XCSoarInterface::Basic().track,
              XCSoarInterface::Basic().flarm.traffic,
              CommonInterface::GetComputerSettings().team_code);

  wdf->UpdateTaskDirection(XCSoarInterface::Calculated().task_stats.task_valid &&
                           XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining.IsOk(),
                           XCSoarInterface::Calculated().task_stats.
                           current_leg.solution_remaining.cruise_track_bearing);

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
FlarmTrafficControl::OnMouseMove(PixelScalar x, PixelScalar y,
                                   gcc_unused unsigned keys)
{
  gestures.Update(x, y);

  return true;
}

bool
FlarmTrafficControl::OnMouseDown(PixelScalar x, PixelScalar y)
{
  gestures.Start(x, y, Layout::Scale(20));

  return true;
}

bool
FlarmTrafficControl::OnMouseUp(PixelScalar x, PixelScalar y)
{
  const TCHAR *gesture = gestures.Finish();
  if (gesture && OnMouseGesture(gesture))
    return true;

  if (!WarningMode())
    SelectNearTarget(x, y, Layout::Scale(15));

  return true;
}

bool
FlarmTrafficControl::OnMouseGesture(const TCHAR* gesture)
{
  if (StringIsEqual(gesture, _T("U"))) {
    ZoomIn();
    return true;
  }
  if (StringIsEqual(gesture, _T("D"))) {
    ZoomOut();
    return true;
  }
  if (StringIsEqual(gesture, _T("L"))) {
    PrevTarget();
    return true;
  }
  if (StringIsEqual(gesture, _T("R"))) {
    NextTarget();
    return true;
  }
  if (StringIsEqual(gesture, _T("UD"))) {
    SetAutoZoom(true);
    return true;
  }
  if (StringIsEqual(gesture, _T("DR"))) {
    OpenDetails();
    return true;
  }
  if (StringIsEqual(gesture, _T("RL"))) {
    SwitchData();
    return true;
  }

  return false;
}

static Window *
OnCreateFlarmTrafficControl(ContainerWindow &parent,
                            PixelScalar left, PixelScalar top,
                            UPixelScalar width, UPixelScalar height,
                            const WindowStyle style)
{
  const Look &look = UIGlobals::GetLook();
  wdf = new FlarmTrafficControl(look.flarm_dialog);
  wdf->set(parent, left, top, width, height, style);

  return wdf;
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCreateFlarmTrafficControl),
  DeclareCallBackEntry(OnDetailsClicked),
  DeclareCallBackEntry(OnZoomInClicked),
  DeclareCallBackEntry(OnZoomOutClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnSwitchDataClicked),
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
  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  Layout::landscape ? _T("IDR_XML_FLARMTRAFFIC_L") :
                                      _T("IDR_XML_FLARMTRAFFIC"));
  assert(wf != NULL);

  // Set dialog events
  wf->SetKeyDownNotify(FormKeyDown);
  wf->SetTimerNotify(OnTimerNotify);

  // Update Radar and Selection for the first time
  Update();

  // Get the last chosen Side Data configuration
  auto_zoom = (CheckBox *)wf->FindByName(_T("AutoZoom"));
  auto_zoom->SetState(wdf->GetAutoZoom());

  north_up = (CheckBox *)wf->FindByName(_T("NorthUp"));
  north_up->SetState(wdf->GetNorthUp());

  // Show the dialog
  wf->ShowModal();

  // After dialog closed -> delete it
  delete wf;
  wf = NULL;
}
