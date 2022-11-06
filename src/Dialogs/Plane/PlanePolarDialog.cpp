/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
#include "ui/canvas/Color.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarStore.hpp"
#include "Polar/PolarFileGlue.hpp"
#include "Plane/Plane.hpp"
#include "system/Path.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "util/ConvertString.hpp"

class PlanePolarWidget final
  : public RowFormWidget, DataFieldListener {
  enum Controls {
    NAME,
    INVALID,
    SHAPE,
    REFERENCE_MASS,
  };

  Plane plane;

public:
  PlanePolarWidget(const Plane &_plane, const DialogLook &_look)
    :RowFormWidget(_look), plane(_plane) {}

  const Plane &GetValue() const {
    return plane;
  }

  void CreateButtons(WidgetDialog &buttons) {
    buttons.AddButton(_("List"), [this](){ ListClicked(); });
    buttons.AddButton(_("Import"), [this](){ ImportClicked(); });
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
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
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

  LoadValue(REFERENCE_MASS, plane.polar_shape.reference_mass, UnitGroup::MASS);
}

void
PlanePolarWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                          [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddReadOnly(_("Name"), nullptr, plane.polar_name);

  Add(std::make_unique<TextWidget>());
  SetRowVisible(INVALID, false);

  DataFieldListener *listener = this;
  Add(std::make_unique<PolarShapeEditWidget>(plane.polar_shape, listener));

  AddFloat(_("Reference Mass"), _("Reference mass of the polar."),
           _T("%.0f %s"), _T("%.0f"),
           0, 1000, 5, false,
           UnitGroup::MASS, plane.polar_shape.reference_mass);
}

void
PlanePolarWidget::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);
  UpdateInvalidLabel();
}

bool
PlanePolarWidget::Save(bool &_changed) noexcept
{
  bool changed = false;

  PolarShapeEditWidget &widget = GetShapeEditor();
  if (widget.Save(changed)) {
    if (widget.GetPolarShape().IsValid())
      plane.polar_shape = widget.GetPolarShape();
  }

  changed |= SaveValue(REFERENCE_MASS, UnitGroup::MASS, plane.polar_shape.reference_mass);

  _changed |= changed;
  return true;
}

inline void
PlanePolarWidget::ListClicked()
{
  const auto internal_polars = PolarStore::GetAll();
  ComboList list;
  for (const auto &i : internal_polars)
    list.Append(i.name);

  // let the user select
  int result = ComboPicker(_("Load Polar"), list, NULL);
  if (result < 0)
    return;

  const PolarStore::Item &item = internal_polars[list[result].int_value];

  plane.polar_shape.reference_mass = item.reference_mass;
  plane.empty_mass = item.empty_mass;
  plane.max_ballast = item.max_ballast;

  if (item.wing_area > 0.0)
    plane.wing_area = item.wing_area;

  if (item.v_no > 0.0)
    plane.max_speed = item.v_no;

  plane.polar_shape = item.ToPolarShape();

  plane.polar_name = item.name;

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
  } catch (...) {
  }

  plane.polar_shape.reference_mass = polar.shape.reference_mass;
  // plane.empty_mass = polar;
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
PlanePolarWidget::OnModified([[maybe_unused]] DataField &df) noexcept
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
  TWidgetDialog<PlanePolarWidget>
    dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(), look, caption);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.SetWidget(_plane, look);
  dialog.GetWidget().CreateButtons(dialog);
  const int result = dialog.ShowModal();

  if (result != mrOK)
    return false;

  _plane = dialog.GetWidget().GetValue();
  return true;
}
