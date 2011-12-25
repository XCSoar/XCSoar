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
#include "Task/Tasks/BaseTask/UnorderedTaskPoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Compiler.h"
#include "Compatibility/path.h"
#include "Input/InputEvents.hpp"
#include "NMEA/Aircraft.hpp"
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
static const Waypoint *selected_waypoint = NULL;

static void
NextPage(int Step)
{
  assert(selected_waypoint);
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
      if (selected_waypoint->details.empty()) 
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

  assert(selected_waypoint != NULL);

  protected_task_manager->DoGoto(*selected_waypoint);
  wf->SetModalResult(mrOK);

  CommonInterface::main_window.full_redraw();
}

static void
OnReplaceClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::ReplaceInTask(*selected_waypoint)) {
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
  assert(selected_waypoint != NULL);

  ComputerSettings &settings_computer =
    CommonInterface::SetComputerSettings();

  settings_computer.SetHome(*selected_waypoint);

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

  switch (MapTaskManager::InsertInTask(*selected_waypoint)) {
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

  switch (MapTaskManager::AppendToTask(*selected_waypoint)) {
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

  switch (goto_and_clear_task(*selected_waypoint)) {
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

  switch (MapTaskManager::RemoveFromTask(*selected_waypoint)) {
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

  map_window->PanTo(selected_waypoint->location);
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
  wb->set_visible(MapTaskManager::GetIndexInTask(*selected_waypoint) >= 0);
}

void 
dlgWaypointDetailsShowModal(SingleWindow &parent, const Waypoint& way_point,
                            bool allow_navigation)
{
  const MoreData &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  selected_waypoint = &way_point;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_WAYPOINTDETAILS_L") :
                                      _T("IDR_XML_WAYPOINTDETAILS"));
  assert(wf != NULL);

  TCHAR sTmp[128];
  _stprintf(sTmp, _T("%s: '%s'"), wf->GetCaption(), selected_waypoint->name.c_str());
  wf->SetCaption(sTmp);

  WndProperty *wp = ((WndProperty *)wf->FindByName(_T("prpWpComment")));
  wp->SetText(selected_waypoint->comment.c_str());

  TCHAR *radio_frequency;
  if (selected_waypoint->radio_frequency.IsDefined() &&
      (radio_frequency =
       selected_waypoint->radio_frequency.Format(sTmp, 128)) != NULL)
    ((WndProperty *)wf->FindByName(_T("Radio")))->SetText(radio_frequency);

  const Runway &runway = selected_waypoint->runway;
  if (runway.IsDirectionDefined()) {
    _stprintf(sTmp, _T("%02u"), runway.GetDirectionName());
    if (runway.IsLengthDefined()) {
      _tcscat(sTmp, _T("; "));
      Units::FormatUserDistance(fixed(runway.GetLength()),
                                sTmp + _tcslen(sTmp), 128 - _tcslen(sTmp));
    }

    ((WndProperty *)wf->FindByName(_T("Runway")))->SetText(sTmp);
  }

  TCHAR *location = Units::FormatGeoPoint(selected_waypoint->location,
                                          sTmp, 128);
  if (location != NULL)
    ((WndProperty *)wf->FindByName(_T("Location")))->SetText(location);

  Units::FormatUserAltitude(selected_waypoint->altitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(_T("prpAltitude")))
    ->SetText(sTmp);

  if (basic.connected) {
    SunEphemeris sun;
    sun.CalcSunTimes(selected_waypoint->location,
                     basic.date_time_utc,
                     fixed(GetUTCOffset()) / 3600);

    int sunsethours = (int)sun.TimeOfSunSet;
    int sunsetmins = (int)((sun.TimeOfSunSet - fixed(sunsethours)) * 60);

    _stprintf(sTmp, _T("%02d:%02d"), sunsethours, sunsetmins);
    ((WndProperty *)wf->FindByName(_T("prpSunset")))->SetText(sTmp);
  }

  if (basic.location_available) {
    GeoVector gv = basic.location.DistanceBearing(selected_waypoint->location);

    TCHAR DistanceText[MAX_PATH];
    Units::FormatUserDistance(gv.distance, DistanceText, 10);

    FormatBearing(sTmp, ARRAY_SIZE(sTmp), gv.bearing, DistanceText);
    ((WndProperty *)wf->FindByName(_T("BearingDistance")))->SetText(sTmp);
  }

  if (protected_task_manager != NULL) {
    GlidePolar glide_polar = settings_computer.glide_polar_task;
    const GlidePolar &safety_polar = calculated.glide_polar_safety;

    UnorderedTaskPoint t(way_point, settings_computer.task);

    const AircraftState aircraft_state = ToAircraftState(basic, calculated);
    GlideResult r = TaskSolution::glide_solution_remaining(t, aircraft_state,
                                                           glide_polar);
    // alt reqd at current mc
    wp = (WndProperty *)wf->FindByName(_T("prpMc2"));
    assert(wp != NULL);
    _stprintf(sTmp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(r.altitude_difference),
              Units::GetAltitudeName());
    wp->SetText(sTmp);

    // alt reqd at mc 0
    glide_polar.SetMC(fixed_zero);
    r = TaskSolution::glide_solution_remaining(t, aircraft_state, glide_polar);
    wp = (WndProperty *)wf->FindByName(_T("prpMc0"));
    assert(wp != NULL);
    _stprintf(sTmp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(r.altitude_difference),
              Units::GetAltitudeName());
    wp->SetText(sTmp);

    // alt reqd at safety mc
    r = TaskSolution::glide_solution_remaining(t, aircraft_state, safety_polar);
    wp = (WndProperty *)wf->FindByName(_T("prpMc1"));
    assert(wp != NULL);
    _stprintf(sTmp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(r.altitude_difference),
              Units::GetAltitudeName());
    wp->SetText(sTmp);
  }

  wf->SetKeyDownNotify(FormKeyDown);

  wInfo = ((WndFrame *)wf->FindByName(_T("frmInfos")));
  assert(wInfo != NULL);

  wCommand = ((WndFrame *)wf->FindByName(_T("frmCommands")));
  assert(wCommand != NULL);
  wCommand->hide();

  wDetails = (EditWindow*)wf->FindByName(_T("frmDetails"));
  assert(wDetails != NULL);
  wDetails->set_text(selected_waypoint->details.c_str());

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
