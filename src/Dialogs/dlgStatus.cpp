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
#include "MapWindow.hpp"
#include "GlideComputer.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "Compiler.h"

#include <assert.h>
#include <stdio.h>

#include <algorithm>

using std::min;
using std::max;

static const TCHAR *const captions[] = {
  N_("Flight"),
  N_("System"),
  N_("Task"),
  N_("Task Rules"),
  N_("Times"),
};

static WndForm *wf = NULL;
static TabbedControl *tabbed;
static int status_page = 0;

static void
SetCaption()
{
  static unsigned status_page;
  TCHAR caption[64];

  status_page = tabbed->GetCurrentPage();
  _sntprintf(caption, 64, _T("%u %s"),
             status_page + 1, gettext(captions[status_page]));
  wf->SetCaption(caption);
}

static void
NextPage()
{
  tabbed->NextPage();
  SetCaption();
}

static void
PrevPage()
{
  tabbed->PreviousPage();
  SetCaption();
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
    PrevPage();
    return true;

  case VK_RIGHT:
#ifdef GNAV
  case '7':
#endif
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    NextPage();
    return true;

  default:
    return false;
  }
}

static void
OnNextClicked(gcc_unused WndButton &Sender)
{
  NextPage();
}

static void
OnPrevClicked(gcc_unused WndButton &Sender)
{
  PrevPage();
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(NULL)
};

static void
UpdateValuesSystem()
{
  const NMEA_INFO &basic = CommonInterface::Basic();
  const GPS_STATE &gps = basic.gps;

  TCHAR Temp[80];
  TCHAR Temp2[80];

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpGPS"));
  assert(wp != NULL);
  if (!basic.Connected)
    wp->SetText(_("Disconnected"));
  else if (!basic.LocationAvailable)
    wp->SetText(_("Fix invalid"));
  else if (gps.SatellitesUsed == 0)
    wp->SetText(_("No fix"));
  else
    wp->SetText(_("3D fix"));

  wp->RefreshDisplay();


  wp = (WndProperty*)wf->FindByName(_T("prpNumSat"));
  assert(wp != NULL);
  if (!basic.Connected)
    wp->SetText(_T(""));
  else if (gps.SatellitesUsed >= 0) {
    // known number of sats
    _stprintf(Temp,_T("%d"), gps.SatellitesUsed);
    wp->SetText(Temp);
  } else
    // valid but unknown number of sats
    wp->SetText(_T(">3"));

  wp->RefreshDisplay();


  wp = (WndProperty*)wf->FindByName(_T("prpVario"));
  assert(wp != NULL);
  if (basic.TotalEnergyVarioAvailable)
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
  if (basic.SupplyBatteryVoltageAvailable) {
    _stprintf(Temp2, _T("%.1f V"), (double)basic.SupplyBatteryVoltage);
    _tcscat(Temp, Temp2);
  }

  wp->SetText(Temp);
  wp->RefreshDisplay();
}


static void
UpdateValuesTimes(void)
{
  const NMEA_INFO &basic = CommonInterface::Basic();
  const DERIVED_INFO &calculated = CommonInterface::Calculated();
  const FLYING_STATE &flight = calculated.flight;

  WndProperty *wp;
  TCHAR Temp[1000];
  double sunsettime;
  int sunsethours;
  int sunsetmins;

  sunsettime = XCSoarInterface::Calculated().TimeSunset;
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime - sunsethours) * 60);

  wp = (WndProperty*)wf->FindByName(_T("prpSunset"));
  assert(wp != NULL);
  _stprintf(Temp, _T("%02d:%02d"), sunsethours, sunsetmins);
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(_T("prpLocalTime"));
  assert(wp != NULL);
  Units::TimeToTextHHMMSigned(Temp, (int)DetectCurrentTime(&basic));
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(_T("prpTakeoffTime"));
  assert(wp != NULL);
  if (positive(flight.FlightTime)) {
    Units::TimeToTextHHMMSigned(Temp, TimeLocal((long)flight.TakeOffTime));
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpLandingTime"));
  assert(wp != NULL);
  if (!flight.Flying && positive(flight.FlightTime)) {
    Units::TimeToTextHHMMSigned(Temp,
                      TimeLocal((long)(flight.TakeOffTime
                                       + flight.FlightTime)));
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFlightTime"));
  assert(wp != NULL);
  if (positive(flight.FlightTime)) {
    Units::TimeToTextHHMMSigned(Temp, (int)flight.FlightTime);
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }
}

static const Waypoint* nearest_waypoint;

static void
UpdateValuesFlight(void)
{
  const NMEA_INFO &basic = CommonInterface::Basic();
  const DERIVED_INFO &calculated = CommonInterface::Calculated();

  WndProperty *wp;
  TCHAR Temp[1000];
  TCHAR sLongitude[16];
  TCHAR sLatitude[16];

  Units::LongitudeToString(basic.Location.Longitude,
                           sLongitude, sizeof(sLongitude)-1);
  Units::LatitudeToString(basic.Location.Latitude,
                          sLatitude, sizeof(sLatitude)-1);

  wp = (WndProperty*)wf->FindByName(_T("prpLongitude"));
  assert(wp != NULL);
  wp->SetText(sLongitude);

  wp = (WndProperty*)wf->FindByName(_T("prpLatitude"));
  assert(wp != NULL);
  wp->SetText(sLatitude);

  wp = (WndProperty*)wf->FindByName(_T("prpAltitude"));
  assert(wp != NULL);
  if (basic.GPSAltitudeAvailable) {
    _stprintf(Temp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(basic.GPSAltitude),
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMaxHeightGain"));
  assert(wp != NULL);
  _stprintf(Temp, _T("%d %s"),
            (int)Units::ToUserAltitude(calculated.MaxHeightGain),
            Units::GetAltitudeName());
  wp->SetText(Temp);

  if (nearest_waypoint) {
    GeoVector vec(basic.Location,
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

  const DERIVED_INFO &calculated = CommonInterface::Calculated();
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
      _tcsncpy(name, name2, 63);
      name[63] = _T('\0');
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

  const DERIVED_INFO &calculated = CommonInterface::Calculated();
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

void
dlgStatusShowModal(int start_page)
{
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  _T("IDR_XML_STATUS"));
  assert(wf);

  wf->SetKeyDownNotify(FormKeyDown);
  wf->SetTimerNotify(OnTimerNotify);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  tabbed = ((TabbedControl *)wf->FindByName(_T("tabbed")));
  assert(tabbed != NULL);

  if (start_page != -1) {
    status_page = start_page;

    WndButton *wb;
    wb = ((WndButton *)wf->FindByName(_T("cmdNext")));
    assert(wb != NULL);
    wb->hide();

    wb = ((WndButton *)wf->FindByName(_T("cmdPrev")));
    assert(wb != NULL);
    wb->hide();
  }

  /* restore previous page */
  tabbed->SetCurrentPage(status_page);

  nearest_waypoint = way_points.get_nearest(XCSoarInterface::Basic().Location);

  UpdateValuesSystem();
  UpdateValuesFlight();
  UpdateValuesTask();
  UpdateValuesRules();
  UpdateValuesTimes();
  SetCaption();

  wf->ShowModal();

  /* save page number for next time this dialog is opened */
  status_page = tabbed->GetCurrentPage();

  delete wf;
}
