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

#include "statistics.h"

#include "externs.h"
#include "units.h"
#include "McReady.h"
#include "device.h"

#include "WindowControls.h"
#include "dlgTools.h"
#include "Port.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;

static void OnCancelClicked(WindowControl * Sender){
  wf->SetModalResult(mrCancle);
}

static void OnOKClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static double Range = 0;

static void RefreshCalculator(void) {
  WndProperty* wp;

  RefreshTask();
  RefreshTaskStatistics();

  // update outputs
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEst"));
  if (wp) {
    double dd = CALCULATED_INFO.TaskTimeToGo;
    if (CALCULATED_INFO.TaskStartTime>0.0) {
      dd += GPS_INFO.Time-CALCULATED_INFO.TaskStartTime;
    }
    wp->GetDataField()->SetAsFloat(dd/60.0);
    wp->RefreshDisplay();
  }

  // update outputs
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATTime"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(AATTaskLength);
    wp->RefreshDisplay();
  }

  double d1 = (CALCULATED_INFO.TaskDistanceToGo
	       +CALCULATED_INFO.TaskDistanceCovered);
  if (AATEnabled && (d1==0.0)) {
    d1 = CALCULATED_INFO.AATTargetDistance;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpDistance"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(d1*DISTANCEMODIFY);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMacCready"));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpRange"));
  if (wp) {
    wp->RefreshDisplay();
    wp->GetDataField()->SetAsFloat(Range*100.0);
  }

}


static void OnMacCreadyData(DataField *Sender,
			    DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
      Sender->Set(MACCREADY*LIFTMODIFY);
    break;
    case DataField::daPut:
      MACCREADY = Sender->GetAsFloat()/LIFTMODIFY;
      RefreshCalculator();
    break;
    case DataField::daChange:
      MACCREADY = Sender->GetAsFloat()/LIFTMODIFY;
      RefreshCalculator();
    break;
  }
}


static void OnRangeData(DataField *Sender, DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
      //      Sender->Set(Range*100.0);
    break;
    case DataField::daPut:
    case DataField::daChange:
      Range = Sender->GetAsFloat()/100.0;
      AdjustAATTargets(Range);
      RefreshCalculator();
    break;
  }

}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnMacCreadyData),
  DeclearCallBackEntry(OnRangeData),
  DeclearCallBackEntry(OnOKClicked),
  DeclearCallBackEntry(OnCancelClicked),
  DeclearCallBackEntry(NULL)
};


void dlgTaskCalculatorShowModal(void){

  wf = dlgLoadFromXML(CallBackTable, "\\NOR Flash\\dlgTaskCalculator.xml",
		      hWndMainWindow);

  if (!wf) return;

  // find start value for range
  Range = AdjustAATTargets(2.0);

  RefreshCalculator();

  if (wf->ShowModal() == mrCancle) {
    // todo: restore task settings.
  }
  delete wf;
  wf = NULL;

}

#endif
