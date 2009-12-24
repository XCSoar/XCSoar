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
#include "Dialogs/Message.hpp"
#include "XCSoar.h"
#include "SettingsTask.hpp"
#include "UtilsProfile.hpp"
#include "Profile.hpp"
#include "Registry.hpp"
#include "Math/FastMath.h"
#include "DataField/Enum.hpp"
#include "MainWindow.hpp"
#include "Task.h"

static bool changed = false;
static WndForm *wf=NULL;
static SETTINGS_TASK settings_task;

static void OnRulesActiveData(DataField *Sender,
			      DataField::DataAccessKind_t Mode){

  switch(Mode){
    case DataField::daGet:
    break;
    case DataField::daPut:
    case DataField::daChange:
      // TODO enhancement: hide/show fields as appropriate
    break;
  }
}


static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnRulesActiveData),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};



static void setVariables(void) {
  WndProperty *wp;

  wp = (WndProperty*)wf->FindByName(_T("prpOLCEnabled"));
  if (wp) {
    wp->GetDataField()->Set(XCSoarInterface::SettingsComputer().EnableOLC);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFAIFinishHeight"));
  if (wp) {
    wp->GetDataField()->Set(settings_task.EnableFAIFinishHeight);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartHeightRef"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("AGL")));
    dfe->addEnumText(gettext(_T("MSL")));
    dfe->Set(settings_task.StartHeightRef);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpOLCRules"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Sprint")));
    dfe->addEnumText(gettext(_T("Triangle")));
    dfe->addEnumText(gettext(_T("Classic")));
    dfe->Set(XCSoarInterface::SettingsComputer().OLCRules);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFinishMinHeight"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(settings_task.FinishMinHeight*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxHeight"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(settings_task.StartMaxHeight*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxSpeed"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(settings_task.StartMaxSpeed*SPEEDMODIFY));
    wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
    wp->RefreshDisplay();
  }
}


bool dlgTaskRules(void){

  WndProperty *wp;

  wf = dlgLoadFromXML(CallBackTable,
                      _T("dlgTaskRules.xml"),
		      XCSoarInterface::main_window,
		      _T("IDR_XML_TASKRULES"));

  if (!wf) return false;

  settings_task = task.getSettings();

  setVariables();

  changed = false;

  wf->ShowModal();

  // TODO enhancement: implement a cancel button that skips all this below after exit.

  int ival;

  changed |= SetValueRegistryOnChange(wf, _T("prpFAIFinishHeight"),
				      szRegistryFAIFinishHeight,
				      settings_task.EnableFAIFinishHeight);
  changed |= SetValueRegistryOnChange(wf, _T("prpStartHeightRef"),
				      szRegistryStartHeightRef,
				      settings_task.StartHeightRef);
  changed |= SetValueRegistryOnChange(wf, _T("prpOLCRules"),
				      szRegistryOLCRules,
				      XCSoarInterface::SetSettingsComputer().OLCRules);
  changed |= SetValueOnChange(wf, _T("prpOLCEnabled"),
			      XCSoarInterface::SetSettingsComputer().EnableOLC);

  wp = (WndProperty*)wf->FindByName(_T("prpFinishMinHeight"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)settings_task.FinishMinHeight != ival) {
      settings_task.FinishMinHeight = ival;
      SetToRegistry(szRegistryFinishMinHeight,settings_task.FinishMinHeight);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxHeight"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)settings_task.StartMaxHeight != ival) {
      settings_task.StartMaxHeight = ival;
      SetToRegistry(szRegistryStartMaxHeight,settings_task.StartMaxHeight);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartMaxSpeed"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/SPEEDMODIFY);
    if ((int)settings_task.StartMaxSpeed != ival) {
      settings_task.StartMaxSpeed = ival;
      SetToRegistry(szRegistryStartMaxSpeed,settings_task.StartMaxSpeed);
      changed = true;
    }
  }

  delete wf;

  if (changed) {

    task.setSettings(settings_task);

    Profile::StoreRegistry();

    MessageBoxX (
		 gettext(_T("Changes to configuration saved.")),
		 _T(""), MB_OK);
  }

  return changed;

  wf = NULL;

}

