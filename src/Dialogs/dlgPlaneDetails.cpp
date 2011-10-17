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

#include "Dialogs/Planes.hpp"
#include "Dialogs/Internal.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Screen/Layout.hpp"
#include "Plane/Plane.hpp"
#include "MainWindow.hpp"

#include <cstdio>

static WndForm *dialog = NULL;
static Plane plane;

static void
UpdateCaption()
{
  TCHAR tmp[128];
  _stprintf(tmp, _("%s: %s"), _("Plane Details"), plane.registration.c_str());
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
  UpdateCaption();

  UpdateButton(_T("RegistrationButton"), plane.registration);
  UpdateButton(_T("CompetitionIDButton"), plane.competition_id);
  UpdateButton(_T("TypeButton"), plane.type);
  UpdateButton(_T("PolarButton"), plane.polar_name);

  LoadFormProperty(*dialog, _T("HandicapEdit"), plane.handicap);
  LoadFormProperty(*dialog, _T("WingAreaEdit"), plane.wing_area);
  LoadFormProperty(*dialog, _T("MaxBallastEdit"), plane.max_ballast);
  LoadFormProperty(*dialog, _T("DumpTimeEdit"), plane.dump_time);
  LoadFormProperty(*dialog, _T("MaxSpeedEdit"),
                   ugHorizontalSpeed, plane.max_speed);
}

static void
UpdatePlane()
{
  SaveFormProperty(*dialog, _T("HandicapEdit"), plane.handicap);
  SaveFormProperty(*dialog, _T("WingAreaEdit"), plane.wing_area);
  SaveFormProperty(*dialog, _T("MaxBallastEdit"), plane.max_ballast);
  SaveFormProperty(*dialog, _T("DumpTimeEdit"), plane.dump_time);
  SaveFormProperty(*dialog, _T("MaxSpeedEdit"),
                   ugHorizontalSpeed, plane.max_speed);
}

static void
OKClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrOK);
}

static void
CancelClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrCancel);
}

static void
RegistrationClicked(gcc_unused WndButton &button)
{
  if (!dlgTextEntryShowModal(*(SingleWindow *)dialog->get_root_owner(),
                             plane.registration.buffer(),
                             plane.registration.MAX_SIZE))
    return;

  UpdateCaption();
  UpdateButton(_T("RegistrationButton"), plane.registration);
}

static void
CompetitionIDClicked(gcc_unused WndButton &button)
{
  if (!dlgTextEntryShowModal(*(SingleWindow *)dialog->get_root_owner(),
                             plane.competition_id.buffer(),
                             plane.competition_id.MAX_SIZE))
    return;

  UpdateButton(_T("CompetitionIDButton"), plane.competition_id);
}

static void
TypeClicked(gcc_unused WndButton &button)
{
  if (!dlgTextEntryShowModal(*(SingleWindow *)dialog->get_root_owner(),
                             plane.type.buffer(),
                             plane.type.MAX_SIZE))
    return;

  UpdateButton(_T("TypeButton"), plane.type);
}

static void
PolarClicked(gcc_unused WndButton &button)
{
  dlgPlanePolarShowModal(*(SingleWindow*)dialog->get_root_owner(), plane);
  Update();
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OKClicked),
  DeclareCallBackEntry(CancelClicked),
  DeclareCallBackEntry(RegistrationClicked),
  DeclareCallBackEntry(CompetitionIDClicked),
  DeclareCallBackEntry(TypeClicked),
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

