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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Internal.hpp"
#include "Screen/Key.h"
#include "Protection.hpp"
#include "Blackboard.hpp"
#include "Math/Earth.hpp"
#include "Hardware/Battery.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Logger/Logger.hpp"
#include "Math/FastMath.h"
#include "LocalTime.hpp"
#include "MainWindow.hpp"
#include "GlideComputer.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "Compiler.h"

#include "Form/TabBar.hpp"
#include "Appearance.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"

#include <assert.h>
#include <stdio.h>

#include <algorithm>

static WndForm *wf = NULL;
static TabBarControl *wTabBar;
static int status_page = 0;

static void
SetTitle()
{
  TCHAR title[99];
  _stprintf(title, _T("Status: %s"), wTabBar->GetButtonCaption((wTabBar->GetCurrentPage())));
  wf->SetCaption(title);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(NULL)
};

static void
UpdateValuesSystem()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const GPSState &gps = basic.gps;

  TCHAR Temp[80];
  TCHAR Temp2[80];

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpGPS"));
  assert(wp != NULL);
  if (!basic.connected)
    wp->SetText(_("Disconnected"));
  else if (!basic.location_available)
    wp->SetText(_("Fix invalid"));
  else if (gps.satellites_used == 0)
    wp->SetText(_("No fix"));
  else
    wp->SetText(_("3D fix"));

  wp->RefreshDisplay();


  wp = (WndProperty*)wf->FindByName(_T("prpNumSat"));
  assert(wp != NULL);
  if (!basic.connected)
    wp->SetText(_T(""));
  else if (gps.satellites_used >= 0) {
    // known number of sats
    _stprintf(Temp,_T("%d"), gps.satellites_used);
    wp->SetText(Temp);
  } else
    // valid but unknown number of sats
    wp->SetText(_T(">3"));

  wp->RefreshDisplay();


  wp = (WndProperty*)wf->FindByName(_T("prpVario"));
  assert(wp != NULL);
  if (basic.total_energy_vario_available)
    wp->SetText(_("Connected"));
  else
    wp->SetText(_("Disconnected"));

  wp->RefreshDisplay();


  wp = (WndProperty*)wf->FindByName(_T("prpFLARM"));
  assert(wp != NULL);
  if (basic.flarm.available)
    wp->SetText(_("Connected"));
  else
    wp->SetText(_("Disconnected"));

  wp->RefreshDisplay();


  wp = (WndProperty*)wf->FindByName(_T("prpLogger"));
  assert(wp != NULL);
  if (logger.isLoggerActive())
    wp->SetText(_("On"));
  else
    wp->SetText(_("Off"));

  wp->RefreshDisplay();


  wp = (WndProperty*)wf->FindByName(_T("prpDeclared"));
  assert(wp != NULL);
  if (logger.isTaskDeclared())
    wp->SetText(_("Yes"));
  else
    wp->SetText(_("No"));

  wp->RefreshDisplay();


  wp = (WndProperty*)wf->FindByName(_T("prpBattery"));
  assert(wp != NULL);
  Temp[0] = 0;
#ifdef HAVE_BATTERY
  if (Power::Battery::RemainingPercentValid) {
    _stprintf(Temp2, _T("%d %% "), Power::Battery::RemainingPercent);
    _tcscat(Temp, Temp2);
  }
#endif
  if (basic.voltage_available) {
    _stprintf(Temp2, _T("%.1f V"), (double)basic.voltage);
    _tcscat(Temp, Temp2);
  }

  wp->SetText(Temp);
  wp->RefreshDisplay();
}


