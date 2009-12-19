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
#include "SettingsTask.hpp"
#include "Math/Earth.hpp"
#include "Battery.h"
#include "Units.hpp"
#include "Logger.h"
#include "Math/FastMath.h"
#include "LocalTime.hpp"
#include "MainWindow.hpp"
#include "Calculations.h" // TODO danger! FAIFinishHeight
#include "MapWindow.h"
#include "GlideComputer.hpp"
#include "Components.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

#ifdef OLD_TASK
#include "Task.h"
#endif

#include <assert.h>

#ifndef _MSC_VER
#include <algorithm>
using std::min;
using std::max;
#endif

static WndForm *wf=NULL;
static bool multi_page = false;
static int status_page = 0;
static WndFrame *wStatus0=NULL;
static WndFrame *wStatus1=NULL;
static WndFrame *wStatus2=NULL;
static WndFrame *wStatus3=NULL;
static WndFrame *wStatus4=NULL;

#define NUMPAGES 5

static void NextPage(int Step){
  status_page += Step;
  if (status_page>=NUMPAGES) { status_page=0; }
  if (status_page<0) { status_page=NUMPAGES-1; }
  switch(status_page) {
  case 0:
    wf->SetCaption(gettext(_T("Status: Aircraft")));
    break;
  case 1:
    wf->SetCaption(gettext(_T("Status: System")));
    break;
  case 2:
    wf->SetCaption(gettext(_T("Status: Task")));
    break;
  case 3:
    wf->SetCaption(gettext(_T("Status: Rules")));
    break;
  case 4:
    wf->SetCaption(gettext(_T("Status: Times")));
    break;
  }
  wStatus0->set_visible(status_page == 0);
  wStatus1->set_visible(status_page == 1);
  wStatus2->set_visible(status_page == 2);
  wStatus3->set_visible(status_page == 3);
  wStatus4->set_visible(status_page == 4);
}


static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
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
      NextPage(-1);
      //((WndButton *)wf->FindByName(_T("cmdPrev")))->SetFocused(true, NULL);
    return true;

    case VK_RIGHT:
    case '7':
      ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
      NextPage(+1);
      //((WndButton *)wf->FindByName(_T("cmdNext")))->SetFocused(true, NULL);
    return true;

  default:
    return false;
  }
}

static void OnNextClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(-1);
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(NULL)
};

static bool first = true;

