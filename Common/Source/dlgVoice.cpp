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

#include "StdAfx.h"
#include "XCSoar.h"
#include "Dialogs.h"
#include "Units.h"
#include "InputEvents.h"
#include "dlgTools.h"
#include "Utils.h"
#include "Registry.hpp"
#include "VegaVoice.h"
#include "DataField/Base.hpp"

static WndForm *wf=NULL;

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};



void dlgVoiceShowModal(void){
  wf = dlgLoadFromXML(CallBackTable,
                      TEXT("dlgVoice.xml"),
		      hWndMainWindow,
		      TEXT("IDR_XML_VOICE"));

  WndProperty* wp;

  if (!wf) return;

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceClimbRate"));
  if (wp) {
    wp->GetDataField()->Set(EnableVoiceClimbRate);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceTerrain"));
  if (wp) {
    wp->GetDataField()->Set(EnableVoiceTerrain);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceWaypointDistance"));
  if (wp) {
    wp->GetDataField()->Set(EnableVoiceWaypointDistance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceTaskAltitudeDifference"));
  if (wp) {
    wp->GetDataField()->Set(EnableVoiceTaskAltitudeDifference);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceMacCready"));
  if (wp) {
    wp->GetDataField()->Set(EnableVoiceMacCready);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceNewWaypoint"));
  if (wp) {
    wp->GetDataField()->Set(EnableVoiceNewWaypoint);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceInSector"));
  if (wp) {
    wp->GetDataField()->Set(EnableVoiceInSector);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceAirspace"));
  if (wp) {
    wp->GetDataField()->Set(EnableVoiceAirspace);
    wp->RefreshDisplay();
  }

  wf->ShowModal();

  bool changed = false;

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceClimbRate"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != EnableVoiceClimbRate) {
      EnableVoiceClimbRate = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceClimbRate, EnableVoiceClimbRate);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceTerrain"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != EnableVoiceTerrain) {
      EnableVoiceTerrain = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceTerrain, EnableVoiceTerrain);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceWaypointDistance"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != EnableVoiceWaypointDistance) {
      EnableVoiceWaypointDistance = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceWaypointDistance,
		    EnableVoiceWaypointDistance);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceTaskAltitudeDifference"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != EnableVoiceTaskAltitudeDifference) {
      EnableVoiceTaskAltitudeDifference = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceTaskAltitudeDifference,
		    EnableVoiceTaskAltitudeDifference);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceMacCready"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != EnableVoiceMacCready) {
      EnableVoiceMacCready = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceMacCready,
		    EnableVoiceMacCready);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceNewWaypoint"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != EnableVoiceNewWaypoint) {
      EnableVoiceNewWaypoint = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceNewWaypoint,
		    EnableVoiceNewWaypoint);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceInSector"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != EnableVoiceInSector) {
      EnableVoiceInSector = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceInSector,
		    EnableVoiceInSector);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVoiceAirspace"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() != EnableVoiceAirspace) {
      EnableVoiceAirspace = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryVoiceAirspace,
		    EnableVoiceAirspace);
      changed = true;
    }
  }

  if (changed) {
    StoreRegistry();

    MessageBoxX(gettext(TEXT("Changes to configuration saved.")),
		 TEXT(""), MB_OK);
  }

  delete wf;
  wf = NULL;

}

