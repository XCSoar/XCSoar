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

#include "InputEvents.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Interface.hpp"
#include "Pages.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Units/Units.hpp"
#include "Asset.hpp"

// eventAutoZoom - Turn on|off|toggle AutoZoom
// misc:
//	auto on - Turn on if not already
//	auto off - Turn off if not already
//	auto toggle - Toggle current full screen status
//	auto show - Shows autozoom status
//	+	- Zoom in
//	++	- Zoom in near
//	-	- Zoom out
//	--	- Zoom out far
//	n.n	- Zoom to a set scale
//	show - Show current zoom scale
void
InputEvents::eventZoom(const TCHAR* misc)
{
  // JMW pass through to handler in MapWindow
  // here:
  // -1 means toggle
  // 0 means off
  // 1 means on

  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  if (_tcscmp(misc, _T("auto toggle")) == 0)
    sub_AutoZoom(-1);
  else if (_tcscmp(misc, _T("auto on")) == 0)
    sub_AutoZoom(1);
  else if (_tcscmp(misc, _T("auto off")) == 0)
    sub_AutoZoom(0);
  else if (_tcscmp(misc, _T("auto show")) == 0) {
    if (settings_map.auto_zoom_enabled)
      Message::AddMessage(_("Auto. zoom on"));
    else
      Message::AddMessage(_("Auto. zoom off"));
  } else if (_tcscmp(misc, _T("slowout")) == 0)
    sub_ScaleZoom(-1);
  else if (_tcscmp(misc, _T("slowin")) == 0)
    sub_ScaleZoom(1);
  else if (_tcscmp(misc, _T("out")) == 0)
    sub_ScaleZoom(-1);
  else if (_tcscmp(misc, _T("in")) == 0)
    sub_ScaleZoom(1);
  else if (_tcscmp(misc, _T("-")) == 0)
    sub_ScaleZoom(-1);
  else if (_tcscmp(misc, _T("+")) == 0)
    sub_ScaleZoom(1);
  else if (_tcscmp(misc, _T("--")) == 0)
    sub_ScaleZoom(-2);
  else if (_tcscmp(misc, _T("++")) == 0)
    sub_ScaleZoom(2);
  else if (_tcscmp(misc, _T("circlezoom toggle")) == 0) {
    settings_map.circle_zoom_enabled = !settings_map.circle_zoom_enabled;
  } else if (_tcscmp(misc, _T("circlezoom on")) == 0) {
    settings_map.circle_zoom_enabled = true;
  } else if (_tcscmp(misc, _T("circlezoom off")) == 0) {
    settings_map.circle_zoom_enabled = false;
  } else if (_tcscmp(misc, _T("circlezoom show")) == 0) {
    if (settings_map.circle_zoom_enabled)
      Message::AddMessage(_("Circling zoom on"));
    else
      Message::AddMessage(_("Circling zoom off"));
  } else {
    TCHAR *endptr;
    double zoom = _tcstod(misc, &endptr);
    if (endptr == misc)
      return;

    sub_SetZoom(Units::ToSysDistance(fixed(zoom)));
  }

  XCSoarInterface::SendSettingsMap(true);
}

/**
 * This function handles all "pan" input events
 * @param misc A string describing the desired pan action.
 *  on             Turn pan on
 *  off            Turn pan off
 *  toogle         Toogles pan mode
 *  supertoggle    Toggles pan mode and fullscreen
 *  up             Pan up
 *  down           Pan down
 *  left           Pan left
 *  right          Pan right
 *  @todo feature: n,n Go that direction - +/-
 *  @todo feature: ??? Go to particular point
 *  @todo feature: ??? Go to waypoint (eg: next, named)
 */
void
InputEvents::eventPan(const TCHAR *misc)
{
  if (_tcscmp(misc, _T("toggle")) == 0)
    sub_Pan(-1);

  else if (_tcscmp(misc, _T("supertoggle")) == 0)
    sub_Pan(-2);

  else if (_tcscmp(misc, _T("on")) == 0)
    sub_Pan(1);

  else if (_tcscmp(misc, _T("off")) == 0)
    sub_Pan(0);

  else if (_tcscmp(misc, _T("up")) == 0)
    if (model_is_hp31x())
      // Scroll wheel on the HP31x series should zoom in pan mode
      sub_ScaleZoom(1);
    else
      sub_PanCursor(0, 1);

  else if (_tcscmp(misc, _T("down")) == 0)
    if (model_is_hp31x())
      // Scroll wheel on the HP31x series should zoom in pan mode
      sub_ScaleZoom(-1);
    else
      sub_PanCursor(0, -1);

  else if (_tcscmp(misc, _T("left")) == 0)
    sub_PanCursor(1, 0);

  else if (_tcscmp(misc, _T("right")) == 0)
    sub_PanCursor(-1, 0);

  else if (_tcscmp(misc, _T("show")) == 0) {
    if (CommonInterface::IsPanning())
      Message::AddMessage(_("Pan mode on"));
    else
      Message::AddMessage(_("Pan mode off"));
  }

  XCSoarInterface::SendSettingsMap(true);
}

