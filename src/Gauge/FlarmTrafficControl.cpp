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

#include "FlarmTrafficControl.hpp"
#include "Dialogs/Traffic/TrafficDialogs.hpp"
#include "Math/Screen.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Form/SymbolButton.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Profile/Profile.hpp"
#include "Compiler.h"
#include "FLARM/Friends.hpp"
#include "Look/FlarmTrafficLook.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "Renderer/UnitSymbolRenderer.hpp"
#include "Input/InputEvents.hpp"
#include "Interface.hpp"
#include "Asset.hpp"

FlarmTrafficControl::FlarmTrafficControl(const FlarmTrafficLook &look)
  :FlarmTrafficWindow(look, Layout::Scale(10),
                      Layout::GetMinimumControlHeight() + Layout::Scale(2)),
   enable_auto_zoom(true), dragging(false),
   zoom(2),
   task_direction(Angle::Degrees(-1)),
   mouse_double_function(nullptr),
   auto_zoom_change_function(nullptr),
   north_up_change_function(nullptr)
{}

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

  if (north_up_change_function != nullptr)
    north_up_change_function(enabled);
}

void
FlarmTrafficControl::SetAutoZoom(bool enabled)
{
  TrafficSettings &settings = CommonInterface::SetUISettings().traffic;
  settings.auto_zoom = enable_auto_zoom = enabled;
  Profile::Set(ProfileKeys::FlarmAutoZoom, enabled);

  if (auto_zoom_change_function != nullptr)
    auto_zoom_change_function(enabled);
}

