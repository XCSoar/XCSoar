/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Dialogs/Planes.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/String.hpp"
#include "Screen/Layout.hpp"
#include "Plane/Plane.hpp"
#include "Language/Language.hpp"

#include <cstdio>

static WndForm *dialog = NULL;
static Plane plane;

static void
UpdateCaption()
{
  StaticString<128> tmp;
  tmp.Format(_T("%s: %s"), _("Plane Details"),
             GetFormValueString(*dialog, _T("Registration")));
  dialog->SetCaption(tmp);
}

static void
UpdateButton(const TCHAR *button, const TCHAR *caption)
{
  WndButton* b = (WndButton*)dialog->FindByName(button);
  assert(b != NULL);
  b->SetCaption(caption);
}

static void
Update()
{
  LoadFormProperty(*dialog, _T("Registration"), plane.registration);
  LoadFormProperty(*dialog, _T("CompetitionID"), plane.competition_id);
  LoadFormProperty(*dialog, _T("Type"), plane.type);
  UpdateButton(_T("PolarButton"), plane.polar_name);

  LoadFormProperty(*dialog, _T("HandicapEdit"), plane.handicap);
  LoadFormProperty(*dialog, _T("WingAreaEdit"), plane.wing_area);
  LoadFormProperty(*dialog, _T("MaxBallastEdit"), plane.max_ballast);
  LoadFormProperty(*dialog, _T("DumpTimeEdit"), plane.dump_time);
  LoadFormProperty(*dialog, _T("MaxSpeedEdit"),
                   UnitGroup::HORIZONTAL_SPEED, plane.max_speed);

  UpdateCaption();
}

static void
UpdatePlane()
{
  SaveFormProperty(*dialog, _T("Registration"), plane.registration);
  SaveFormProperty(*dialog, _T("CompetitionID"), plane.competition_id);
  SaveFormProperty(*dialog, _T("Type"), plane.type);
  SaveFormProperty(*dialog, _T("HandicapEdit"), plane.handicap);
  SaveFormProperty(*dialog, _T("WingAreaEdit"), plane.wing_area);
  SaveFormProperty(*dialog, _T("MaxBallastEdit"), plane.max_ballast);
  SaveFormProperty(*dialog, _T("DumpTimeEdit"), plane.dump_time);
  SaveFormProperty(*dialog, _T("MaxSpeedEdit"),
                   UnitGroup::HORIZONTAL_SPEED, plane.max_speed);
}

static void
OKClicked()
{
  dialog->SetModalResult(mrOK);
}

static void
CancelClicked()
{
  dialog->SetModalResult(mrCancel);
}

static void
OnRegistrationData(DataField *sender, DataField::DataAccessMode mode)
{
  if (mode == DataField::daChange)
    UpdateCaption();
}

static void
PolarClicked()
{
  dlgPlanePolarShowModal(*(SingleWindow*)dialog->GetRootOwner(), plane);
  Update();
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OKClicked),
  DeclareCallBackEntry(CancelClicked),
  DeclareCallBackEntry(OnRegistrationData),
  DeclareCallBackEntry(PolarClicked),
  DeclareCallBackEntry(NULL)
};

bool
dlgPlaneDetailsShowModal(SingleWindow &parent, Plane &_plane)
{
  plane = _plane;

  dialog = LoadDialog(CallBackTable, parent, Layout::landscape ?
                      _T("IDR_XML_PLANE_DETAILS_L") : _T("IDR_XML_PLANE_DETAILS"));
  assert(dialog != NULL);

  Update();
  bool result = (dialog->ShowModal() == mrOK);
  if (result) {
    UpdatePlane();
    _plane = plane;
  }

  delete dialog;

  return result;
}

