/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008  

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

#include "StdAfx.h"

#include "Statistics.h"

#include "externs.h"
#include "Units.h"
#include "dlgTools.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static void UpdateWind(bool set) {
  WndProperty *wp;
  double ws = 0.0, wb = 0.0;
  wp = (WndProperty*)wf->FindByName(TEXT("prpSpeed"));
  if (wp) {
    ws = wp->GetDataField()->GetAsFloat()/SPEEDMODIFY;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpDirection"));
  if (wp) {
    wb = wp->GetDataField()->GetAsFloat();
  }
  if ((ws != CALCULATED_INFO.WindSpeed)
      ||(wb != CALCULATED_INFO.WindBearing)) {
    if (set) {
      SetWindEstimate(ws, wb);
    }
    CALCULATED_INFO.WindSpeed = ws;
    CALCULATED_INFO.WindBearing = wb;
  }
}


static void OnSaveClicked(WindowControl * Sender){
	(void)Sender;
        UpdateWind(true);
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
      UpdateWind(false);
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
      UpdateWind(false);
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
  DeclareCallBackEntry(OnWindSpeedData),
  DeclareCallBackEntry(OnWindDirectionData),
  DeclareCallBackEntry(OnSaveClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void dlgWindSettingsShowModal(void){

  char filename[MAX_PATH];
  LocalPathS(filename, TEXT("dlgWindSettings.xml"));
  wf = dlgLoadFromXML(CallBackTable, 
		      
                      filename, 
		      hWndMainWindow,
		      TEXT("IDR_XML_WINDSETTINGS"));

  if (wf) {
    WndProperty* wp;

    wp = (WndProperty*)wf->FindByName(TEXT("prpSpeed"));
    if (wp) {
      wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
      wp->RefreshDisplay();
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpAutoWind"));
    if (wp) {
      DataFieldEnum* dfe;
      dfe = (DataFieldEnum*)wp->GetDataField();
      dfe->addEnumText(gettext(TEXT("Manual")));
      dfe->addEnumText(gettext(TEXT("Circling")));
      dfe->addEnumText(gettext(TEXT("ZigZag")));
      dfe->addEnumText(gettext(TEXT("Both")));
      wp->GetDataField()->Set(AutoWindMode);
      wp->RefreshDisplay();

      wp = (WndProperty*)wf->FindByName(TEXT("prpTrailDrift"));
      if (wp) {
        wp->GetDataField()->Set(MapWindow::EnableTrailDrift);
        wp->RefreshDisplay();
      }
    }

    wf->ShowModal();

    wp = (WndProperty*)wf->FindByName(TEXT("prpAutoWind"));
    if (wp) {
      if (AutoWindMode != wp->GetDataField()->GetAsInteger()) {
	AutoWindMode = wp->GetDataField()->GetAsInteger();
	SetToRegistry(szRegistryAutoWind, AutoWindMode);
      }
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpTrailDrift"));
    if (wp) {
      if (MapWindow::EnableTrailDrift != wp->GetDataField()->GetAsBoolean()) {
        MapWindow::EnableTrailDrift = wp->GetDataField()->GetAsBoolean();
        // SetToRegistry(szRegistryTrailDrift, MapWindow::EnableTrailDrift);
      }
    }
    
    delete wf;
  }
  wf = NULL;

}

