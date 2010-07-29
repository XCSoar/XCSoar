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
#include "Blackboard.hpp"
#include "SettingsUser.hpp"
#include "SettingsComputer.hpp"
#include "Units.hpp"
#include "Profile.hpp"
#include "DataField/Enum.hpp"
#include "MainWindow.hpp"

static WndForm *wf = NULL;

static void
OnCancel(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrCancel);
}

static void
UpdateWind(bool set)
{
  WndProperty *wp;
  fixed speed = fixed_zero, direction = fixed_zero;
  wp = (WndProperty*)wf->FindByName(_T("prpSpeed"));
  if (wp)
    speed = Units::ToSysSpeed(wp->GetDataField()->GetAsFixed());

  wp = (WndProperty*)wf->FindByName(_T("prpDirection"));
  if (wp)
    direction = wp->GetDataField()->GetAsFixed();

  if ((speed != XCSoarInterface::Basic().wind.norm)
      ||(direction != XCSoarInterface::Basic().wind.bearing.value_degrees())) {
    /* JMW illegal
    if (set) {
      SetWindEstimate(ws, wb);
    }
    XCSoarInterface::Calculated().WindSpeed = ws;
    XCSoarInterface::Calculated().WindBearing = wb;
    */
  }
}

static void
OnOkay(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void
OnWindSpeedData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daGet:
    Sender->Set(Units::ToUserWindSpeed(XCSoarInterface::Basic().wind.norm));
    break;
  case DataField::daPut:
    UpdateWind(false);
    break;
  case DataField::daChange:
    // calc alt...
    break;
  }
}

static void
OnWindDirectionData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daGet:
    Sender->Set(XCSoarInterface::Basic().wind.bearing.value_degrees());
    break;
  case DataField::daPut:
    UpdateWind(false);
    break;
  }
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnWindSpeedData),
  DeclareCallBackEntry(OnWindDirectionData),
  DeclareCallBackEntry(OnOkay),
  DeclareCallBackEntry(OnCancel),
  DeclareCallBackEntry(NULL)
};

void
dlgWindSettingsShowModal(void)
{
  wf = dlgLoadFromXML(CallBackTable, XCSoarInterface::main_window,
		                  _T("IDR_XML_WINDSETTINGS"));
  if (wf == NULL)
    return;

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpSpeed"));
  if (wp) {
    wp->GetDataField()->SetMax(Units::ToUserWindSpeed(
        Units::ToSysUnit(200.0, unKiloMeterPerHour)));
    wp->GetDataField()->SetUnits(Units::GetSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoWind"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Manual"));
    dfe->addEnumText(_("Circling"));
    dfe->addEnumText(_("ZigZag"));
    dfe->addEnumText(_("Both"));
    wp->GetDataField()->Set(XCSoarInterface::SettingsComputer().AutoWindMode);
    wp->RefreshDisplay();

    wp = (WndProperty*)wf->FindByName(_T("prpTrailDrift"));
    if (wp) {
      wp->GetDataField()->Set(XCSoarInterface::SettingsMap().EnableTrailDrift);
      wp->RefreshDisplay();
    }
  }

  if (wf->ShowModal() == mrOK)
    UpdateWind(true);

  SaveFormProperty(wf, _T("prpAutoWind"), szProfileAutoWind,
                   XCSoarInterface::SetSettingsComputer().AutoWindMode);
  SaveFormProperty(wf, _T("prpTrailDrift"),
                   XCSoarInterface::SetSettingsMap().EnableTrailDrift);

  delete wf;
}
