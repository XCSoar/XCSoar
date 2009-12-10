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
#include "Units.hpp"
#include "UtilsProfile.hpp"
#include "Profile.hpp"
#include "Registry.hpp"
#include "Audio/VegaVoice.h"
#include "DataField/Base.hpp"
#include "MainWindow.hpp"

static WndForm *wf=NULL;

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};


static void LoadIntoForm(WndForm &form, const SETTINGS_COMPUTER &settings){
  WndProperty* wp;
  wp = (WndProperty*)form.FindByName(_T("prpVoiceClimbRate"));
  if (wp) {
    wp->GetDataField()->Set(settings.EnableVoiceClimbRate);
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)form.FindByName(_T("prpVoiceTerrain"));
  if (wp) {
    wp->GetDataField()->Set(settings.EnableVoiceTerrain);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)form.FindByName(_T("prpVoiceWaypointDistance"));
  if (wp) {
    wp->GetDataField()->Set(settings.EnableVoiceWaypointDistance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)form.FindByName(_T("prpVoiceTaskAltitudeDifference"));
  if (wp) {
    wp->GetDataField()->Set(settings.EnableVoiceTaskAltitudeDifference);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)form.FindByName(_T("prpVoiceMacCready"));
  if (wp) {
    wp->GetDataField()->Set(settings.EnableVoiceMacCready);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)form.FindByName(_T("prpVoiceNewWaypoint"));
  if (wp) {
    wp->GetDataField()->Set(settings.EnableVoiceNewWaypoint);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)form.FindByName(_T("prpVoiceInSector"));
  if (wp) {
    wp->GetDataField()->Set(settings.EnableVoiceInSector);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)form.FindByName(_T("prpVoiceAirspace"));
  if (wp) {
    wp->GetDataField()->Set(settings.EnableVoiceAirspace);
    wp->RefreshDisplay();
  }
}

static bool SaveFromForm(WndForm &form, SETTINGS_COMPUTER &settings){
  const WndProperty* wp;
  bool changed = false;
  wp = (const WndProperty*)form.FindByName(_T("prpVoiceClimbRate"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != settings.EnableVoiceClimbRate) {
      settings.EnableVoiceClimbRate = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceClimbRate, settings.EnableVoiceClimbRate);
      changed = true;
    }
  }
  
  wp = (const WndProperty*)form.FindByName(_T("prpVoiceTerrain"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != settings.EnableVoiceTerrain) {
      settings.EnableVoiceTerrain = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceTerrain, settings.EnableVoiceTerrain);
      changed = true;
    }
  }
  
  wp = (const WndProperty*)form.FindByName(_T("prpVoiceWaypointDistance"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != settings.EnableVoiceWaypointDistance) {
      settings.EnableVoiceWaypointDistance = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceWaypointDistance, settings.EnableVoiceWaypointDistance);
      changed = true;
    }
  }
  
  wp = (const WndProperty*)form.FindByName(_T("prpVoiceTaskAltitudeDifference"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != settings.EnableVoiceTaskAltitudeDifference) {
      settings.EnableVoiceTaskAltitudeDifference = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceTaskAltitudeDifference, settings.EnableVoiceTaskAltitudeDifference);
      changed = true;
    }
   }
 
  wp = (const WndProperty*)form.FindByName(_T("prpVoiceMacCready"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != settings.EnableVoiceMacCready) {
      settings.EnableVoiceMacCready = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceMacCready, settings.EnableVoiceMacCready);
      changed = true;
    }
  }
  
  wp = (const WndProperty*)form.FindByName(_T("prpVoiceNewWaypoint"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != settings.EnableVoiceNewWaypoint) {
      settings.EnableVoiceNewWaypoint = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceNewWaypoint,settings.EnableVoiceNewWaypoint);
      changed = true;
    }
  }
  
  wp = (const WndProperty*)form.FindByName(_T("prpVoiceInSector"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != settings.EnableVoiceInSector) {
      settings.EnableVoiceInSector = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceInSector, settings.EnableVoiceInSector);
      changed = true;
    }
  }
  
  wp = (const WndProperty*)form.FindByName(_T("prpVoiceAirspace"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != settings.EnableVoiceAirspace) {
      settings.EnableVoiceAirspace = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceAirspace,settings.EnableVoiceAirspace);
      changed = true;
    }
  }
  return changed;
}


void dlgVoiceShowModal(void){
  wf = dlgLoadFromXML(CallBackTable,
                      _T("dlgVoice.xml"),
		      XCSoarInterface::main_window,
		      _T("IDR_XML_VOICE"));

  
  if (!wf) return;

  LoadIntoForm(*wf, XCSoarInterface::SettingsComputer());

  wf->ShowModal();

  bool changed = false;

  changed = SaveFromForm(*wf, XCSoarInterface::SetSettingsComputer());
  
  if (changed) {
    Profile::StoreRegistry();

    MessageBoxX(gettext(_T("Changes to configuration saved.")),
		 _T(""), MB_OK);
  }
    
  delete wf;
  wf = NULL;

}

