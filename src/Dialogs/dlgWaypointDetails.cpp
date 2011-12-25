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

#include "Dialogs/Waypoint.hpp"
#include "Dialogs/Internal.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Engine/Task/TaskEvents.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Protection.hpp"
#include "Math/Earth.hpp"
#include "Profile/Profile.hpp"
#include "LocalTime.hpp"
#include "UtilsText.hpp"
#include "OS/PathName.hpp"
#include "Math/SunEphemeris.hpp"
#include "ComputerSettings.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Math/FastMath.h"
#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Components.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/TaskManager.hpp"
#include "Task/MapTaskManager.hpp"
#include "Task/Tasks/TaskSolvers/TaskSolution.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Compiler.h"
#include "Compatibility/path.h"
#include "Input/InputEvents.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Units/Units.hpp"
#include "Units/AngleFormatter.hpp"
#include "Util/Macros.hpp"

#include <assert.h>
#include <stdio.h>
#include <windef.h> /* for MAX_PATH */

static int page = 0;
static WndForm *wf = NULL;
static EditWindow *wDetails = NULL;
static WndFrame *wInfo = NULL;
static WndFrame *wCommand = NULL;
static const Waypoint *waypoint = NULL;

static void
NextPage(int Step)
{
  assert(waypoint);
  bool page_ok = false;
  page += Step;

  do {
    if (page < 0)
      page = 4;
    if (page > 4)
      page = 0;

    switch (page) {
    case 0:
      page_ok = true;
      break;

    case 1:
      if (waypoint->details.empty())
        page += Step;
      else
        page_ok = true;

      break;

    case 2:
      page_ok = true;
      break;

    default:
      page_ok = true;
      page = 0;
      break;
      // error!
    }
  } while (!page_ok);

  wInfo->set_visible(page == 0);
  wDetails->set_visible(page == 1);
  wCommand->set_visible(page == 2);
}

static void
OnNextClicked(gcc_unused WndButton &button)
{
  NextPage(+1);
}

static void
OnPrevClicked(gcc_unused WndButton &button)
{
  NextPage(-1);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static bool
FormKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  switch (key_code) {
  case VK_LEFT:
#ifdef GNAV
  case '6':
#endif
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    NextPage(-1);
    return true;

  case VK_RIGHT:
#ifdef GNAV
  case '7':
#endif
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    NextPage(+1);
    return true;

  default:
    return false;
  }
}

static void
OnGotoClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  assert(waypoint != NULL);

  protected_task_manager->DoGoto(*waypoint);
  wf->SetModalResult(mrOK);

  CommonInterface::main_window.full_redraw();
}

static void
OnReplaceClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::ReplaceInTask(*waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
    MessageBoxX(_("No active task point."), _("Replace in task"),
                MB_OK | MB_ICONINFORMATION);
    break;

  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_TO_GOTO:
  case MapTaskManager::MUTATED_FROM_GOTO:
    break;
  }
}

static void 
OnNewHomeClicked(gcc_unused WndButton &button)
{
  assert(waypoint != NULL);

  ComputerSettings &settings_computer =
    CommonInterface::SetComputerSettings();

  settings_computer.SetHome(*waypoint);

  {
    ScopeSuspendAllThreads suspend;
    WaypointGlue::SetHome(way_points, terrain,
                          settings_computer,
                          false);
  }

  wf->SetModalResult(mrOK);
}