void
FlarmTrafficControl::CalcAutoZoom()
{
  bool warning_mode = WarningMode();
  RoughDistance zoom_dist = fixed(0);

  for (auto it = data.list.begin(), end = data.list.end();
      it != end; ++it) {
    if (warning_mode && !it->HasAlarm())
      continue;

    zoom_dist = std::max(it->distance, zoom_dist);
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
    task_direction = Angle::Degrees(-1);
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
  const unsigned label_width = canvas.CalcTextSize(_("Vario")).cx;
  canvas.DrawText(rc.right - label_width, rc.top, _("Vario"));

  // Format climb rate
  TCHAR buffer[20];
  Unit unit = Units::GetUserVerticalSpeedUnit();
  FormatUserVerticalSpeed(climb_rate, buffer, false);

  // Calculate unit size
  canvas.Select(look.info_units_font);
  const unsigned unit_width = UnitSymbolRenderer::GetSize(canvas, unit).cx;
  const unsigned unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.info_units_font, unit);

  UPixelScalar space_width = unit_width / 3;

  // Calculate value size
  canvas.Select(look.info_values_font);
  const unsigned value_height = look.info_values_font.GetAscentHeight();
  const unsigned value_width = canvas.CalcTextSize(buffer).cx;

  // Calculate positions
  const int max_height = std::max(unit_height, value_height);
  const int y = rc.top + look.info_units_font.GetHeight() + max_height;

  // Paint value
  canvas.DrawText(rc.right - unit_width - space_width - value_width,
                  y - value_height,
                  buffer);

  // Paint unit
  canvas.Select(look.info_units_font);
  UnitSymbolRenderer::Draw(canvas,
                           RasterPoint(rc.right - unit_width, y - unit_height),
                           unit, look.unit_fraction_pen);
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
  const unsigned unit_width = UnitSymbolRenderer::GetSize(canvas, unit).cx;
  const unsigned unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.info_units_font, unit);

  const unsigned space_width = unit_width / 3;

  // Calculate value size
  canvas.Select(look.info_values_font);
  const unsigned value_height = look.info_values_font.GetAscentHeight();
  const unsigned value_width = canvas.CalcTextSize(buffer).cx;

  // Calculate positions
  const unsigned max_height = std::max(unit_height, value_height);

  // Paint value
  canvas.DrawText(rc.left, rc.bottom - value_height, buffer);

  // Paint unit
  canvas.Select(look.info_units_font);
  UnitSymbolRenderer::Draw(canvas,
                           RasterPoint(rc.left + value_width + space_width,
                                       rc.bottom - unit_height),
                           unit, look.unit_fraction_pen);


  // Paint label
  canvas.Select(look.info_labels_font);
  canvas.DrawText(rc.left,
                  rc.bottom - max_height - look.info_labels_font.GetHeight(),
                  _("Distance"));
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
  const unsigned unit_width = UnitSymbolRenderer::GetSize(canvas, unit).cx;
  const unsigned unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.info_units_font, unit);

  const unsigned space_width = unit_width / 3;

  // Calculate value size
  canvas.Select(look.info_values_font);
  const unsigned value_height = look.info_values_font.GetAscentHeight();
  const unsigned value_width = canvas.CalcTextSize(buffer).cx;

  // Calculate positions
  const unsigned max_height = std::max(unit_height, value_height);

  // Paint value
  canvas.DrawText(rc.right - unit_width - space_width - value_width,
                  rc.bottom - value_height,
                  buffer);

  // Paint unit
  canvas.Select(look.info_units_font);
  UnitSymbolRenderer::Draw(canvas,
                           RasterPoint(rc.right - unit_width,
                                       rc.bottom - unit_height),
                           unit, look.unit_fraction_pen);


  // Paint label
  canvas.Select(look.info_labels_font);
  const unsigned label_width = canvas.CalcTextSize(_("Rel. Alt.")).cx;
  canvas.DrawText(rc.right - label_width,
                  rc.bottom - max_height - look.info_labels_font.GetHeight(),
                  _("Rel. Alt."));
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
    FlarmColor team_color = FlarmFriends::GetFriendColor(traffic.id);

    // If team color found -> draw a colored circle in front of the name
    if (team_color != FlarmColor::NONE) {
      switch (team_color) {
      case FlarmColor::GREEN:
        canvas.Select(look.team_brush_green);
        break;
      case FlarmColor::BLUE:
        canvas.Select(look.team_brush_blue);
        break;
      case FlarmColor::YELLOW:
        canvas.Select(look.team_brush_yellow);
        break;
      case FlarmColor::MAGENTA:
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

  canvas.DrawText(rc.left, rc.top, buffer);
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

  const unsigned padding = Layout::GetTextPadding();
  PixelRect rc;
  rc.left = padding;
  rc.top = padding;
  rc.right = canvas.GetWidth() - padding;
  rc.bottom = canvas.GetHeight() - padding;

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

void
FlarmTrafficControl::OpenDetails()
{
  // If warning is displayed -> prevent from opening details dialog
  if (WarningMode())
    return;

  // Don't open the details dialog if no plane selected
  const FlarmTraffic *traffic = GetTarget();
  if (traffic == NULL)
    return;

  // Show the details dialog
  dlgFlarmTrafficDetailsShowModal(traffic->id);
}

void
FlarmTrafficControl::SwitchData()
{
  if (side_display_type == FlarmTrafficWindow::SIDE_INFO_VARIO)
    side_display_type = FlarmTrafficWindow::SIDE_INFO_RELATIVE_ALTITUDE;
  else
    side_display_type = FlarmTrafficWindow::SIDE_INFO_VARIO;

  Profile::SetEnum(ProfileKeys::FlarmSideData, side_display_type);
}

bool
FlarmTrafficControl::OnMouseMove(PixelScalar x, PixelScalar y,
                                 gcc_unused unsigned keys)
{
  if (dragging)
    gestures.Update(x, y);

  return true;
}

bool
FlarmTrafficControl::OnMouseDown(PixelScalar x, PixelScalar y)
{
  if (!dragging) {
    dragging = true;
    SetCapture();
    gestures.Start(x, y, Layout::Scale(20));
  }

  return true;
}

bool
FlarmTrafficControl::OnMouseUp(PixelScalar x, PixelScalar y)
{
  if (dragging) {
    StopDragging();

    const TCHAR *gesture = gestures.Finish();
    if (gesture && OnMouseGesture(gesture))
      return true;
  }

  if (!WarningMode())
    SelectNearTarget(x, y, Layout::Scale(15));

  return true;
}

bool
FlarmTrafficControl::OnMouseDouble(PixelScalar x, PixelScalar y)
{
  StopDragging();

  if (mouse_double_function == nullptr)
    return false;

  return mouse_double_function(x, y);
}

void
FlarmTrafficControl::OnCancelMode()
{
  FlarmTrafficWindow::OnCancelMode();
  StopDragging();
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

  return InputEvents::processGesture(gesture);
}

bool
FlarmTrafficControl::OnKeyDown(unsigned key_code)
{
  switch (key_code) {
  case KEY_UP:
    if (!HasPointer())
      break;

    ZoomIn();
    return true;

  case KEY_DOWN:
    if (!HasPointer())
      break;

    ZoomOut();
    return true;

  case KEY_LEFT:
#ifdef GNAV
  case '6':
#endif
    PrevTarget();
    return true;

  case KEY_RIGHT:
#ifdef GNAV
  case '7':
#endif
    NextTarget();
    return true;
  }

  return FlarmTrafficWindow::OnKeyDown(key_code) ||
    InputEvents::processKey(key_code);
}
