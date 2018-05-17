/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/FilePicker.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Form/DataField/Listener.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Screen/Color.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarStore.hpp"
#include "Polar/PolarFileGlue.hpp"
#include "Plane/Plane.hpp"
#include "OS/Path.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

class PlanePolarWidget final
  : public RowFormWidget, DataFieldListener, ActionListener {
  enum Controls {
    NAME,
    INVALID,
    SHAPE,
    REFERENCE_MASS,
    DRY_MASS,
  };

  enum Actions {
    LIST,
    IMPORT,
  };

  Plane plane;

public:
  PlanePolarWidget(const Plane &_plane, const DialogLook &_look)
    :RowFormWidget(_look), plane(_plane) {}

  const Plane &GetValue() const {
    return plane;
  }

  void CreateButtons(WidgetDialog &buttons) {
    buttons.AddButton(_("List"), *this, LIST);
    buttons.AddButton(_("Import"), *this, IMPORT);
  }

private:
  PolarShapeEditWidget &GetShapeEditor() {
    return (PolarShapeEditWidget &)GetRowWidget(SHAPE);
  }

  void LoadPolarShape(const PolarShape &shape) {
    GetShapeEditor().SetPolarShape(shape);
  }

  void UpdatePolarLabel() {
    SetText(NAME, plane.polar_name);
  }

  void UpdateInvalidLabel();
  void Update();

  void ListClicked();
  void ImportClicked();

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual void Show(const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;

  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;
};

void
PlanePolarWidget::UpdateInvalidLabel()
{
  PolarShapeEditWidget &widget = GetShapeEditor();
  bool changed = false;
  bool valid = widget.Save(changed) &&
    widget.GetPolarShape().IsValid();
  bool visible = !valid;

  SetRowVisible(INVALID, visible);

  if (visible) {
    TextWidget &widget = (TextWidget &)GetRowWidget(INVALID);
    widget.SetText(_("Invalid"));
    widget.SetColor(COLOR_RED);
  }
}

void
PlanePolarWidget::Update()
{
  LoadPolarShape(plane.polar_shape);
  UpdatePolarLabel();

  LoadValue(REFERENCE_MASS, plane.reference_mass, UnitGroup::MASS);
  LoadValue(DRY_MASS, plane.dry_mass, UnitGroup::MASS);
}

void
PlanePolarWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddReadOnly(_("Name"), nullptr, plane.polar_name);

  Add(new TextWidget());
  SetRowVisible(INVALID, false);

  Add(new PolarShapeEditWidget(plane.polar_shape, this));

  AddFloat(_("Reference Mass"), _("Reference mass of the polar"),
           _T("%.0f %s"), _T("%.0f"),
           0, 1000, 5, false,
           UnitGroup::MASS, plane.reference_mass);

  AddFloat(_("Dry Mass"), _("Dry all-up flying mass of your plane"),
           _T("%.0f %s"), _T("%.0f"),
           0, 1000, 5, false,
           UnitGroup::MASS, plane.dry_mass);
}

void
PlanePolarWidget::Show(const PixelRect &rc)
{
  RowFormWidget::Show(rc);
  UpdateInvalidLabel();
}

bool
PlanePolarWidget::Save(bool &_changed)
{
  bool changed = false;

  PolarShapeEditWidget &widget = GetShapeEditor();
  if (widget.Save(changed)) {
    if (widget.GetPolarShape().IsValid())
      plane.polar_shape = widget.GetPolarShape();
  }

  changed |= SaveValue(REFERENCE_MASS, UnitGroup::MASS, plane.reference_mass);
  changed |= SaveValue(DRY_MASS, UnitGroup::MASS, plane.dry_mass);

  _changed |= changed;
  return true;
}

inline void
PlanePolarWidget::ListClicked()
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

  const PolarStore::Item &item = PolarStore::GetItem(list[result].int_value);

  plane.reference_mass = item.reference_mass;
  plane.dry_mass = item.reference_mass;
  plane.max_ballast = item.max_ballast;

  if (item.wing_area > 0.0)
    plane.wing_area = item.wing_area;

  if (item.v_no > 0.0)
    plane.max_speed = item.v_no;

  plane.polar_shape = item.ToPolarShape();

  plane.polar_name = list[result].string_value.c_str();

  if (item.contest_handicap > 0)
    plane.handicap = item.contest_handicap;

  Update();
}

inline void
PlanePolarWidget::ImportClicked()
{
  // let the user select
  const auto path = FilePicker(_("Load Polar From File"), _T("*.plr\0"));
  if (path == nullptr)
    return;

  PolarInfo polar;
  try {
    PolarGlue::LoadFromFile(polar, path);
  } catch (const std::runtime_error &) {
  }

  plane.reference_mass = polar.reference_mass;
  plane.dry_mass = polar.reference_mass;
  plane.max_ballast = polar.max_ballast;

  if (polar.wing_area > 0)
    plane.wing_area = polar.wing_area;

  if (polar.v_no > 0)
    plane.max_speed = polar.v_no;

  plane.polar_shape = polar.shape;

  plane.polar_name = path.GetBase().c_str();

  Update();
}

void
PlanePolarWidget::OnAction(int id)
{
  switch (id) {
  case LIST:
    ListClicked();
    break;

  case IMPORT:
    ImportClicked();
    break;
  }
}

void
PlanePolarWidget::OnModified(DataField &df)
{
  plane.polar_name = _T("Custom");
  UpdatePolarLabel();
  UpdateInvalidLabel();
}

bool
dlgPlanePolarShowModal(Plane &_plane)
{
  StaticString<128> caption;
  caption.Format(_T("%s: %s"), _("Plane Polar"), _plane.registration.c_str());

  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  PlanePolarWidget widget(_plane, look);
  dialog.CreateAuto(UIGlobals::GetMainWindow(), caption, &widget);
  widget.CreateButtons(dialog);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);
  const int result = dialog.ShowModal();
  dialog.StealWidget();

  if (result != mrOK)
    return false;

  _plane = widget.GetValue();
  return true;
}