static void
OnInsertInTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::InsertInTask(*waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;

  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_TO_GOTO:
    MessageBoxX(_("Created Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::MUTATED_FROM_GOTO:
    MessageBoxX(_("Created 2-point task from Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  }
}

static void
OnAppendInTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::AppendToTask(*waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_TO_GOTO:
    MessageBoxX(_("Created Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::MUTATED_FROM_GOTO:
    MessageBoxX(_("Created 2-point task from Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  }
}

#if 0
// JMW disabled until 6.2 work, see #996
static task_edit_result
goto_and_clear_task(const Waypoint &wp)
{
  if (protected_task_manager == NULL)
    return INVALID;

  protected_task_manager->DoGoto(wp);
  TaskEvents task_events;
  const OrderedTask blank(task_events,
                          XCSoarInterface::GetComputerSettings(),
                          XCSoarInterface::Calculated().glide_polar_task);
  protected_task_manager->task_commit(blank);

  return SUCCESS;
}

static unsigned
ordered_task_size()
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::Lease task_manager(*protected_task_manager);
  const OrderedTask &ot = task_manager->get_ordered_task();
  if (ot.check_task())
    return ot.TaskSize();

  return 0;
}

static void
OnGotoAndClearTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  if ((ordered_task_size() > 2) && MessageBoxX(_("Clear current task?"),
                        _("Goto and clear task"), MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  switch (goto_and_clear_task(*waypoint)) {
  case SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case NOTASK:
  case UNMODIFIED:
  case INVALID:
    MessageBoxX(_("Unknown error creating task."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  }
}
#endif

static void
OnRemoveFromTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::RemoveFromTask(*waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
    MessageBoxX(_("Waypoint not in task."), _("Remove from task"),
                MB_OK | MB_ICONINFORMATION);
    break;

  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_FROM_GOTO:
  case MapTaskManager::MUTATED_TO_GOTO:
    break;
  }
}

static void
OnActivatePanClicked(gcc_unused WndButton &button)
{
  GlueMapWindow *map_window = CommonInterface::main_window.ActivateMap();
  if (map_window == NULL)
    return;

  map_window->PanTo(waypoint->location);
  XCSoarInterface::main_window.SetFullScreen(true);
  InputEvents::setMode(InputEvents::MODE_PAN);
  wf->SetModalResult(mrOK);
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
    DeclareCallBackEntry(OnNextClicked),
    DeclareCallBackEntry(OnPrevClicked),
    DeclareCallBackEntry(OnGotoClicked),
    DeclareCallBackEntry(OnCloseClicked),
    DeclareCallBackEntry(OnReplaceClicked),
    DeclareCallBackEntry(OnInsertInTaskClicked),
    DeclareCallBackEntry(OnAppendInTaskClicked),
    DeclareCallBackEntry(OnRemoveFromTaskClicked),
    DeclareCallBackEntry(OnNewHomeClicked),
    DeclareCallBackEntry(OnActivatePanClicked),
    DeclareCallBackEntry(NULL)
};

static void
ShowTaskCommands()
{
  if (protected_task_manager == NULL)
    return;

  WndButton *wb = ((WndButton *)wf->FindByName(_T("cmdRemoveFromTask")));
  assert(wb != NULL);
  wb->set_visible(MapTaskManager::GetIndexInTask(*waypoint) >= 0);
}

static void
UpdateCaption(const TCHAR *waypoint_name)
{
  StaticString<256> buffer;
  buffer.Format(_T("%s: '%s'"), _("Waypoint Info"), waypoint_name);
  wf->SetCaption(buffer);
}

static void
UpdateComment(const TCHAR *comment)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("prpWpComment"));
  assert(wp != NULL);
  wp->SetText(comment);
}

static void
UpdateLocation(const GeoPoint &location)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("Location"));
  assert(wp != NULL);

  TCHAR buffer[64];
  if (Units::FormatGeoPoint(location, buffer, ARRAY_SIZE(buffer)) != NULL)
    wp->SetText(buffer);
}

static void
UpdateDistanceBearing(const GeoPoint &own_location,
                      const GeoPoint &waypoint_location)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("BearingDistance"));
  assert(wp != NULL);

  GeoVector vector = own_location.DistanceBearing(waypoint_location);

  TCHAR distance_buffer[32];
  Units::FormatUserDistance(vector.distance, distance_buffer,
                            ARRAY_SIZE(distance_buffer));

  TCHAR buffer[64];
  FormatBearing(buffer, ARRAY_SIZE(buffer), vector.bearing, distance_buffer);

  wp->SetText(buffer);
}

static void
UpdateElevation(fixed elevation)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("prpAltitude"));
  assert(wp != NULL);

  TCHAR buffer[64];
  Units::FormatUserAltitude(elevation, buffer, ARRAY_SIZE(buffer));
  wp->SetText(buffer);
}

static void
UpdateRadioFrequency(const RadioFrequency &radio_frequency)
{
  if (!radio_frequency.IsDefined())
    return;

  WndProperty *wp = (WndProperty *)wf->FindByName(_T("Radio"));
  assert(wp != NULL);

  TCHAR buffer[64];
  if (radio_frequency.Format(buffer, ARRAY_SIZE(buffer)) != NULL) {
    _tcscat(buffer, _T(" MHz"));
    wp->SetText(buffer);
  }
}

static void
UpdateRunwayInformation(const Runway &runway)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("Runway"));
  assert(wp != NULL);

  StaticString<64> buffer;
  if (runway.IsDirectionDefined())
    buffer.Format(_T("%02u"), runway.GetDirectionName());
  else
    buffer.clear();

  if (runway.IsLengthDefined()) {
    if (!buffer.empty())
      buffer += _T("; ");

    TCHAR length_buffer[16];
    Units::FormatUserDistance(fixed(runway.GetLength()),
                              length_buffer, ARRAY_SIZE(length_buffer));
    buffer += length_buffer;
  }

  wp->SetText(buffer);
}

