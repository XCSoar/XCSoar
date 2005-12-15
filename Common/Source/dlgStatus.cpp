/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"

#include "externs.h"
#include "units.h"
#include "externs.h"
#include "Waypointparser.h"


#include "WindowControls.h"
#include "dlgTools.h"

extern HWND   hWndMainWindow;
extern BOOL extGPSCONNECT;
static WndForm *wf=NULL;

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static int OnFormLButtonUp(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  wf->SetModalResult(mrOK);
  return(0);
}

void dlgStatusShowModal(void){

  WndProperty *wp;

  TCHAR Temp[1000];
  int iwaypoint= -1;
  double sunsettime;
  int sunsethours;
  int sunsetmins;
  double bearing;
  double distance;
  TCHAR sLongitude[16];
  TCHAR sLatitude[16];


  wf = dlgLoadFromXML(NULL, "\\NOR Flash\\dlgStatus.xml", hWndMainWindow);
  wf->SetLButtonUpNotify(OnFormLButtonUp);

  Units::LongitudeToString(GPS_INFO.Longitude, sLongitude, sizeof(sLongitude)-1);
  Units::LatitudeToString(GPS_INFO.Latitude, sLatitude, sizeof(sLatitude)-1);

  sunsettime = DoSunEphemeris(GPS_INFO.Longitude,
                              GPS_INFO.Latitude);
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime-sunsethours)*60);

  wp = (WndProperty*)wf->FindByName(TEXT("prpLongitude"));
  wp->SetText(sLongitude);

  wp = (WndProperty*)wf->FindByName(TEXT("prpLatitude"));
  wp->SetText(sLatitude);

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
  _stprintf(Temp, TEXT("%.0f %s"), GPS_INFO.Altitude*ALTITUDEMODIFY, Units::GetAltitudeName());
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(TEXT("prpSunset"));
  _stprintf(Temp, TEXT("%02d:%02d"), sunsethours,sunsetmins);
  wp->SetText(Temp);

  iwaypoint = FindNearestWayPoint(GPS_INFO.Longitude,
                                  GPS_INFO.Latitude,
                                  100000.0); // big range limit
  if (iwaypoint>=0) {

    bearing = Bearing(GPS_INFO.Latitude,
                      GPS_INFO.Longitude,
                      WayPointList[iwaypoint].Latitude,
                      WayPointList[iwaypoint].Longitude);

    distance = Distance(GPS_INFO.Latitude,
                        GPS_INFO.Longitude,
                        WayPointList[iwaypoint].Latitude,
                        WayPointList[iwaypoint].Longitude)*DISTANCEMODIFY;

    wp = (WndProperty*)wf->FindByName(TEXT("prpNear"));
    wp->SetText(WayPointList[iwaypoint].Name);

    wp = (WndProperty*)wf->FindByName(TEXT("prpBearing"));
    _stprintf(Temp, TEXT("%d"), (int)(bearing+0.5));
    wp->SetText(Temp);

    wp = (WndProperty*)wf->FindByName(TEXT("prpDistance"));
    _stprintf(Temp, TEXT("%.1f %s"), distance, Units::GetDistanceName());
    wp->SetText(Temp);

  } else {
    wp = (WndProperty*)wf->FindByName(TEXT("prpNear"));
    wp->SetText(TEXT("-"));
    wp = (WndProperty*)wf->FindByName(TEXT("prpBearing"));
    wp->SetText(TEXT("-"));
    wp = (WndProperty*)wf->FindByName(TEXT("prpDistance"));
    wp->SetText(TEXT("-"));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpGPS"));
  if (extGPSCONNECT) {
    if (GPS_INFO.NAVWarning) {
      wp->SetText(gettext(TEXT("fix invalid")));
    } else {
      wp->SetText(gettext(TEXT("3D fix")));
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpNumSat"));
    _stprintf(Temp,TEXT("%d"),GPS_INFO.SatellitesUsed);
    wp->SetText(Temp);

  } else {
    wp->SetText(gettext(TEXT("disconnected")));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVario"));
  if (GPS_INFO.VarioAvailable) {
    wp->SetText(gettext(TEXT("connected")));
  } else {
    wp->SetText(gettext(TEXT("disconnected")));
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpLogger"));
  if (LoggerActive) {
    wp->SetText(gettext(TEXT("ON")));
  } else {
    wp->SetText(gettext(TEXT("OFF")));
  }

  wf->ShowModal();

  delete wf;

  wf = NULL;

}



