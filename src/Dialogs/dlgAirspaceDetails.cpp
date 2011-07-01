/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Blackboard.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "MainWindow.hpp"
#include "Components.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

#include "Compiler.h"

#include <assert.h>
#include <stdio.h>

static const AbstractAirspace* airspace;
static WndForm *wf = NULL;

static void
OnAcknowledgeClicked(gcc_unused WndButton &Sender)
{
  assert(airspace);

  if (airspace_warnings == NULL)
    return;

  bool acked = airspace_warnings->get_ack_day(*airspace);

  int answer = MessageBoxX(airspace->get_name_text(true).c_str(), acked ?
                           _("Enable airspace again?") : _("Acknowledge for day?"),
                           MB_YESNO | MB_ICONQUESTION);

  if (answer == IDYES) {
    airspace_warnings->acknowledge_day(*airspace, !acked);
    wf->SetModalResult(mrOK);
  }
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnAcknowledgeClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

static void
UpdateAckButton()
{
  assert(airspace);

  if (airspace_warnings == NULL)
    return;

  WndButton* ack = (WndButton*)wf->FindByName(_T("cmdAcknowledge"));
  assert(ack != NULL);
  ack->SetCaption(airspace_warnings->get_ack_day(*airspace) ?
                  _("Enable") : _("Ack Day"));
}

static void
SetValues(void)
{
  assert(airspace);

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpName"));
  assert(wp != NULL);
  wp->SetText(airspace->get_name_text(true).c_str());
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpRadio"));
  assert(wp != NULL);
  wp->SetText(airspace->get_radio_text().c_str());
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpType"));
  assert(wp != NULL);
  wp->SetText(airspace->get_type_text());
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpTop"));
  assert(wp != NULL);
  wp->SetText(airspace->get_top_text().c_str());
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpBase"));
  assert(wp != NULL);
  wp->SetText(airspace->get_base_text().c_str());
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpRange"));
  assert(wp != NULL);
  const GeoPoint &ac_loc = XCSoarInterface::Basic().Location;
  const GeoPoint closest_loc = airspace->closest_point(ac_loc);
  const GeoVector vec(ac_loc, closest_loc);
  TCHAR buf[80];
  _stprintf(buf, _T("%d%s"), (int)Units::ToUserDistance(vec.Distance),
            Units::GetDistanceName());
  wp->SetText(buf);
  wp->RefreshDisplay();
}

void
dlgAirspaceDetails(const AbstractAirspace& the_airspace)
{
  airspace = &the_airspace;

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  _T("IDR_XML_AIRSPACEDETAILS"));
  assert(wf != NULL);

  SetValues();
  UpdateAckButton();

  wf->ShowModal();

  delete wf;
}
