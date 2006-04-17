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
#include "device.h"
#include "InputEvents.h"
#include "WindowControls.h"
#include "dlgTools.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;


//
//

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}


bool EnableAutoBrightness=true;
int BrightnessValue=0;
///////////

static void UpdateValues() {
  static DWORD fpsTimeLast = 0;
  DWORD fpsTime = ::GetTickCount();
  if (fpsTime-fpsTimeLast<200) {
    return;
  }
  fpsTimeLast = fpsTime;

  TCHAR text[100];
  if (EnableAutoBrightness) {
    InputEvents::eventDLLExecute(
				 TEXT("altairplatform.dll SetAutoMode on"));
    _stprintf(text,TEXT("altairplatform.dll SetAutoBrightness %03d"),
	      BrightnessValue);
  } else {
    InputEvents::eventDLLExecute(
				 TEXT("altairplatform.dll SetAutoMode off"));
    _stprintf(text,TEXT("altairplatform.dll SetManualBrightness %03d"),
	      BrightnessValue);
  }
  InputEvents::eventDLLExecute(text);

}

static void OnAutoData(DataField *Sender, DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
      Sender->Set(EnableAutoBrightness);
    break;
    case DataField::daPut:
    case DataField::daChange:
      EnableAutoBrightness = (Sender->GetAsInteger()!=0);
      UpdateValues();
    break;
  }
}


static void OnBrightnessData(DataField *Sender,
			     DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
      Sender->SetAsFloat(BrightnessValue);
    break;
    case DataField::daPut:
    case DataField::daChange:
      BrightnessValue = iround(Sender->GetAsFloat());
      UpdateValues();
    break;
  }
}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnAutoData),
  DeclearCallBackEntry(OnBrightnessData),
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(NULL)
};



void dlgBrightnessShowModal(void){

  wf = dlgLoadFromXML(CallBackTable, LocalPathS(TEXT("dlgBrightness.xml")),
		      hWndMainWindow,
		      TEXT("IDR_XML_BRIGHTNESS"));

  WndProperty* wp;

  if (wf) {

    wp = (WndProperty*)wf->FindByName(TEXT("prpBrightness"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(BrightnessValue);
      wp->RefreshDisplay();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpAuto"));
    if (wp) {
      wp->GetDataField()->Set(EnableAutoBrightness);
      wp->RefreshDisplay();
    }
    wf->ShowModal();

    UpdateValues();

    delete wf;
  }
  wf = NULL;

}


#endif
