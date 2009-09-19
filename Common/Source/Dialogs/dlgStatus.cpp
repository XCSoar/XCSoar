/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include "Settings.hpp"
#include "SettingsTask.hpp"
#include "Math/Earth.hpp"
#include "Battery.h"
#include "Units.hpp"
#include "Waypointparser.h"
#include "Logger.h"
#include "Math/FastMath.h"
#include "LocalTime.hpp"
#include "MainWindow.hpp"
#include "Calculations.h" // TODO danger! FAIFinishHeight
#include "MapWindow.h"
#include "GlideComputer.hpp"
#include "WayPointList.hpp"
#include "Components.hpp"

#include <assert.h>

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
    wf->SetCaption(gettext(TEXT("Status: Aircraft")));
    break;
  case 1:
    wf->SetCaption(gettext(TEXT("Status: System")));
    break;
  case 2:
    wf->SetCaption(gettext(TEXT("Status: Task")));
    break;
  case 3:
    wf->SetCaption(gettext(TEXT("Status: Rules")));
    break;
  case 4:
    wf->SetCaption(gettext(TEXT("Status: Times")));
    break;
  }
  wStatus0->SetVisible(status_page == 0);
  wStatus1->SetVisible(status_page == 1);
  wStatus2->SetVisible(status_page == 2);
  wStatus3->SetVisible(status_page == 3);
  wStatus4->SetVisible(status_page == 4);
}


static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}


static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
	(void)lParam;
	(void)Sender;
  switch(wParam & 0xffff){
    case VK_LEFT:
    case '6':
      ((WndButton *)wf->FindByName(TEXT("cmdPrev")))->set_focus();
      NextPage(-1);
      //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
    case VK_RIGHT:
    case '7':
      ((WndButton *)wf->FindByName(TEXT("cmdNext")))->set_focus();
      NextPage(+1);
      //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return(0);
  }
  return(1);
}

static void OnNextClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(-1);
}


