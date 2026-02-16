// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
  PlanePolarWidget(const Plane &_plane, const DialogLook &_look) noexcept
    :RowFormWidget(_look), plane(_plane) {}

  const Plane &GetValue() const noexcept {
    return plane;
  }

  void CreateButtons(WidgetDialog &buttons) noexcept {
    buttons.AddButton(_("List"), [this](){ ListClicked(); });
    buttons.AddButton(_("Import"), [this](){ ImportClicked(); });
  }

private:
  PolarShapeEditWidget &GetShapeEditor() noexcept {
    return (PolarShapeEditWidget &)GetRowWidget(SHAPE);
  }

  void LoadPolarShape(const PolarShape &shape) noexcept {
    GetShapeEditor().SetPolarShape(shape);
  }

  void UpdatePolarLabel() noexcept {
    SetText(NAME, plane.polar_name);
  }

  void UpdateInvalidLabel() noexcept;
  void Update() noexcept;

  void ListClicked() noexcept;
  void ImportClicked() noexcept;

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
PlanePolarWidget::UpdateInvalidLabel() noexcept
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
PlanePolarWidget::Update() noexcept
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
           "%.0f %s", "%.0f",
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
PlanePolarWidget::ListClicked() noexcept
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
PlanePolarWidget::ImportClicked() noexcept
{
  // let the user select
  const auto path = FilePicker(_("Load Polar From File"), "*.plr\0");
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
  plane.polar_name = "Custom";
  UpdatePolarLabel();
  UpdateInvalidLabel();
}

bool
dlgPlanePolarShowModal(Plane &_plane) noexcept
{
  StaticString<128> caption;
  caption.Format("%s: %s", _("Plane Polar"), _plane.registration.c_str());

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