static void
UpdateSunsetTime(const GeoPoint &location, const BrokenDateTime &date_time)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("prpSunset"));
  assert(wp != NULL);

  SunEphemeris::Result sun = SunEphemeris::CalcSunTimes(
      location, date_time, fixed(GetUTCOffset()) / 3600);

  int sunset_hour = (int)sun.time_of_sunset;
  int sunset_minute = (int)((sun.time_of_sunset - fixed(sunset_hour)) * 60);

  StaticString<64> buffer;
  buffer.Format(_T("%02d:%02d"), sunset_hour, sunset_minute);

  wp->SetText(buffer);
}

static void
UpdateArrivalAltitudes(const ComputerSettings &settings_computer,
                       const MoreData &basic, const DerivedInfo &calculated,
                       const Waypoint &waypoint)
{
  GlidePolar glide_polar = settings_computer.glide_polar_task;
  const GlidePolar &safety_polar = calculated.glide_polar_safety;

  // alt reqd at current mc
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("prpMc2"));
  assert(wp != NULL);

  GlideResult r = TaskSolution::GlideSolutionRemaining(
      basic.location, waypoint.location,
      waypoint.altitude + settings_computer.task.safety_height_arrival,
      basic.nav_altitude, calculated.wind, glide_polar);

  StaticString<64> buffer;
  buffer.Format(_T("%.0f %s"),
                (double)Units::ToUserAltitude(r.altitude_difference),
                Units::GetAltitudeName());
  wp->SetText(buffer);

  // alt reqd at mc 0
  wp = (WndProperty *)wf->FindByName(_T("prpMc0"));
  assert(wp != NULL);

  glide_polar.SetMC(fixed_zero);
  r = TaskSolution::GlideSolutionRemaining(
      basic.location, waypoint.location,
      waypoint.altitude + settings_computer.task.safety_height_arrival,
      basic.nav_altitude, calculated.wind, glide_polar);

  buffer.Format(_T("%.0f %s"),
                (double)Units::ToUserAltitude(r.altitude_difference),
                Units::GetAltitudeName());
  wp->SetText(buffer);

  // alt reqd at safety mc
  wp = (WndProperty *)wf->FindByName(_T("prpMc1"));
  assert(wp != NULL);

  r = TaskSolution::GlideSolutionRemaining(
      basic.location, waypoint.location,
      waypoint.altitude + settings_computer.task.safety_height_arrival,
      basic.nav_altitude, calculated.wind, safety_polar);

  buffer.Format(_T("%.0f %s"),
                (double)Units::ToUserAltitude(r.altitude_difference),
                Units::GetAltitudeName());
  wp->SetText(buffer);
}

void 
dlgWaypointDetailsShowModal(SingleWindow &parent, const Waypoint &_waypoint,
                            bool allow_navigation)
{
  const MoreData &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  waypoint = &_waypoint;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_WAYPOINTDETAILS_L") :
                                      _T("IDR_XML_WAYPOINTDETAILS"));
  assert(wf != NULL);

  UpdateCaption(waypoint->name.c_str());
  UpdateComment(waypoint->comment.c_str());

  UpdateLocation(waypoint->location);

  if (basic.location_available)
    UpdateDistanceBearing(basic.location, waypoint->location);

  UpdateElevation(waypoint->altitude);
  UpdateRadioFrequency(waypoint->radio_frequency);
  UpdateRunwayInformation(waypoint->runway);

  if (basic.connected)
    UpdateSunsetTime(waypoint->location, basic.date_time_utc);

  if (protected_task_manager != NULL)
    UpdateArrivalAltitudes(settings_computer, basic, calculated, *waypoint);

  wf->SetKeyDownNotify(FormKeyDown);

  wInfo = ((WndFrame *)wf->FindByName(_T("frmInfos")));
  assert(wInfo != NULL);

  wCommand = ((WndFrame *)wf->FindByName(_T("frmCommands")));
  assert(wCommand != NULL);
  wCommand->hide();

  wDetails = (EditWindow*)wf->FindByName(_T("frmDetails"));
  assert(wDetails != NULL);
  wDetails->set_text(waypoint->details.c_str());

  if (!allow_navigation) {
    WndButton* butnav = (WndButton *)wf->FindByName(_T("cmdPrev"));
    assert(butnav != NULL);
    butnav->hide();

    butnav = (WndButton *)wf->FindByName(_T("cmdNext"));
    assert(butnav != NULL);
    butnav->hide();

    butnav = (WndButton *)wf->FindByName(_T("cmdGoto"));
    assert(butnav != NULL);
    butnav->hide();
  }

  ShowTaskCommands();

  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;
}
