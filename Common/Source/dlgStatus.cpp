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
#include "Process.h"


#include "WindowControls.h"
#include "dlgTools.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static int OnFormLButtonUp(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  static int nignore = 0;
  (void)lParam;
  (void)wParam;
  (void)Sender;
#ifndef GNAV
  if (nignore) {
    nignore = 0;
    wf->SetModalResult(mrOK);
    return(0);
  }
  nignore++;
#else
  wf->SetModalResult(mrOK);
#endif
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

  char filename[MAX_PATH];
  LocalPathS(filename, TEXT("dlgStatusAircraft.xml"));
  wf = dlgLoadFromXML(NULL, 
                      filename, 
		      hWndMainWindow,
		      TEXT("IDR_XML_STATUSAIRCRAFT"));

  if (!wf) return;

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wf->SetLButtonUpNotify(OnFormLButtonUp);

  Units::LongitudeToString(GPS_INFO.Longitude, sLongitude, sizeof(sLongitude)-1);
  Units::LatitudeToString(GPS_INFO.Latitude, sLatitude, sizeof(sLatitude)-1);

  sunsettime = DoSunEphemeris(GPS_INFO.Longitude,
                              GPS_INFO.Latitude);
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime-sunsethours)*60);

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
    _stprintf(Temp, TEXT("%.0f %s"), GPS_INFO.Altitude*ALTITUDEMODIFY, Units::GetAltitudeName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSunset"));
  if (wp) {
    _stprintf(Temp, TEXT("%02d:%02d"), sunsethours,sunsetmins);
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLocalTime"));
  if (wp) {
    Units::TimeToText(Temp, (int)DetectCurrentTime());
    wp->SetText(Temp);
  }

  iwaypoint = FindNearestWayPoint(GPS_INFO.Longitude,
                                  GPS_INFO.Latitude,
                                  100000.0); // big range limit
  if (iwaypoint>=0) {

    DistanceBearing(GPS_INFO.Latitude,
                    GPS_INFO.Longitude,
                    WayPointList[iwaypoint].Latitude,
                    WayPointList[iwaypoint].Longitude,
                    &distance,
                    &bearing);

    wp = (WndProperty*)wf->FindByName(TEXT("prpNear"));
    if (wp) {
      wp->SetText(WayPointList[iwaypoint].Name);
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpBearing"));
    if (wp) {
      _stprintf(Temp, TEXT("%d°"), iround(bearing));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpFlightTime"));
  if (wp) {

    TCHAR sTakeOff[32];
    TCHAR sLanding[32];
    TCHAR sFlight[32];

    if (CALCULATED_INFO.FlightTime > 0){
      Units::TimeToText(sTakeOff, (int)TimeLocal((long)CALCULATED_INFO.TakeOffTime));
      Units::TimeToText(sLanding, (int)TimeLocal((long)(CALCULATED_INFO.TakeOffTime + CALCULATED_INFO.FlightTime)));
      Units::TimeToText(sFlight, (int)CALCULATED_INFO.FlightTime);

      _tcscpy(Temp, sTakeOff);

      if (!CALCULATED_INFO.Flying){
        _tcscat(Temp, TEXT("-"));
        _tcscat(Temp, sLanding);
      }

      _tcscat(Temp, TEXT("("));
      _tcscat(Temp, sFlight);
      _tcscat(Temp, TEXT(")"));

    } else {
      _stprintf(Temp, TEXT("-"));
    }

    wp->SetText(Temp);
  }

  wf->ShowModal();

  delete wf;

  wf = NULL;

}