static void UpdateValuesSystem() {
  static unsigned extGPSCONNECT_last = XCSoarInterface::Basic().Connected;
  static int NAVWarning_last = XCSoarInterface::Basic().NAVWarning;
  static int SatellitesUsed_last = XCSoarInterface::Basic().SatellitesUsed;
  static int VarioAvailable_last = XCSoarInterface::Basic().VarioAvailable;
  static int FLARM_Available_last = XCSoarInterface::Basic().FLARM_Available;
  static bool LoggerActive_last = logger.isLoggerActive();
  static bool DeclaredToDevice_last = logger.isTaskDeclared();
  static double SupplyBatteryVoltage_last = XCSoarInterface::Basic().SupplyBatteryVoltage;
  static int PDABatteryPercent_last = PDABatteryPercent;

  if (first ||
      (extGPSCONNECT_last != XCSoarInterface::Basic().Connected) ||
      (NAVWarning_last != XCSoarInterface::Basic().NAVWarning) ||
      (SatellitesUsed_last != XCSoarInterface::Basic().SatellitesUsed) ||
      (VarioAvailable_last != XCSoarInterface::Basic().VarioAvailable) ||
      (FLARM_Available_last != XCSoarInterface::Basic().FLARM_Available) ||
      (LoggerActive_last != logger.isLoggerActive()) ||
      (DeclaredToDevice_last != logger.isTaskDeclared()) ||
      (SupplyBatteryVoltage_last != XCSoarInterface::Basic().SupplyBatteryVoltage) ||
      (PDABatteryPercent_last != PDABatteryPercent)) {
    first = false;

    extGPSCONNECT_last = XCSoarInterface::Basic().Connected;
    NAVWarning_last = XCSoarInterface::Basic().NAVWarning;
    SatellitesUsed_last = XCSoarInterface::Basic().SatellitesUsed;
    VarioAvailable_last = XCSoarInterface::Basic().VarioAvailable;
    FLARM_Available_last = XCSoarInterface::Basic().FLARM_Available;
    LoggerActive_last = logger.isLoggerActive();
    DeclaredToDevice_last = logger.isTaskDeclared();
    SupplyBatteryVoltage_last = XCSoarInterface::Basic().SupplyBatteryVoltage;
    PDABatteryPercent_last = PDABatteryPercent;

  } else {
    return;
  }

  TCHAR Temp[80];
  TCHAR Temp2[80];

  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(_T("prpGPS"));
  if (wp) {
    if (XCSoarInterface::Basic().Connected) {
      if (XCSoarInterface::Basic().NAVWarning) {
        wp->SetText(gettext(_T("Fix invalid")));
      } else {
        if (XCSoarInterface::Basic().SatellitesUsed==0) {
          wp->SetText(gettext(_T("No fix")));
        } else {
          wp->SetText(gettext(_T("3D fix")));
        }
      }
      wp->RefreshDisplay();

      wp = (WndProperty*)wf->FindByName(_T("prpNumSat"));
      if (wp) {
        if (XCSoarInterface::Basic().SatellitesUsed >= 0) {  // known numer of sats
          _stprintf(Temp,_T("%d"),XCSoarInterface::Basic().SatellitesUsed);
        } else { // valid but unknown number of sats
          _stprintf(Temp,_T(">3"));
        }
        wp->SetText(Temp);
        wp->RefreshDisplay();
      }
    } else {
      wp->SetText(gettext(_T("Disconnected")));
      wp->RefreshDisplay();
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpVario"));
  if (wp) {
    if (XCSoarInterface::Basic().VarioAvailable) {
      wp->SetText(gettext(_T("Connected")));
    } else {
      wp->SetText(gettext(_T("Disconnected")));
    }
    wp->RefreshDisplay();
  }

  if (wp) {
    wp = (WndProperty*)wf->FindByName(_T("prpFLARM"));
    if (XCSoarInterface::Basic().FLARM_Available) {
      wp->SetText(gettext(_T("Connected")));
    } else {
      wp->SetText(gettext(_T("Disconnected")));
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpLogger"));
  if (wp) {
    logger.LinkGRecordDLL();
    if (logger.LoggerGActive()) {
      if (logger.isLoggerActive()) {
        wp->SetText(gettext(_T("ON (G)")));
      } else {
        wp->SetText(gettext(_T("OFF (G)")));
      }
    }
    else { // no G Record
      if (logger.isLoggerActive()) {
        wp->SetText(gettext(_T("ON (no G)")));
      } else {
        wp->SetText(gettext(_T("OFF (no G)")));
      }
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDeclared"));
  if (wp) {
    if (logger.isTaskDeclared()) {
      wp->SetText(gettext(_T("YES")));
    } else {
      wp->SetText(gettext(_T("NO")));
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpBattery"));
  if (wp) {
    _stprintf(Temp,_T("\0"));
#if !defined(GNAV) && !defined(WINDOWSPC) && !defined(HAVE_POSIX)
    _stprintf(Temp2,_T("%d%% "), PDABatteryPercent);
    _tcscat(Temp, Temp2);
#endif
    if (XCSoarInterface::Basic().SupplyBatteryVoltage == 0) {
      _stprintf(Temp2,_T("\0"));
    } else {
      _stprintf(Temp2,_T("%.1f V"),XCSoarInterface::Basic().SupplyBatteryVoltage);
    }
    _tcscat(Temp, Temp2);

    wp->SetText(Temp);
    wp->RefreshDisplay();
  }
}


static void UpdateValuesTimes(void) {
  WndProperty *wp;
  TCHAR Temp[1000];
  double sunsettime;
  int sunsethours;
  int sunsetmins;

  sunsettime = XCSoarInterface::Calculated().TimeSunset;
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime-sunsethours)*60);

  wp = (WndProperty*)wf->FindByName(_T("prpSunset"));
  if (wp) {
    _stprintf(Temp, _T("%02d:%02d"), sunsethours,sunsetmins);
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpLocalTime"));
  if (wp) {
    Units::TimeToText(Temp, (int)DetectCurrentTime(&XCSoarInterface::Basic()));
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTakeoffTime"));
  if (wp) {
    if (XCSoarInterface::Calculated().FlightTime>0) {
      Units::TimeToText(Temp,
                        (int)TimeLocal((long)XCSoarInterface::Calculated().TakeOffTime.as_long()));
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpLandingTime"));
  if (wp) {
    if (!XCSoarInterface::Calculated().Flying) {
      Units::TimeToText(Temp,
                        (int)TimeLocal((long)(XCSoarInterface::Calculated().TakeOffTime
                                              +XCSoarInterface::Calculated().FlightTime).as_long()));
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFlightTime"));
  if (wp) {
    if (XCSoarInterface::Calculated().FlightTime > 0){
      Units::TimeToText(Temp, (int)XCSoarInterface::Calculated().FlightTime);
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }

}

static const Waypoint* nearest_waypoint;

static void UpdateValuesFlight(void) {
  WndProperty *wp;
  TCHAR Temp[1000];
  TCHAR sLongitude[16];
  TCHAR sLatitude[16];

  Units::LongitudeToString(XCSoarInterface::Basic().Location.Longitude,
                           sLongitude, sizeof(sLongitude)-1);
  Units::LatitudeToString(XCSoarInterface::Basic().Location.Latitude,
                          sLatitude, sizeof(sLatitude)-1);

  wp = (WndProperty*)wf->FindByName(_T("prpLongitude"));
  if (wp) {
    wp->SetText(sLongitude);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpLatitude"));
  if (wp) {
    wp->SetText(sLatitude);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAltitude"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"),
              (double)XCSoarInterface::Basic().Altitude*ALTITUDEMODIFY,
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMaxHeightGain"));
  if (wp) {
    _stprintf(Temp, _T("%d %s"),
              (XCSoarInterface::Calculated().MaxHeightGain*ALTITUDEMODIFY).as_int(),
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }

  if (nearest_waypoint) {

    GeoVector vec(XCSoarInterface::Basic().Location, nearest_waypoint->Location);

    wp = (WndProperty*)wf->FindByName(_T("prpNear"));
    if (wp) {
      wp->SetText(nearest_waypoint->Name.c_str());
    }
    wp = (WndProperty*)wf->FindByName(_T("prpBearing"));
    if (wp) {
      _stprintf(Temp, _T("%d")_T(DEG), vec.Bearing.as_int());
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
    if (wp) {
      wp->SetText(_T("-"));
    }
    wp = (WndProperty*)wf->FindByName(_T("prpBearing"));
    if (wp) {
      wp->SetText(_T("-"));
    }
    wp = (WndProperty*)wf->FindByName(_T("prpDistance"));
    if (wp) {
      wp->SetText(_T("-"));
    }
  }
}


static void UpdateValuesRules(void) {
  WndProperty *wp;
  TCHAR Temp[80];

  wp = (WndProperty*)wf->FindByName(_T("prpValidStart"));
  if (wp) {
    if (positive(XCSoarInterface::Calculated().common_stats.task_time_elapsed)) {
      /// \todo proper task validity check
      wp->SetText(gettext(_T("TRUE")));
    } else {
      wp->SetText(gettext(_T("FALSE")));
    }
  }
  wp = (WndProperty*)wf->FindByName(_T("prpValidFinish"));
  if (wp) {
    if (XCSoarInterface::Calculated().common_stats.task_finished) {
      wp->SetText(gettext(_T("TRUE")));
    } else {
      wp->SetText(gettext(_T("FALSE")));
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartTime"));
  if (wp) {
    if (positive(XCSoarInterface::Calculated().common_stats.task_time_elapsed)) {
      fixed the_time = XCSoarInterface::Calculated().task_stats.Time - 
        XCSoarInterface::Calculated().common_stats.task_time_elapsed;
      Units::TimeToText(Temp, (int)TimeLocal(the_time));
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }

#ifdef OLD_TASK

  wp = (WndProperty*)wf->FindByName(_T("prpStartSpeed"));
  if (wp) {
    if (XCSoarInterface::Calculated().TaskStartTime>0) {
      _stprintf(Temp, TEXT("%d %s"),
                (TASKSPEEDMODIFY*XCSoarInterface::Calculated().TaskStartSpeed).as_int(),
                Units::GetTaskSpeedName());
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }
  // StartMaxHeight, StartMaxSpeed;

  wp = (WndProperty*)wf->FindByName(_T("prpStartPoint"));
  if (wp) {
    int wp_index = task.getWaypointIndex(0);
    if (wp_index>=0) {
      wp->SetText(way_points.get(wp_index).Name);
    } else {
      wp->SetText(_T(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartHeight"));
  if (wp) {
    if (XCSoarInterface::Calculated().TaskStartTime>0) {
      _stprintf(Temp, _T("%.0f %s"),
                (XCSoarInterface::Calculated().TaskStartAltitude)*ALTITUDEMODIFY,
                Units::GetAltitudeName());
      wp->SetText(Temp);
    } else {
      wp->SetText(_T(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFinishAlt"));
  if (wp) {
    double finish_min =
      FAIFinishHeight(XCSoarInterface::SettingsComputer(),
		      XCSoarInterface::Calculated(), -1);
    _stprintf(Temp, _T("%.0f %s"),
              finish_min*ALTITUDEMODIFY,
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }
#endif

}


static void UpdateValuesTask(void) {
  WndProperty *wp;
  TCHAR Temp[80];

#ifdef OLD_TASK
  wp = (WndProperty*)wf->FindByName(_T("prpTaskTime"));
  Units::TimeToText(Temp, (int)task.getSettings().AATTaskLength*60);
  if (wp) {
    if (!task.getSettings().AATEnabled) {
      wp->hide();
    } else {
      wp->SetText(Temp);
    }
  }
#endif

  wp = (WndProperty*)wf->FindByName(_T("prpETETime"));
  if (wp) {
    Units::TimeToText(Temp, XCSoarInterface::Calculated().common_stats.task_time_elapsed
                      +XCSoarInterface::Calculated().common_stats.task_time_remaining);
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRemainingTime"));
  if (wp) {
    Units::TimeToText(Temp, XCSoarInterface::Calculated().common_stats.task_time_remaining);
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskDistance"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"), DISTANCEMODIFY*
              XCSoarInterface::Calculated().task_stats.total.planned.get_distance(),
              Units::GetDistanceName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRemainingDistance"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"),
              DISTANCEMODIFY*
              XCSoarInterface::Calculated().task_stats.total.remaining.get_distance(),
              Units::GetDistanceName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpEstimatedSpeed"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"),
              TASKSPEEDMODIFY*
              XCSoarInterface::Calculated().task_stats.total.planned.get_speed(), 
              Units::GetTaskSpeedName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAverageSpeed"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"),
              TASKSPEEDMODIFY*
              XCSoarInterface::Calculated().task_stats.total.travelled.get_speed(), 
              Units::GetTaskSpeedName());
    wp->SetText(Temp);
  }
}


static int OnTimerNotify(WindowControl * Sender) {
  (void)Sender;

  UpdateValuesSystem();

  return 0;
}

void dlgStatusShowModal(int start_page){

  if (start_page==-1) {
    multi_page = true;
    status_page = max(0,min(NUMPAGES-1,status_page));
  } else {
    status_page = start_page;
    multi_page = false;
  }

  first = true;

  wf = dlgLoadFromXML(CallBackTable,
                      _T("dlgStatus.xml"),
		      XCSoarInterface::main_window,
		      _T("IDR_XML_STATUS"));

  if (!wf) return;

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wStatus0    = ((WndFrame *)wf->FindByName(_T("frmStatusFlight")));
  wStatus1    = ((WndFrame *)wf->FindByName(_T("frmStatusSystem")));
  wStatus2    = ((WndFrame *)wf->FindByName(_T("frmStatusTask")));
  wStatus3    = ((WndFrame *)wf->FindByName(_T("frmStatusRules")));
  wStatus4    = ((WndFrame *)wf->FindByName(_T("frmStatusTimes")));

  assert(wStatus0!=NULL);
  assert(wStatus1!=NULL);
  assert(wStatus2!=NULL);
  assert(wStatus3!=NULL);
  assert(wStatus4!=NULL);

  wf->SetTimerNotify(OnTimerNotify);

  if (!multi_page) {
    WndButton *wb;
    wb = ((WndButton *)wf->FindByName(_T("cmdNext")));
    if (wb != NULL) {
      wb->hide();
    }
    wb = ((WndButton *)wf->FindByName(_T("cmdPrev")));
    if (wb != NULL) {
      wb->hide();
    }
  }

  nearest_waypoint = way_points.get_nearest(XCSoarInterface::Basic().Location);

  UpdateValuesSystem();
  UpdateValuesFlight();
  UpdateValuesTask();
  UpdateValuesRules();
  UpdateValuesTimes();

  NextPage(0); // just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  wf = NULL;

}