/**
 * This function switches the pan mode on and off
 * @param vswitch This parameter determines what to do:
 * -2 supertoogle
 * -1 toogle
 * 1  on
 * 0  off
 * @see InputEvents::eventPan()
 */
void
InputEvents::sub_Pan(int vswitch)
{
  GlueMapWindow *map_window = CommonInterface::main_window.ActivateMap();
  if (map_window == NULL)
    return;

  bool oldPan = map_window->IsPanning();

  if (vswitch == -2) {
    // supertoogle, toogle pan mode and fullscreen
    map_window->TogglePan();
    XCSoarInterface::main_window.SetFullScreen(true);
  } else if (vswitch == -1)
    // toogle, toogle pan mode only
    map_window->TogglePan();
  else
    // 1 = enable pan mode
    // 0 = disable pan mode
    map_window->SetPan(vswitch != 0);

  if (map_window->IsPanning() != oldPan) {
    if (map_window->IsPanning()) {
      setMode(MODE_PAN);
    } else {
      setMode(MODE_DEFAULT);
      Pages::Update();
    }
  }
}

void
InputEvents::sub_PanCursor(int dx, int dy)
{
  GlueMapWindow *map_window = CommonInterface::main_window.GetMapIfActive();
  if (map_window == NULL || !map_window->IsPanning())
    return;

  const WindowProjection &projection = map_window->VisibleProjection();

  RasterPoint pt = projection.GetScreenOrigin();
  pt.x -= dx * projection.GetScreenWidth() / 4;
  pt.y -= dy * projection.GetScreenHeight() / 4;
  map_window->SetLocation(projection.ScreenToGeo(pt));

  map_window->QuickRedraw();
}

// called from UI or input event handler (same thread)
void
InputEvents::sub_AutoZoom(int vswitch)
{
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  if (vswitch == -1)
    settings_map.auto_zoom_enabled = !settings_map.auto_zoom_enabled;
  else
    settings_map.auto_zoom_enabled = (vswitch != 0); // 0 off, 1 on

  Profile::Set(szProfileAutoZoom, settings_map.auto_zoom_enabled);

  if (settings_map.auto_zoom_enabled &&
      CommonInterface::main_window.GetMap() != NULL)
    CommonInterface::main_window.GetMap()->SetPan(false);

  ActionInterface::SendSettingsMap(true);
}

void
InputEvents::sub_SetZoom(fixed value)
{
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();
  GlueMapWindow *map_window = CommonInterface::main_window.ActivateMap();
  if (map_window == NULL)
    return;

  DisplayMode displayMode = XCSoarInterface::main_window.GetDisplayMode();
  if (settings_map.auto_zoom_enabled &&
      !(displayMode == DM_CIRCLING && settings_map.circle_zoom_enabled) &&
      !CommonInterface::IsPanning()) {
    settings_map.auto_zoom_enabled = false;  // disable autozoom if user manually changes zoom
    Profile::Set(szProfileAutoZoom, false);
    Message::AddMessage(_("Auto. zoom off"));
  }

  fixed vmin = CommonInterface::SettingsComputer().glide_polar_task.GetVMin();
  fixed scale_2min_distance = vmin * fixed_int_constant(12);
  const fixed scale_500m = fixed_int_constant(50);
  const fixed scale_1600km = fixed_int_constant(1600*100);
  fixed minreasonable = (displayMode == DM_CIRCLING) ?
                        scale_500m : max(scale_500m, scale_2min_distance);

  value = max(minreasonable, min(scale_1600km, value));
  map_window->SetMapScale(value);
  map_window->QuickRedraw();
}

void
InputEvents::sub_ScaleZoom(int vswitch)
{
  const GlueMapWindow *map_window = CommonInterface::main_window.ActivateMap();
  if (map_window == NULL)
    return;

  const MapWindowProjection &projection =
      map_window->VisibleProjection();

  fixed value = projection.GetMapScale();

  if (projection.HaveScaleList()) {
    value = projection.StepMapScale(value, -vswitch);
  } else {
    if (vswitch == 1)
      // zoom in a little
      value /= fixed_sqrt_two;
    else if (vswitch == -1)
      // zoom out a little
      value *= fixed_sqrt_two;
    else if (vswitch == 2)
      // zoom in a lot
      value /= 2;
    else if (vswitch == -2)
      // zoom out a lot
      value *= 2;
  }

  sub_SetZoom(value);
}
