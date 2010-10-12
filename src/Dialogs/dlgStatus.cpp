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
#include "Protection.hpp"
#include "Blackboard.hpp"
#include "Math/Earth.hpp"
#include "Hardware/Battery.hpp"
#include "Units.hpp"
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
static bool multi_page = false;
static int status_page = 0;

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static bool
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  (void)Sender;
  switch (key_code) {
  case VK_LEFT:
  case '6':
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    tabbed->PreviousPage();
    return true;

  case VK_RIGHT:
  case '7':
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    tabbed->NextPage();
    return true;

  default:
    return false;
  }
}

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
OnNextClicked(WindowControl * Sender)
{
  (void)Sender;

  tabbed->NextPage();
  SetCaption();
}

static void
OnPrevClicked(WindowControl * Sender)
{
  (void)Sender;

  tabbed->PreviousPage();
  SetCaption();
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(NULL)
};

static void
UpdateValuesSystem()
{
  static const GPS_STATE &gps = XCSoarInterface::Basic().gps;

  TCHAR Temp[80];
  TCHAR Temp2[80];

  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(_T("prpGPS"));
  if (wp) {
    if (gps.Connected) {
      if (gps.NAVWarning)
        wp->SetText(_("Fix invalid"));
      else if (gps.SatellitesUsed == 0)
        wp->SetText(_("No fix"));
      else
        wp->SetText(_("3D fix"));
      wp->RefreshDisplay();

      wp = (WndProperty*)wf->FindByName(_T("prpNumSat"));
      if (wp) {
        if (gps.SatellitesUsed >= 0)
          // known number of sats
          _stprintf(Temp,_T("%d"), gps.SatellitesUsed);
        else
          // valid but unknown number of sats
          _stprintf(Temp,_T(">3"));

        wp->SetText(Temp);
        wp->RefreshDisplay();
      }
    } else {
      wp->SetText(_("Disconnected"));
      wp->RefreshDisplay();
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpVario"));
  if (wp) {
    if (XCSoarInterface::Basic().TotalEnergyVarioAvailable)
      wp->SetText(_("Connected"));
    else
      wp->SetText(_("Disconnected"));

    wp->RefreshDisplay();
  }

  if (wp) {
    wp = (WndProperty*)wf->FindByName(_T("prpFLARM"));
    if (XCSoarInterface::Basic().flarm.FLARM_Available)
      wp->SetText(_("Connected"));
    else
      wp->SetText(_("Disconnected"));

    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpLogger"));
  if (wp) {
    if (logger.isLoggerActive())
      wp->SetText(_("ON"));
    else
      wp->SetText(_("OFF"));

    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDeclared"));
  if (wp) {
    if (logger.isTaskDeclared())
      wp->SetText(_("YES"));
    else
      wp->SetText(_("NO"));

    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpBattery"));
  if (wp) {
    _stprintf(Temp,_T("\0"));
#ifdef HAVE_BATTERY
    if (PDABatteryPercent >= 0) {
      _stprintf(Temp2, _T("%d%% "), PDABatteryPercent);
      _tcscat(Temp, Temp2);
    }
#endif
    if (XCSoarInterface::Basic().SupplyBatteryVoltage == fixed_zero)
      _stprintf(Temp2,_T("\0"));
    else
      _stprintf(Temp2, _T("%.1f V"),
                (double)XCSoarInterface::Basic().SupplyBatteryVoltage);

    _tcscat(Temp, Temp2);

    wp->SetText(Temp);
    wp->RefreshDisplay();
  }
}


static void
UpdateValuesTimes(void)
{
  WndProperty *wp;
  TCHAR Temp[1000];
  double sunsettime;
  int sunsethours;
  int sunsetmins;

  sunsettime = XCSoarInterface::Calculated().TimeSunset;
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime - sunsethours) * 60);

  wp = (WndProperty*)wf->FindByName(_T("prpSunset"));
  if (wp) {
    _stprintf(Temp, _T("%02d:%02d"), sunsethours, sunsetmins);
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpLocalTime"));
  if (wp) {
    Units::TimeToText(Temp, (int)DetectCurrentTime(&XCSoarInterface::Basic()));
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTakeoffTime"));
  if (wp) {
    if (positive(XCSoarInterface::Basic().flight.FlightTime)) {
      Units::TimeToText(Temp,
                        TimeLocal((long)XCSoarInterface::Basic().flight.TakeOffTime));
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpLandingTime"));
  if (wp) {
    if (!XCSoarInterface::Basic().flight.Flying &&
        positive(XCSoarInterface::Basic().flight.FlightTime)) {
      Units::TimeToText(Temp,
                        TimeLocal((long)(XCSoarInterface::Basic().flight.TakeOffTime
                                              + XCSoarInterface::Basic().flight.FlightTime)));
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFlightTime"));
  if (wp) {
    if (positive(XCSoarInterface::Basic().flight.FlightTime)) {
      Units::TimeToText(Temp, (int)XCSoarInterface::Basic().flight.FlightTime);
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }
}

static const Waypoint* nearest_waypoint;

static void
UpdateValuesFlight(void)
{
  WndProperty *wp;
  TCHAR Temp[1000];
  TCHAR sLongitude[16];
  TCHAR sLatitude[16];

  Units::LongitudeToString(XCSoarInterface::Basic().Location.Longitude,
                           sLongitude, sizeof(sLongitude)-1);
  Units::LatitudeToString(XCSoarInterface::Basic().Location.Latitude,
                          sLatitude, sizeof(sLatitude)-1);

  wp = (WndProperty*)wf->FindByName(_T("prpLongitude"));
  if (wp)
    wp->SetText(sLongitude);

  wp = (WndProperty*)wf->FindByName(_T("prpLatitude"));
  if (wp)
    wp->SetText(sLatitude);

  wp = (WndProperty*)wf->FindByName(_T("prpAltitude"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(XCSoarInterface::Basic().GPSAltitude),
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMaxHeightGain"));
  if (wp) {
    _stprintf(Temp, _T("%d %s"),
              (int)Units::ToUserAltitude(XCSoarInterface::Calculated().MaxHeightGain),
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }

  if (nearest_waypoint) {
    GeoVector vec(XCSoarInterface::Basic().Location,
                  nearest_waypoint->Location);

    wp = (WndProperty*)wf->FindByName(_T("prpNear"));
    if (wp)
      wp->SetText(nearest_waypoint->Name.c_str());

    wp = (WndProperty*)wf->FindByName(_T("prpBearing"));
    if (wp) {
      _stprintf(Temp, _T("%d")_T(DEG), (int)vec.Bearing.value_degrees());
      wp->SetText(Temp);
    }

    wp = (WndProperty*)wf->FindByName(_T("prpDistance"));
    if (wp) {
      TCHAR DistanceText[MAX_PATH];
      Units::FormatUserDistance(vec.Distance, DistanceText, 10);
      wp->SetText(DistanceText);
    }
  } else {
    wp = (WndProperty*)wf->FindByName(_T("prpNear"));
    if (wp)
      wp->SetText(_T("-"));

    wp = (WndProperty*)wf->FindByName(_T("prpBearing"));
    if (wp)
      wp->SetText(_T("-"));

    wp = (WndProperty*)wf->FindByName(_T("prpDistance"));
    if (wp)
      wp->SetText(_T("-"));
  }
}

static void
UpdateValuesRules(void)
{
  WndProperty *wp;
  TCHAR Temp[80];

  wp = (WndProperty*)wf->FindByName(_T("prpValidStart"));
  if (wp) {
    if (XCSoarInterface::Calculated().common_stats.task_started)
      /// @todo proper task validity check
      wp->SetText(_("TRUE"));
    else
      wp->SetText(_("FALSE"));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpValidFinish"));
  if (wp) {
    if (XCSoarInterface::Calculated().common_stats.task_finished)
      wp->SetText(_("TRUE"));
    else
      wp->SetText(_("FALSE"));
  }

  AIRCRAFT_STATE start_state = protected_task_manager.get_start_state();

  wp = (WndProperty*)wf->FindByName(_T("prpStartTime"));
  if (wp) {
    if (XCSoarInterface::Calculated().common_stats.task_started) {
      Units::TimeToText(Temp, (int)TimeLocal(start_state.Time));
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartSpeed"));
  if (wp) {
    if (XCSoarInterface::Calculated().common_stats.task_started) {
      _stprintf(Temp, _T("%d %s"),
                (int)Units::ToUserTaskSpeed(start_state.Speed),
                Units::GetTaskSpeedName());
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }
  // StartMaxHeight, StartMaxSpeed;

  wp = (WndProperty*)wf->FindByName(_T("prpStartHeight"));
  if (wp) {
    if (XCSoarInterface::Calculated().common_stats.task_started) {
      _stprintf(Temp, _T("%.0f %s"),
                (double)Units::ToUserAltitude(start_state.NavAltitude),
                Units::GetAltitudeName());
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFinishAlt"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(protected_task_manager.get_finish_height()),
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }

#ifdef OLD_TASK
  wp = (WndProperty*)wf->FindByName(_T("prpStartPoint"));
  if (wp) {
    int wp_index = task.getWaypointIndex(0);
    if (wp_index>=0) {
      wp->SetText(way_points.get(wp_index).Name);
    } else {
      wp->SetText(_T(""));
    }
  }
#endif
}

static void
UpdateValuesTask(void)
{
  WndProperty *wp;
  TCHAR Temp[80];

  wp = (WndProperty*)wf->FindByName(_T("prpTaskTime"));
  Units::TimeToText(Temp, protected_task_manager.get_ordered_task_behaviour().aat_min_time);
  if (wp) {
    if (XCSoarInterface::Calculated().task_stats.has_targets)
      wp->SetText(Temp);
    else
      wp->hide();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpETETime"));
  if (wp) {
    Units::TimeToText(Temp, 
                      XCSoarInterface::Calculated().task_stats.total.TimeElapsed
                      +XCSoarInterface::Calculated().task_stats.total.TimeRemaining);
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRemainingTime"));
  if (wp) {
    Units::TimeToText(Temp, 
                      XCSoarInterface::Calculated().task_stats.total.TimeRemaining);
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskDistance"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"), (double)Units::ToUserDistance(
              XCSoarInterface::Calculated().task_stats.total.planned.get_distance()),
              Units::GetDistanceName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRemainingDistance"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"), (double)Units::ToUserDistance(
              XCSoarInterface::Calculated().task_stats.total.remaining.get_distance()),
              Units::GetDistanceName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpEstimatedSpeed"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"), (double)Units::ToUserTaskSpeed(
              XCSoarInterface::Calculated().task_stats.total.planned.get_speed()),
              Units::GetTaskSpeedName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAverageSpeed"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"), (double)Units::ToUserTaskSpeed(
              XCSoarInterface::Calculated().task_stats.total.travelled.get_speed()),
              Units::GetTaskSpeedName());
    wp->SetText(Temp);
  }
}

static int
OnTimerNotify(WindowControl * Sender)
{
  (void)Sender;
  UpdateValuesSystem();
  return 0;
}

void
dlgStatusShowModal(int start_page)
{
  multi_page = (start_page == -1);

  if (!multi_page)
    status_page = start_page;

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  _T("IDR_XML_STATUS"));
  if (!wf)
    return;

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  tabbed = ((TabbedControl *)wf->FindByName(_T("tabbed")));
  assert(tabbed != NULL);

  /* restore previous page */
  tabbed->SetCurrentPage(status_page);

  wf->SetTimerNotify(OnTimerNotify);

  if (!multi_page) {
    WndButton *wb;
    wb = ((WndButton *)wf->FindByName(_T("cmdNext")));
    if (wb != NULL)
      wb->hide();

    wb = ((WndButton *)wf->FindByName(_T("cmdPrev")));
    if (wb != NULL)
      wb->hide();
  }

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

  wf = NULL;
}
