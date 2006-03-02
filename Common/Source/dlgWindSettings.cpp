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

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static void OnSaveClicked(WindowControl * Sender){
  SaveWindToRegistry();
  wf->SetModalResult(mrOK);
}

static void OnWindSpeedData(DataField *Sender, DataField::DataAccessKind_t Mode){

  switch(Mode){
    case DataField::daGet:
      Sender->SetMax(SPEEDMODIFY*(200.0/TOKPH));
      Sender->Set(SPEEDMODIFY*CALCULATED_INFO.WindSpeed);
    break;
    case DataField::daPut:
      CALCULATED_INFO.WindSpeed = Sender->GetAsFloat()/SPEEDMODIFY;
    break;
    case DataField::daChange:
      // calc alt...
    break;
  }
}

static void OnWindDirectionData(DataField *Sender, DataField::DataAccessKind_t Mode){

  double lastWind;

  switch(Mode){
    case DataField::daGet:
      lastWind = CALCULATED_INFO.WindBearing;
      if (lastWind < 0.5)
        lastWind = 360.0;
      Sender->Set(lastWind);
    break;
    case DataField::daPut:
      CALCULATED_INFO.WindBearing = Sender->GetAsFloat();
    break;
    case DataField::daChange:
      lastWind = Sender->GetAsFloat();
      if (lastWind < 0.5)
        Sender->Set(360.0);
      if (lastWind > 360.5)
        Sender->Set(1.0);
    break;
  }

}

static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnWindSpeedData),
  DeclearCallBackEntry(OnWindDirectionData),
  DeclearCallBackEntry(OnSaveClicked),
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(NULL)
};

void dlgWindSettingsShowModal(void){

  wf = dlgLoadFromXML(CallBackTable, "\\NOR Flash\\dlgWindSettings.xml", hWndMainWindow);

  if (wf) {
    WndProperty* wp;
    wp = (WndProperty*)wf->FindByName(TEXT("prpSpeed"));
    if (wp) {
      wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
      wp->RefreshDisplay();
    }
    wf->ShowModal();
    
    delete wf;
  }
  wf = NULL;

}


#endif
