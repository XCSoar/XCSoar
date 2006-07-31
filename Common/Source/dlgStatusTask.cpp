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
#if (NEWINFOBOX>0)

#include "stdafx.h"

#include "externs.h"
#include "units.h"
#include "externs.h"
#include "Waypointparser.h"

#include "WindowControls.h"
#include "dlgTools.h"
#include "Process.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static int OnFormLButtonUp(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  static int nignore = 0;
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


void dlgStatusTaskShowModal(void){

  WndProperty *wp;

  TCHAR Temp[1000];

  wf = dlgLoadFromXML(NULL, LocalPathS(TEXT("dlgStatusTask.xml")), hWndMainWindow,
		      TEXT("IDR_XML_STATUSTASK"));
  if (!wf) return;

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wf->SetLButtonUpNotify(OnFormLButtonUp);

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskTime"));
  Units::TimeToText(Temp, (int)AATTaskLength*60);
  wp->SetText(Temp);

  double dd = CALCULATED_INFO.TaskTimeToGo;
  if (CALCULATED_INFO.TaskStartTime>0.0) {
    dd += GPS_INFO.Time-CALCULATED_INFO.TaskStartTime;
  }
  
  wp = (WndProperty*)wf->FindByName(TEXT("prpETETime"));
  Units::TimeToText(Temp, (int)dd);
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(TEXT("prpRemainingTime"));
  Units::TimeToText(Temp, (int)CALCULATED_INFO.TaskTimeToGo);
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartTime"));
  if (CALCULATED_INFO.ValidStart) {
    if (CALCULATED_INFO.TaskStartTime>0) {
      Units::TimeToText(Temp, (int)TimeLocal((int)(CALCULATED_INFO.TaskStartTime)));
    } else {
      Units::TimeToText(Temp, 0);
    }
    wp->SetText(Temp);
  } else {
    wp->SetText(TEXT("INVALID"));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskDistance"));
  _stprintf(Temp, TEXT("%.0f %s"), DISTANCEMODIFY*
	    (CALCULATED_INFO.TaskDistanceToGo
	    +CALCULATED_INFO.TaskDistanceCovered), 
	    Units::GetDistanceName());
  wp->SetText(Temp);

  wp = (WndProperty*)wf->FindByName(TEXT("prpRemainingDistance"));
  if (AATEnabled) {
    _stprintf(Temp, TEXT("%.0f %s"), 
	      DISTANCEMODIFY*CALCULATED_INFO.AATTargetDistance, 
	      Units::GetDistanceName());
  } else {
    _stprintf(Temp, TEXT("%.0f %s"), 
	      DISTANCEMODIFY*CALCULATED_INFO.TaskDistanceToGo, 
	      Units::GetDistanceName());
  }
  wp->SetText(Temp);

  double d1 = 
    (CALCULATED_INFO.TaskDistanceToGo
     +CALCULATED_INFO.TaskDistanceCovered)/dd;
  // JMW this fails for OLC

  wp = (WndProperty*)wf->FindByName(TEXT("prpEstimatedSpeed"));
  _stprintf(Temp, TEXT("%.0f %s"), 
	    TASKSPEEDMODIFY*d1, Units::GetTaskSpeedName());
  wp->SetText(Temp);
  
  wp = (WndProperty*)wf->FindByName(TEXT("prpAverageSpeed"));
  _stprintf(Temp, TEXT("%.0f %s"), 
	    TASKSPEEDMODIFY*CALCULATED_INFO.TaskSpeed, 
	    Units::GetTaskSpeedName());
  wp->SetText(Temp);

  wf->ShowModal();

  delete wf;

  wf = NULL;

}



#endif
