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
#include "Dialogs/ComboPicker.hpp"
#include "DataField/ComboList.hpp"
#include "DataField/Base.hpp"
#include "Screen/Layout.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarStore.hpp"
#include "Polar/PolarFileGlue.hpp"
#include "Plane/Plane.hpp"
#include "OS/FileUtil.hpp"
#include "LocalPath.hpp"
#include "MainWindow.hpp"

#include <cstdio>

static WndForm *dialog = NULL;
static Plane plane;
static bool loading = false;

static void
UpdateCaption()
{
  TCHAR tmp[128];
  _stprintf(tmp, _("%s: %s"), _("Plane Polar"), plane.registration.c_str());
  dialog->SetCaption(tmp);
}

static void
UpdatePolarLabel()
{
  TCHAR tmp[128];
  _stprintf(tmp, _("%s: %s"), _("Polar"), plane.polar_name.c_str());

  WndFrame *label = ((WndFrame *)dialog->FindByName(_T("PolarLabel")));
  assert(label != NULL);
  label->SetCaption(tmp);
}

static void
UpdateInvalidLabel()
{
  PolarCoefficients coeff =
      PolarCoefficients::FromVW(plane.v1, plane.v2, plane.v3,
                                plane.w1, plane.w2, plane.w3);
  bool visible = !coeff.IsValid();

  WndFrame *label = ((WndFrame *)dialog->FindByName(_T("InvalidLabel")));
  assert(label != NULL);
  label->set_visible(visible);
}

static void
Update()
{
  UpdateCaption();

  loading = true;
  LoadFormProperty(*dialog, _T("V1Edit"), ugHorizontalSpeed, plane.v1);
  LoadFormProperty(*dialog, _T("V2Edit"), ugHorizontalSpeed, plane.v2);
  LoadFormProperty(*dialog, _T("V3Edit"), ugHorizontalSpeed, plane.v3);
  LoadFormProperty(*dialog, _T("W1Edit"), ugVerticalSpeed, plane.w1);
  LoadFormProperty(*dialog, _T("W2Edit"), ugVerticalSpeed, plane.w2);
  LoadFormProperty(*dialog, _T("W3Edit"), ugVerticalSpeed, plane.w3);

  LoadFormProperty(*dialog, _T("ReferenceMassEdit"), plane.reference_mass);
  LoadFormProperty(*dialog, _T("DryMassEdit"), plane.dry_mass);
  loading = false;

  UpdatePolarLabel();
  UpdateInvalidLabel();
}

static void
UpdatePlane()
{
  SaveFormProperty(*dialog, _T("V1Edit"), ugHorizontalSpeed, plane.v1);
  SaveFormProperty(*dialog, _T("V2Edit"), ugHorizontalSpeed, plane.v2);
  SaveFormProperty(*dialog, _T("V3Edit"), ugHorizontalSpeed, plane.v3);
  SaveFormProperty(*dialog, _T("W1Edit"), ugVerticalSpeed, plane.w1);
  SaveFormProperty(*dialog, _T("W2Edit"), ugVerticalSpeed, plane.w2);
  SaveFormProperty(*dialog, _T("W3Edit"), ugVerticalSpeed, plane.w3);

  SaveFormProperty(*dialog, _T("ReferenceMassEdit"), plane.reference_mass);
  SaveFormProperty(*dialog, _T("DryMassEdit"), plane.dry_mass);
}

static void
OkayClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrOK);
}

static void
CancelClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrCancel);
}

static void
ListClicked(gcc_unused WndButton &button)
{
  ComboList list;
  unsigned len = PolarStore::Count();
  for (unsigned i = 0; i < len; i++)
    list.Append(i, PolarStore::GetItem(i).name);

  list.Sort();

  // let the user select
  int result = ComboPicker(XCSoarInterface::main_window,
                           _("Load Polar"), list, NULL);
  if (result < 0)
    return;

  assert((unsigned)result < len);

  const PolarStore::Item &item = PolarStore::GetItem(list[result].DataFieldIndex);

  plane.reference_mass = fixed(item.reference_mass);
  plane.dry_mass = fixed(item.reference_mass);
  plane.max_ballast = fixed(item.max_ballast);

  if (item.wing_area > 0.0)
    plane.wing_area = fixed(item.wing_area);

  if (item.v_no > 0.0)
    plane.max_speed = fixed(item.v_no);

  plane.v1 = Units::ToSysUnit(fixed(item.v1), unKiloMeterPerHour);
  plane.v2 = Units::ToSysUnit(fixed(item.v2), unKiloMeterPerHour);
  plane.v3 = Units::ToSysUnit(fixed(item.v3), unKiloMeterPerHour);
  plane.w1 = fixed(item.w1);
  plane.w2 = fixed(item.w2);
  plane.w3 = fixed(item.w3);

  plane.polar_name = list[result].StringValue;

  if (item.contest_handicap > 0)
    plane.handicap = item.contest_handicap;

  Update();
}

class PolarFileVisitor: public File::Visitor
{
private:
  ComboList &list;

public:
  PolarFileVisitor(ComboList &_list): list(_list) {}

  void Visit(const TCHAR* path, const TCHAR* filename) {
    list.Append(0, path, filename);
  }
};

static void
ImportClicked(gcc_unused WndButton &button)
{
  ComboList list;
  PolarFileVisitor fv(list);

  // Fill list
  VisitDataFiles(_T("*.plr"), fv);

  list.Sort();

  // let the user select
  int result = ComboPicker(XCSoarInterface::main_window,
                           _("Load Polar From File"), list, NULL);
  if (result < 0)
    return;

  assert((unsigned)result < list.size());

  PolarInfo polar;
  const TCHAR* path = list[result].StringValue;
  PolarGlue::LoadFromFile(polar, path);

  plane.reference_mass = polar.reference_mass;
  plane.dry_mass = polar.reference_mass;
  plane.max_ballast = polar.max_ballast;

  if (positive(polar.wing_area))
    plane.wing_area = polar.wing_area;

  if (positive(polar.v_no))
    plane.max_speed = polar.v_no;

  plane.v1 = polar.v1;
  plane.v2 = polar.v2;
  plane.v3 = polar.v3;
  plane.w1 = polar.w1;
  plane.w2 = polar.w2;
  plane.w3 = polar.w3;

  plane.polar_name = list[result].StringValueFormatted;

  Update();
}

static void
PolarChanged(gcc_unused DataField *Sender, DataField::DataAccessKind_t Mode)
{
  if (loading)
    return;

  switch (Mode) {
  case DataField::daChange:
    plane.polar_name = _T("Custom");
    UpdatePolarLabel();
    UpdateInvalidLabel();
    break;

  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    return;
  }
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OkayClicked),
  DeclareCallBackEntry(CancelClicked),
  DeclareCallBackEntry(ListClicked),
  DeclareCallBackEntry(ImportClicked),
  DeclareCallBackEntry(PolarChanged),
  DeclareCallBackEntry(NULL)
};

bool
dlgPlanePolarShowModal(SingleWindow &parent, Plane &_plane)
{
  plane = _plane;

  dialog = LoadDialog(CallBackTable, parent, Layout::landscape ?
                      _T("IDR_XML_PLANE_POLAR_L") : _T("IDR_XML_PLANE_POLAR"));
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

