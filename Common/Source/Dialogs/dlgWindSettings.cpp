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
#include "Blackboard.hpp"
#include "SettingsUser.hpp"
#include "SettingsComputer.hpp"
#include "Units.hpp"
#include "Registry.hpp"
#include "DataField/Enum.hpp"
#include "Math/Units.h"
#include "MainWindow.hpp"

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
  if ((ws != XCSoarInterface::Calculated().WindSpeed)
      ||(wb != XCSoarInterface::Calculated().WindBearing)) {
    /* JMW illegal
    if (set) {
      SetWindEstimate(ws, wb);
    }
    XCSoarInterface::Calculated().WindSpeed = ws;
    XCSoarInterface::Calculated().WindBearing = wb;
    */
  }
}


static void OnSaveClicked(WindowControl * Sender){
  (void)Sender;
  UpdateWind(true);
  Profile::SaveWindToRegistry();
  wf->SetModalResult(mrOK);
}


static void OnWindSpeedData(DataField *Sender, DataField::DataAccessKind_t Mode){

  switch(Mode){
    case DataField::daGet:
      Sender->SetMax(SPEEDMODIFY*(200.0/TOKPH));
      Sender->Set(SPEEDMODIFY*XCSoarInterface::Calculated().WindSpeed);
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
      lastWind = XCSoarInterface::Calculated().WindBearing;
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
  wf = dlgLoadFromXML(CallBackTable,
                      TEXT("dlgWindSettings.xml"),
		      XCSoarInterface::main_window,
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
      wp->GetDataField()->Set(XCSoarInterface::SettingsComputer().AutoWindMode);
      wp->RefreshDisplay();

      wp = (WndProperty*)wf->FindByName(TEXT("prpTrailDrift"));
      if (wp) {
        wp->GetDataField()->Set(XCSoarInterface::SettingsMap().EnableTrailDrift);
        wp->RefreshDisplay();
      }
    }

    wf->ShowModal();

    SetValueRegistryOnChange(wf, TEXT("prpAutoWind"), szRegistryAutoWind,
			     XCSoarInterface::SetSettingsComputer().AutoWindMode);
    SetValueOnChange(wf, TEXT("prpTrailDrift"),
		     XCSoarInterface::SetSettingsMap().EnableTrailDrift);

    delete wf;
  }
  wf = NULL;

}