//////////////

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
  static bool LoggerActive_last = isLoggerActive();
  static bool DeclaredToDevice_last = isTaskDeclared();
  static double SupplyBatteryVoltage_last = XCSoarInterface::Basic().SupplyBatteryVoltage;
  static int PDABatteryPercent_last = PDABatteryPercent;

  if (first ||
      (extGPSCONNECT_last != XCSoarInterface::Basic().Connected) ||
      (NAVWarning_last != XCSoarInterface::Basic().NAVWarning) ||
      (SatellitesUsed_last != XCSoarInterface::Basic().SatellitesUsed) ||
      (VarioAvailable_last != XCSoarInterface::Basic().VarioAvailable) ||
      (FLARM_Available_last != XCSoarInterface::Basic().FLARM_Available) ||
      (LoggerActive_last != isLoggerActive()) ||
      (DeclaredToDevice_last != isTaskDeclared()) ||
      (SupplyBatteryVoltage_last != XCSoarInterface::Basic().SupplyBatteryVoltage) ||
      (PDABatteryPercent_last != PDABatteryPercent)) {
    first = false;

    extGPSCONNECT_last = XCSoarInterface::Basic().Connected;
    NAVWarning_last = XCSoarInterface::Basic().NAVWarning;
    SatellitesUsed_last = XCSoarInterface::Basic().SatellitesUsed;
    VarioAvailable_last = XCSoarInterface::Basic().VarioAvailable;
    FLARM_Available_last = XCSoarInterface::Basic().FLARM_Available;
    LoggerActive_last = isLoggerActive();
    DeclaredToDevice_last = isTaskDeclared();
    SupplyBatteryVoltage_last = XCSoarInterface::Basic().SupplyBatteryVoltage;
    PDABatteryPercent_last = PDABatteryPercent;

  } else {
    return;
  }

  TCHAR Temp[80];
  TCHAR Temp2[80];

  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpGPS"));
  if (wp) {
    if (XCSoarInterface::Basic().Connected) {
      if (XCSoarInterface::Basic().NAVWarning) {
        wp->SetText(gettext(TEXT("Fix invalid")));
      } else {
        if (XCSoarInterface::Basic().SatellitesUsed==0) {
          wp->SetText(gettext(TEXT("No fix")));
        } else {
          wp->SetText(gettext(TEXT("3D fix")));
        }
      }
      wp->RefreshDisplay();

      wp = (WndProperty*)wf->FindByName(TEXT("prpNumSat"));
      if (wp) {
        if (XCSoarInterface::Basic().SatellitesUsed >= 0) {  // known numer of sats
          _stprintf(Temp,TEXT("%d"),XCSoarInterface::Basic().SatellitesUsed);
        } else { // valid but unknown number of sats
          _stprintf(Temp,TEXT(">3"));
        }
        wp->SetText(Temp);
        wp->RefreshDisplay();
      }
    } else {
      wp->SetText(gettext(TEXT("Disconnected")));
      wp->RefreshDisplay();
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVario"));
  if (wp) {
    if (XCSoarInterface::Basic().VarioAvailable) {
      wp->SetText(gettext(TEXT("Connected")));
    } else {
      wp->SetText(gettext(TEXT("Disconnected")));
    }
    wp->RefreshDisplay();
  }

  if (wp) {
    wp = (WndProperty*)wf->FindByName(TEXT("prpFLARM"));
    if (XCSoarInterface::Basic().FLARM_Available) {
      wp->SetText(gettext(TEXT("Connected")));
    } else {
      wp->SetText(gettext(TEXT("Disconnected")));
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLogger"));
  if (wp) {
    LinkGRecordDLL();
    if (LoggerGActive()) {
      if (isLoggerActive()) {
        wp->SetText(gettext(TEXT("ON (G)")));
      } else {
        wp->SetText(gettext(TEXT("OFF (G)")));
      }
    }
    else { // no G Record
      if (isLoggerActive()) {
        wp->SetText(gettext(TEXT("ON (no G)")));
      } else {
        wp->SetText(gettext(TEXT("OFF (no G)")));
      }
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpDeclared"));
  if (wp) {
    if (isTaskDeclared()) {
      wp->SetText(gettext(TEXT("YES")));
    } else {
      wp->SetText(gettext(TEXT("NO")));
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpBattery"));
  if (wp) {
    _stprintf(Temp,TEXT("\0"));
#ifndef WINDOWSPC
#ifndef GNAV
    _stprintf(Temp2,TEXT("%d%% "), PDABatteryPercent);
    _tcscat(Temp, Temp2);
#endif
#endif
    if (XCSoarInterface::Basic().SupplyBatteryVoltage == 0) {
      _stprintf(Temp2,TEXT("\0"));
    } else {
      _stprintf(Temp2,TEXT("%.1f V"),XCSoarInterface::Basic().SupplyBatteryVoltage);
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpSunset"));
  if (wp) {
    _stprintf(Temp, TEXT("%02d:%02d"), sunsethours,sunsetmins);
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLocalTime"));
  if (wp) {
    Units::TimeToText(Temp, (int)DetectCurrentTime(&XCSoarInterface::Basic()));
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTakeoffTime"));
  if (wp) {
    if (XCSoarInterface::Calculated().FlightTime>0) {
      Units::TimeToText(Temp,
                        (int)TimeLocal((long)XCSoarInterface::Calculated().TakeOffTime));
      wp->SetText(Temp);
    } else {
      wp->SetText(TEXT(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLandingTime"));
  if (wp) {
    if (!XCSoarInterface::Calculated().Flying) {
      Units::TimeToText(Temp,
                        (int)TimeLocal((long)(XCSoarInterface::Calculated().TakeOffTime
                                              +XCSoarInterface::Calculated().FlightTime)));
      wp->SetText(Temp);
    } else {
      wp->SetText(TEXT(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFlightTime"));
  if (wp) {
    if (XCSoarInterface::Calculated().FlightTime > 0){
      Units::TimeToText(Temp, (int)XCSoarInterface::Calculated().FlightTime);
      wp->SetText(Temp);
    } else {
      wp->SetText(TEXT(""));
    }
  }

}

static int nearest_waypoint= -1;


static void UpdateValuesFlight(void) {
  WndProperty *wp;
  TCHAR Temp[1000];
  double bearing;
  double distance;
  TCHAR sLongitude[16];
  TCHAR sLatitude[16];

  Units::LongitudeToString(XCSoarInterface::Basic().Location.Longitude, 
                           sLongitude, sizeof(sLongitude)-1);
  Units::LatitudeToString(XCSoarInterface::Basic().Location.Latitude, 
                          sLatitude, sizeof(sLatitude)-1);

  wp = (WndProperty*)wf->FindByName(TEXT("prpLongitude"));
  if (wp) {
    wp->SetText(sLongitude);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLatitude"));
  if (wp) {
    wp->SetText(sLatitude);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
  if (wp) {
    _stprintf(Temp, TEXT("%.0f %s"),
              XCSoarInterface::Basic().Altitude*ALTITUDEMODIFY,
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMaxHeightGain"));
  if (wp) {
    _stprintf(Temp, TEXT("%.0f %s"),
              XCSoarInterface::Calculated().MaxHeightGain*ALTITUDEMODIFY,
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }

  if (nearest_waypoint>=0) {

    DistanceBearing(XCSoarInterface::Basic().Location,
                    way_points.get(nearest_waypoint).Location,
                    &distance,
                    &bearing);

    wp = (WndProperty*)wf->FindByName(TEXT("prpNear"));
    if (wp) {
      wp->SetText(way_points.get(nearest_waypoint).Name);
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpBearing"));
    if (wp) {
      _stprintf(Temp, TEXT("%d")TEXT(DEG), iround(bearing));
      wp->SetText(Temp);
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpDistance"));
    if (wp) {
      TCHAR DistanceText[MAX_PATH];
      Units::FormatUserDistance(distance,DistanceText, 10);
      wp->SetText(DistanceText);
    }

  } else {
    wp = (WndProperty*)wf->FindByName(TEXT("prpNear"));
    if (wp) {
      wp->SetText(TEXT("-"));
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpBearing"));
    if (wp) {
      wp->SetText(TEXT("-"));
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpDistance"));
    if (wp) {
      wp->SetText(TEXT("-"));
    }
  }

}


static void UpdateValuesRules(void) {
  WndProperty *wp;
  TCHAR Temp[80];

  wp = (WndProperty*)wf->FindByName(TEXT("prpValidStart"));
  if (wp) {
    if (XCSoarInterface::Calculated().ValidStart) {
      wp->SetText(gettext(TEXT("TRUE")));
    } else {
      wp->SetText(gettext(TEXT("FALSE")));
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpValidFinish"));
  if (wp) {
    if (XCSoarInterface::Calculated().ValidFinish) {
      wp->SetText(gettext(TEXT("TRUE")));
    } else {
      wp->SetText(gettext(TEXT("FALSE")));
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartTime"));
  if (wp) {
    if (XCSoarInterface::Calculated().TaskStartTime>0) {
      Units::TimeToText(Temp, (int)TimeLocal((int)(XCSoarInterface::Calculated().TaskStartTime)));
      wp->SetText(Temp);
    } else {
      wp->SetText(TEXT(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartSpeed"));
  if (wp) {
    if (XCSoarInterface::Calculated().TaskStartTime>0) {
      _stprintf(Temp, TEXT("%.0f %s"),
                TASKSPEEDMODIFY*XCSoarInterface::Calculated().TaskStartSpeed,
                Units::GetTaskSpeedName());
      wp->SetText(Temp);
    } else {
      wp->SetText(TEXT(""));
    }
  }
  // StartMaxHeight, StartMaxSpeed;

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartPoint"));
  if (wp) {
    int wp_index = task.getWaypointIndex(0);
    if (wp_index>=0) {
      wp->SetText(way_points.get(wp_index).Name);
    } else {
      wp->SetText(TEXT(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartHeight"));
  if (wp) {
    if (XCSoarInterface::Calculated().TaskStartTime>0) {
      _stprintf(Temp, TEXT("%.0f %s"),
                (XCSoarInterface::Calculated().TaskStartAltitude)*ALTITUDEMODIFY,
                Units::GetAltitudeName());
      wp->SetText(Temp);
    } else {
      wp->SetText(TEXT(""));
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFinishAlt"));
  if (wp) {
    double finish_min = 
      FAIFinishHeight(XCSoarInterface::SettingsComputer(), 
		      XCSoarInterface::Calculated(), -1);
    _stprintf(Temp, TEXT("%.0f %s"),
              finish_min*ALTITUDEMODIFY,
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }

}


static void UpdateValuesTask(void) {
  WndProperty *wp;
  TCHAR Temp[80];

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskTime"));
  Units::TimeToText(Temp, (int)task.getSettings().AATTaskLength*60);
  if (wp) {
    if (!task.getSettings().AATEnabled) {
      wp->SetVisible(false);
    } else {
      wp->SetText(Temp);
    }
  }

  double dd = XCSoarInterface::Calculated().TaskTimeToGo;
  if (XCSoarInterface::Calculated().TaskStartTime>0.0) {
    dd += XCSoarInterface::Basic().Time-XCSoarInterface::Calculated().TaskStartTime;
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpETETime"));
  if (wp) {
    Units::TimeToText(Temp, (int)dd);
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpRemainingTime"));
  if (wp) {
    Units::TimeToText(Temp, (int)XCSoarInterface::Calculated().TaskTimeToGo);
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskDistance"));
  if (wp) {
    _stprintf(Temp, TEXT("%.0f %s"), DISTANCEMODIFY*
              (XCSoarInterface::Calculated().TaskDistanceToGo
               +XCSoarInterface::Calculated().TaskDistanceCovered),
              Units::GetDistanceName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpRemainingDistance"));
  if (wp) {
    if (task.getSettings().AATEnabled) {
      _stprintf(Temp, TEXT("%.0f %s"),
                DISTANCEMODIFY*XCSoarInterface::Calculated().AATTargetDistance,
                Units::GetDistanceName());
    } else {
      _stprintf(Temp, TEXT("%.0f %s"),
                DISTANCEMODIFY*XCSoarInterface::Calculated().TaskDistanceToGo,
                Units::GetDistanceName());
    }
    wp->SetText(Temp);
  }

  double d1 =
    (XCSoarInterface::Calculated().TaskDistanceToGo
     +XCSoarInterface::Calculated().TaskDistanceCovered)/dd;
  // TODO bug: this fails for OLC

  wp = (WndProperty*)wf->FindByName(TEXT("prpEstimatedSpeed"));
  if (wp) {
    _stprintf(Temp, TEXT("%.0f %s"),
              TASKSPEEDMODIFY*d1, Units::GetTaskSpeedName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAverageSpeed"));
  if (wp) {
    _stprintf(Temp, TEXT("%.0f %s"),
              TASKSPEEDMODIFY*XCSoarInterface::Calculated().TaskSpeed,
              Units::GetTaskSpeedName());
    wp->SetText(Temp);
  }
}


static int OnTimerNotify(WindowControl * Sender) {
  (void)Sender;

  UpdateValuesSystem();

  return 0;
}


//////////////

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
                      TEXT("dlgStatus.xml"),
		      XCSoarInterface::main_window,
		      TEXT("IDR_XML_STATUS"));

  if (!wf) return;

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wStatus0    = ((WndFrame *)wf->FindByName(TEXT("frmStatusFlight")));
  wStatus1    = ((WndFrame *)wf->FindByName(TEXT("frmStatusSystem")));
  wStatus2    = ((WndFrame *)wf->FindByName(TEXT("frmStatusTask")));
  wStatus3    = ((WndFrame *)wf->FindByName(TEXT("frmStatusRules")));
  wStatus4    = ((WndFrame *)wf->FindByName(TEXT("frmStatusTimes")));

  assert(wStatus0!=NULL);
  assert(wStatus1!=NULL);
  assert(wStatus2!=NULL);
  assert(wStatus3!=NULL);
  assert(wStatus4!=NULL);

  wf->SetTimerNotify(OnTimerNotify);

  if (!multi_page) {
    WndButton *wb;
    wb = ((WndButton *)wf->FindByName(TEXT("cmdNext")));
    if (wb != NULL) {
      wb->SetVisible(false);
    }
    wb = ((WndButton *)wf->FindByName(TEXT("cmdPrev")));
    if (wb != NULL) {
      wb->SetVisible(false);
    }
  }

  nearest_waypoint = FindNearestWayPoint(way_points,
                                         XCSoarInterface::main_window.map,
					 XCSoarInterface::Basic().Location,
                                         100000.0, true); // big range limit

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

