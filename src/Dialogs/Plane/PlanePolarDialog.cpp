/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "PlaneDialogs.hpp"
#include "PolarShapeEditWidget.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Form/DataField/Base.hpp"
#include "Widget/DockWindow.hpp"
#include "Screen/Layout.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarStore.hpp"
#include "Polar/PolarFileGlue.hpp"
#include "Plane/Plane.hpp"
#include "OS/FileUtil.hpp"
#include "LocalPath.hpp"
#include "Units/Units.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"
#include "Language/Language.hpp"

#include <cstdio>

static WndForm *dialog = NULL;
static Plane plane;
static bool loading = false;

static PolarShapeEditWidget &
GetShapeEditor(SubForm &form)
{
  DockWindow &dock = *(DockWindow *)form.FindByName(_T("shape"));
  return *(PolarShapeEditWidget *)dock.GetWidget();
}

static void
UpdateCaption()
{
  StaticString<128> tmp;
  tmp.Format(_T("%s: %s"), _("Plane Polar"), plane.registration.c_str());
  dialog->SetCaption(tmp);
}

static void
UpdatePolarLabel()
{
  StaticString<128> tmp;
  tmp.Format(_T("%s: %s"), _("Polar"), plane.polar_name.c_str());

  WndFrame *label = ((WndFrame *)dialog->FindByName(_T("PolarLabel")));
  assert(label != NULL);
  label->SetCaption(tmp);
}

static void
UpdateInvalidLabel()
{
  PolarShapeEditWidget &widget = GetShapeEditor(*dialog);
  bool changed = false;
  bool valid = widget.Save(changed) &&
    widget.GetPolarShape().IsValid();
  bool visible = !valid;

  WndFrame *label = ((WndFrame *)dialog->FindByName(_T("InvalidLabel")));
  assert(label != NULL);
  label->SetVisible(visible);
}

static void
LoadPolarShape(SubForm &form, const PolarShape &shape)
{
  GetShapeEditor(form).SetPolarShape(shape);
}

static void
SavePolarShape(SubForm &form, PolarShape &shape)
{
  PolarShapeEditWidget &widget = GetShapeEditor(form);
  bool changed = false;
  widget.Save(changed);
  shape = widget.GetPolarShape();
}

static void
Update()
{
  UpdateCaption();

  loading = true;
  LoadPolarShape(*dialog, plane.polar_shape);

  LoadFormProperty(*dialog, _T("ReferenceMassEdit"), plane.reference_mass);
  LoadFormProperty(*dialog, _T("DryMassEdit"), plane.dry_mass);
  loading = false;

  UpdatePolarLabel();
  UpdateInvalidLabel();
}

static void
UpdatePlane()
{
  SavePolarShape(*dialog, plane.polar_shape);

  SaveFormProperty(*dialog, _T("ReferenceMassEdit"), plane.reference_mass);
  SaveFormProperty(*dialog, _T("DryMassEdit"), plane.dry_mass);
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
ListClicked()
{
  ComboList list;
  unsigned len = PolarStore::Count();
  for (unsigned i = 0; i < len; i++)
    list.Append(i, PolarStore::GetItem(i).name);

  list.Sort();

  // let the user select
  int result = ComboPicker(_("Load Polar"), list, NULL);
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

  plane.polar_shape = item.ToPolarShape();

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
ImportClicked()
{
  ComboList list;
  PolarFileVisitor fv(list);

  // Fill list
  VisitDataFiles(_T("*.plr"), fv);

  list.Sort();

  // let the user select
  int result = ComboPicker(_("Load Polar From File"), list, NULL);
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

  plane.polar_shape = polar.shape;

  plane.polar_name = list[result].StringValueFormatted;

  Update();
}

static void
PolarChanged(gcc_unused DataField *Sender, DataField::DataAccessMode Mode)
{
  if (loading)
    return;

  switch (Mode) {
  case DataField::daChange:
    plane.polar_name = _T("Custom");
    UpdatePolarLabel();
    UpdateInvalidLabel();
    break;

  case DataField::daSpecial:
    return;
  }
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OKClicked),
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

  DockWindow &dock = *(DockWindow *)dialog->FindByName(_T("shape"));
  PolarShapeEditWidget *shape_editor =
    new PolarShapeEditWidget(plane.polar_shape);
  dock.SetWidget(shape_editor);
  shape_editor->SetDataAccessCallback(PolarChanged);

  Update();
  bool result = (dialog->ShowModal() == mrOK);
  if (result) {
    UpdatePlane();
    _plane = plane;
  }

  delete dialog;

  return result;
}