static void
UpdateValuesTimes(void)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const FlyingState &flight = calculated.flight;

  WndProperty *wp;
  TCHAR Temp[1000];
  double sunsettime;
  int sunsethours;
  int sunsetmins;

  sunsettime = XCSoarInterface::Calculated().sunset_time;
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime - sunsethours) * 60);

  wp = (WndProperty*)wf->FindByName(_T("prpSunset"));
  assert(wp != NULL);
  _stprintf(Temp, _T("%02d:%02d"), sunsethours, sunsetmins);
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(_T("prpLocalTime"));
  assert(wp != NULL);
  Units::TimeToTextHHMMSigned(Temp, DetectCurrentTime(basic));
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(_T("prpTakeoffTime"));
  assert(wp != NULL);
  if (positive(flight.flight_time)) {
    Units::TimeToTextHHMMSigned(Temp, TimeLocal((long)flight.takeoff_time));
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpLandingTime"));
  assert(wp != NULL);
  if (!flight.flying && positive(flight.flight_time)) {
    Units::TimeToTextHHMMSigned(Temp,
                      TimeLocal((long)(flight.takeoff_time
                                       + flight.flight_time)));
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFlightTime"));
  assert(wp != NULL);
  if (positive(flight.flight_time)) {
    Units::TimeToTextHHMMSigned(Temp, (int)flight.flight_time);
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }
}

static const Waypoint* nearest_waypoint;

static void
UpdateValuesFlight(void)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  WndProperty *wp;
  TCHAR Temp[1000];
  TCHAR sLongitude[16];
  TCHAR sLatitude[16];

  Units::LongitudeToString(basic.location.Longitude,
                           sLongitude, sizeof(sLongitude)-1);
  Units::LatitudeToString(basic.location.Latitude,
                          sLatitude, sizeof(sLatitude)-1);

  wp = (WndProperty*)wf->FindByName(_T("prpLongitude"));
  assert(wp != NULL);
  wp->SetText(sLongitude);

  wp = (WndProperty*)wf->FindByName(_T("prpLatitude"));
  assert(wp != NULL);
  wp->SetText(sLatitude);

  wp = (WndProperty*)wf->FindByName(_T("prpAltitude"));
  assert(wp != NULL);
  if (basic.gps_altitude_available) {
    _stprintf(Temp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(basic.gps_altitude),
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMaxHeightGain"));
  assert(wp != NULL);
  _stprintf(Temp, _T("%d %s"),
            (int)Units::ToUserAltitude(calculated.max_height_gain),
            Units::GetAltitudeName());
  wp->SetText(Temp);

  if (nearest_waypoint) {
    GeoVector vec(basic.location,
                  nearest_waypoint->Location);

    wp = (WndProperty*)wf->FindByName(_T("prpNear"));
    assert(wp != NULL);
    wp->SetText(nearest_waypoint->Name.c_str());

    wp = (WndProperty*)wf->FindByName(_T("prpBearing"));
    assert(wp != NULL);
    _stprintf(Temp, _T("%d")_T(DEG), (int)vec.Bearing.value_degrees());
    wp->SetText(Temp);

    wp = (WndProperty*)wf->FindByName(_T("prpDistance"));
    assert(wp != NULL);
    TCHAR DistanceText[MAX_PATH];
    Units::FormatUserDistance(vec.Distance, DistanceText, 10);
    wp->SetText(DistanceText);
  } else {
    wp = (WndProperty*)wf->FindByName(_T("prpNear"));
    assert(wp != NULL);
    wp->SetText(_T("-"));

    wp = (WndProperty*)wf->FindByName(_T("prpBearing"));
    assert(wp != NULL);
    wp->SetText(_T("-"));

    wp = (WndProperty*)wf->FindByName(_T("prpDistance"));
    assert(wp != NULL);
    wp->SetText(_T("-"));
  }
}

static void
UpdateValuesRules(void)
{
  if (protected_task_manager == NULL)
    return;

  WndProperty *wp;
  TCHAR Temp[80];

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const CommonStats &common_stats = calculated.common_stats;

  wp = (WndProperty*)wf->FindByName(_T("prpValidStart"));
  assert(wp != NULL);
  if (calculated.common_stats.task_started)
    /// @todo proper task validity check
    wp->SetText(_("Yes"));
  else
    wp->SetText(_("No"));

  wp = (WndProperty*)wf->FindByName(_T("prpValidFinish"));
  assert(wp != NULL);
  if (common_stats.task_finished)
    wp->SetText(_("Yes"));
  else
    wp->SetText(_("No"));

  AIRCRAFT_STATE start_state = protected_task_manager->get_start_state();

  wp = (WndProperty*)wf->FindByName(_T("prpStartTime"));
  assert(wp != NULL);
  if (common_stats.task_started) {
    Units::TimeToTextHHMMSigned(Temp, (int)TimeLocal(start_state.Time));
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartSpeed"));
  assert(wp != NULL);
  if (common_stats.task_started) {
    _stprintf(Temp, _T("%d %s"),
              (int)Units::ToUserTaskSpeed(start_state.Speed),
              Units::GetTaskSpeedName());
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  // StartMaxHeight, StartMaxSpeed;
  wp = (WndProperty*)wf->FindByName(_T("prpStartHeight"));
  assert(wp != NULL);
  if (common_stats.task_started) {
    _stprintf(Temp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(start_state.NavAltitude),
              Units::GetAltitudeName());
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFinishAlt"));
  assert(wp != NULL);
  _stprintf(Temp, _T("%.0f %s"),
            (double)Units::ToUserAltitude(protected_task_manager->get_finish_height()),
            Units::GetAltitudeName());
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(_T("prpStartPoint"));
  assert(wp != NULL);

  TCHAR name[64];
  {
    ProtectedTaskManager::Lease task_manager(*protected_task_manager);
    const TCHAR *name2 = task_manager->get_mode() == TaskManager::MODE_ORDERED
      ? task_manager->get_ordered_taskpoint_name(0)
      : NULL;
    if (name2 != NULL) {
      CopyString(name, name2, 64);
    } else
      name[0] = _T('\0');
  }

  wp->SetText(name);
}

static void
UpdateValuesTask(void)
{
  if (protected_task_manager == NULL)
    return;

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.task_stats;

  WndProperty *wp;
  TCHAR Temp[80];

  wp = (WndProperty*)wf->FindByName(_T("prpTaskTime"));
  Units::TimeToTextHHMMSigned(Temp, protected_task_manager->get_ordered_task_behaviour().aat_min_time);
  assert(wp != NULL);
  if (task_stats.has_targets)
    wp->SetText(Temp);
  else
    wp->hide();

  wp = (WndProperty*)wf->FindByName(_T("prpETETime"));
  assert(wp != NULL);
  Units::TimeToTextHHMMSigned(Temp,
                    task_stats.total.TimeElapsed +
                    task_stats.total.TimeRemaining);
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(_T("prpRemainingTime"));
  assert(wp != NULL);
  Units::TimeToTextHHMMSigned(Temp, task_stats.total.TimeRemaining);
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(_T("prpTaskDistance"));
  assert(wp != NULL);
  _stprintf(Temp, _T("%.0f %s"), (double)Units::ToUserDistance(
            task_stats.total.planned.get_distance()),
            Units::GetDistanceName());
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(_T("prpRemainingDistance"));
  assert(wp != NULL);
  _stprintf(Temp, _T("%.0f %s"), (double)Units::ToUserDistance(
            task_stats.total.remaining.get_distance()),
            Units::GetDistanceName());
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(_T("prpEstimatedSpeed"));
  assert(wp != NULL);
  _stprintf(Temp, _T("%.0f %s"), (double)Units::ToUserTaskSpeed(
            task_stats.total.planned.get_speed()),
            Units::GetTaskSpeedName());
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(_T("prpAverageSpeed"));
  assert(wp != NULL);
  _stprintf(Temp, _T("%.0f %s"), (double)Units::ToUserTaskSpeed(
            task_stats.total.travelled.get_speed()),
            Units::GetTaskSpeedName());
  wp->SetText(Temp);
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  UpdateValuesSystem();
}

static bool
OnTabUpdate(TabBarControl::EventType EventType)
{
  UpdateValuesSystem();
  UpdateValuesFlight();
  UpdateValuesTask();
  UpdateValuesRules();
  UpdateValuesTimes();

  return true;
}

void
dlgStatusShowModal(int start_page)
{
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ?
                  _T("IDR_XML_STATUS_L") : _T("IDR_XML_STATUS"));
  assert(wf);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wTabBar = ((TabBarControl *)wf->FindByName(_T("TabBar")));
  assert(wTabBar != NULL);

  nearest_waypoint = way_points.get_nearest(XCSoarInterface::Basic().location,
                                            fixed(100000));

  /* setup tabs */

  const DialogTabStyle_t IconsStyle = Appearance.DialogTabStyle;

  const Bitmap *FlightIcon = ((IconsStyle == dtIcon) ?
                              &Graphics::hBmpTabFlight : NULL);
  const Bitmap *SystemIcon = ((IconsStyle == dtIcon) ?
                               &Graphics::hBmpTabSystem : NULL);
  const Bitmap *TaskIcon = ((IconsStyle == dtIcon) ?
                              &Graphics::hBmpTabTask : NULL);
  const Bitmap *RulesIcon = ((IconsStyle == dtIcon) ?
                             &Graphics::hBmpTabRules : NULL);
  const Bitmap *TimesIcon = ((IconsStyle == dtIcon) ?
                             &Graphics::hBmpTabTimes : NULL);

  Window* wFlight = LoadWindow(CallBackTable, wf, *wTabBar, _T("IDR_XML_STATUS_FLIGHT"));
  assert(wFlight);

  const unsigned xoffset = (Layout::landscape ? wTabBar->GetTabWidth() : 0);
  const unsigned yoffset = (!Layout::landscape ? wTabBar->GetTabHeight() : 0);

  Window* wSystem = LoadWindow(CallBackTable, wf, *wTabBar, _T("IDR_XML_STATUS_SYSTEM"));
  assert(wSystem);

  Window* wTask = LoadWindow(CallBackTable, wf, *wTabBar, _T("IDR_XML_STATUS_TASK"));
  assert(wTask);

  Window* wRules = LoadWindow(CallBackTable, wf, *wTabBar, _T("IDR_XML_STATUS_RULES"));
  assert(wRules);

  Window* wTimes = LoadWindow(CallBackTable, wf, *wTabBar, _T("IDR_XML_STATUS_TIMES"));
  assert(wTimes);

  wTabBar->AddClient(wFlight, _T("Flight"), false, FlightIcon, NULL,
                     OnTabUpdate, SetTitle);

  wTabBar->AddClient(wSystem, _T("System"), false, SystemIcon, NULL,
                     OnTabUpdate, SetTitle);

  wTabBar->AddClient(wTask, _T("Task"), false, TaskIcon, NULL,
                     OnTabUpdate, SetTitle);

  wTabBar->AddClient(wRules, _T("Rules"), false, RulesIcon, NULL,
                     OnTabUpdate, SetTitle);

  wTabBar->AddClient(wTimes, _T("Times"), false, TimesIcon, NULL,
                     OnTabUpdate, SetTitle);

  wFlight->move(xoffset, yoffset, wf->GetClientAreaWindow().get_width() - xoffset,
                wf->GetClientAreaWindow().get_height() - yoffset);
  wSystem->move(xoffset, yoffset, wf->GetClientAreaWindow().get_width() - xoffset,
                wf->GetClientAreaWindow().get_height() - yoffset);
  wTask->move(xoffset, yoffset, wf->GetClientAreaWindow().get_width() - xoffset,
                wf->GetClientAreaWindow().get_height() - yoffset);
  wRules->move(xoffset, yoffset, wf->GetClientAreaWindow().get_width() - xoffset,
                wf->GetClientAreaWindow().get_height() - yoffset);
  wTimes->move(xoffset, yoffset, wf->GetClientAreaWindow().get_width() - xoffset,
                wf->GetClientAreaWindow().get_height() - yoffset);

  /* restore previous page */

  if (start_page != -1) {
    status_page = start_page;
  }

  wTabBar->SetCurrentPage(status_page);

  wf->SetTimerNotify(OnTimerNotify);

  SetTitle();

  wf->ShowModal();

  /* save page number for next time this dialog is opened */
  status_page = wTabBar->GetCurrentPage();

  delete wf;
}
