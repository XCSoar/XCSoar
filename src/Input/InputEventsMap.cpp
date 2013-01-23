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

#include "InputEvents.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Pages.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "UIGlobals.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Units/Units.hpp"
#include "UIState.hpp"
#include "Asset.hpp"
#include "Pan.hpp"
#include "Util/Clamp.hpp"

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

  MapSettings &settings_map = CommonInterface::SetMapSettings();

  if (StringIsEqual(misc, _T("auto toggle")))
    sub_AutoZoom(-1);
  else if (StringIsEqual(misc, _T("auto on")))
    sub_AutoZoom(1);
  else if (StringIsEqual(misc, _T("auto off")))
    sub_AutoZoom(0);
  else if (StringIsEqual(misc, _T("auto show"))) {
    if (settings_map.auto_zoom_enabled)
      Message::AddMessage(_("Auto. zoom on"));
    else
      Message::AddMessage(_("Auto. zoom off"));
  } else if (StringIsEqual(misc, _T("slowout")))
    sub_ScaleZoom(-1);
  else if (StringIsEqual(misc, _T("slowin")))
    sub_ScaleZoom(1);
  else if (StringIsEqual(misc, _T("out")))
    sub_ScaleZoom(-1);
  else if (StringIsEqual(misc, _T("in")))
    sub_ScaleZoom(1);
  else if (StringIsEqual(misc, _T("-")))
    sub_ScaleZoom(-1);
  else if (StringIsEqual(misc, _T("+")))
    sub_ScaleZoom(1);
  else if (StringIsEqual(misc, _T("--")))
    sub_ScaleZoom(-2);
  else if (StringIsEqual(misc, _T("++")))
    sub_ScaleZoom(2);
  else if (StringIsEqual(misc, _T("circlezoom toggle"))) {
    settings_map.circle_zoom_enabled = !settings_map.circle_zoom_enabled;
  } else if (StringIsEqual(misc, _T("circlezoom on"))) {
    settings_map.circle_zoom_enabled = true;
  } else if (StringIsEqual(misc, _T("circlezoom off"))) {
    settings_map.circle_zoom_enabled = false;
  } else if (StringIsEqual(misc, _T("circlezoom show"))) {
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

  XCSoarInterface::SendMapSettings(true);
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
  if (StringIsEqual(misc, _T("toggle")) ||
      /* deprecated: */ StringIsEqual(misc, _T("supertoggle")))
    TogglePan();

  else if (StringIsEqual(misc, _T("on")))
    EnterPan();

  else if (StringIsEqual(misc, _T("off")))
    LeavePan();

  else if (StringIsEqual(misc, _T("up")))
    if (IsHP31X())
      // Scroll wheel on the HP31x series should zoom in pan mode
      sub_ScaleZoom(1);
    else
      sub_PanCursor(0, 1);

  else if (StringIsEqual(misc, _T("down")))
    if (IsHP31X())
      // Scroll wheel on the HP31x series should zoom in pan mode
      sub_ScaleZoom(-1);
    else
      sub_PanCursor(0, -1);

  else if (StringIsEqual(misc, _T("left")))
    sub_PanCursor(1, 0);

  else if (StringIsEqual(misc, _T("right")))
    sub_PanCursor(-1, 0);

  XCSoarInterface::SendMapSettings(true);
}

void
InputEvents::sub_PanCursor(int dx, int dy)
{
  GlueMapWindow *map_window = UIGlobals::GetMapIfActive();
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
  MapSettings &settings_map = CommonInterface::SetMapSettings();

  if (vswitch == -1)
    settings_map.auto_zoom_enabled = !settings_map.auto_zoom_enabled;
  else
    settings_map.auto_zoom_enabled = (vswitch != 0); // 0 off, 1 on

  Profile::Set(ProfileKeys::AutoZoom, settings_map.auto_zoom_enabled);

  if (settings_map.auto_zoom_enabled &&
      UIGlobals::GetMap() != NULL)
    UIGlobals::GetMap()->SetPan(false);

  ActionInterface::SendMapSettings(true);
}

void
InputEvents::sub_SetZoom(fixed value)
{
  MapSettings &settings_map = CommonInterface::SetMapSettings();
  GlueMapWindow *map_window = UIGlobals::ActivateMap();
  if (map_window == NULL)
    return;

  const DisplayMode displayMode = CommonInterface::GetUIState().display_mode;
  if (settings_map.auto_zoom_enabled &&
      !(displayMode == DisplayMode::CIRCLING && settings_map.circle_zoom_enabled) &&
      !IsPanning()) {
    settings_map.auto_zoom_enabled = false;  // disable autozoom if user manually changes zoom
    Profile::Set(ProfileKeys::AutoZoom, false);
    Message::AddMessage(_("Auto. zoom off"));
  }

  fixed vmin = CommonInterface::GetComputerSettings().polar.glide_polar_task.GetVMin();
  fixed scale_2min_distance = vmin * 12;
  const fixed scale_100m = fixed(10);
  const fixed scale_1600km = fixed(1600*100);
  fixed minreasonable = displayMode == DisplayMode::CIRCLING
    ? scale_100m
    : std::max(scale_100m, scale_2min_distance);

  value = Clamp(value, minreasonable, scale_1600km);
  map_window->SetMapScale(value);
  map_window->QuickRedraw();
}

void
InputEvents::sub_ScaleZoom(int vswitch)
{
  const GlueMapWindow *map_window = UIGlobals::ActivateMap();
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

void
InputEvents::eventMap(const TCHAR *misc)
{
  if (StringIsEqual(misc, _T("show")))
    UIGlobals::ActivateMap();
}
