// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PlaneDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Listener.hpp"
#include "Plane/Plane.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Computer/Settings.hpp"
#include "UIGlobals.hpp"

class PlaneEditWidget final
  : public RowFormWidget, DataFieldListener {
  enum Controls {
    REGISTRATION,
    COMPETITION_ID,
    POLAR,
    TYPE,
    HANDICAP,
    WING_AREA,
    EMPTY_MASS,
    MAX_BALLAST,
    DUMP_TIME,
    MAX_SPEED,
    WEGLIDE_ID,
  };

  WndForm *dialog;

  Plane plane;

public:
  PlaneEditWidget(const Plane &_plane, const DialogLook &_look,
                  WndForm *_dialog) noexcept
    :RowFormWidget(_look), dialog(_dialog), plane(_plane) {}

  const Plane &GetValue() const noexcept {
    return plane;
  }

  void UpdateCaption() noexcept;
  void UpdatePolarButton() noexcept;
  void PolarButtonClicked() noexcept;

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
PlaneEditWidget::UpdateCaption() noexcept
{
  if (dialog == nullptr)
    return;

  StaticString<128> tmp;
  tmp.Format("%s: %s", _("Plane Details"), GetValueString(REGISTRATION));
  dialog->SetCaption(tmp);
}

void
PlaneEditWidget::UpdatePolarButton() noexcept
{
  const char *caption = _("Polar");
  StaticString<64> buffer;
  if (!plane.polar_name.empty()) {
    buffer.Format("%s: %s", caption, plane.polar_name.c_str());
    caption = buffer;
  }

  Button &polar_button = (Button &)GetRow(POLAR);
  polar_button.SetCaption(caption);
}

void
PlaneEditWidget::OnModified(DataField &df) noexcept
{
  if (IsDataField(REGISTRATION, df))
    UpdateCaption();
}

void
PlaneEditWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddText(_("Registration"), nullptr, plane.registration, this);
  AddText(_("Comp. ID"), nullptr, plane.competition_id);
  AddButton(_("Polar"), [this](){ PolarButtonClicked(); });
  AddText(_("Type"), nullptr, plane.type);
  AddInteger(_("Handicap"), nullptr,
             "%u %%", "%u",
             50, 150, 1,
             plane.handicap);
  AddFloat(_("Wing Area"), nullptr,
           "%.1f m²", "%.1f",
           0, 40, 0.1,
           false, plane.wing_area);
  AddFloat(_("Empty Mass"), _("Net mass of the rigged plane."),
           "%.0f %s", "%.0f",
           0, 1000, 5, false,
           UnitGroup::MASS, plane.empty_mass);
  AddFloat(_("Max. Ballast"), nullptr,
           "%.0f l", "%.0f",
           0, 500, 5,
           false, plane.max_ballast);
  AddInteger(_("Dump Time"), nullptr,
             "%u s", "%u",
             10, 300, 5,
             plane.dump_time);
  AddFloat(_("Max. Cruise Speed"), nullptr,
           "%.0f %s", "%.0f", 0, 300, 5,
           false, UnitGroup::HORIZONTAL_SPEED, plane.max_speed);

  /* TODO: this should be a select list from
     https://api.weglide.org/v1/aircraft */
  if (CommonInterface::GetComputerSettings().weglide.enabled)
    AddInteger(_("WeGlide Type"), nullptr, "%d", "%d", 1, 999,
               1, plane.weglide_glider_type);
  else
    AddDummy();

  UpdateCaption();
  UpdatePolarButton();
}

bool
PlaneEditWidget::Save(bool &_changed) noexcept
{
  bool changed = false;

  changed |= SaveValue(REGISTRATION, plane.registration);
  changed |= SaveValue(COMPETITION_ID, plane.competition_id);
  changed |= SaveValue(TYPE, plane.type);
  changed |= SaveValueInteger(HANDICAP, plane.handicap);
  changed |= SaveValue(WING_AREA, plane.wing_area);
  changed |= SaveValue(EMPTY_MASS, UnitGroup::MASS, plane.empty_mass);
  changed |= SaveValue(MAX_BALLAST, plane.max_ballast);
  changed |= SaveValueInteger(DUMP_TIME, plane.dump_time);
  changed |= SaveValue(MAX_SPEED, UnitGroup::HORIZONTAL_SPEED,
                       plane.max_speed);

  if (CommonInterface::GetComputerSettings().weglide.enabled)
    changed |= SaveValueInteger(WEGLIDE_ID, plane.weglide_glider_type);

  _changed |= changed;
  return true;
}

inline void
PlaneEditWidget::PolarButtonClicked() noexcept
{
  bool changed = false;
  if (!Save(changed))
    return;

  dlgPlanePolarShowModal(plane);
  UpdatePolarButton();
  if (plane.polar_name != "Custom")
    LoadValue(TYPE, plane.polar_name.c_str());

  /* reload attributes that may have been modified */
  LoadValue(HANDICAP, plane.handicap);
  LoadValue(WING_AREA, plane.wing_area);
  LoadValue(EMPTY_MASS, plane.empty_mass, UnitGroup::MASS);
  LoadValue(MAX_BALLAST, plane.max_ballast);
  LoadValue(MAX_SPEED, plane.max_speed, UnitGroup::HORIZONTAL_SPEED);
}

bool
dlgPlaneDetailsShowModal(Plane &_plane) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<PlaneEditWidget>
    dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
           look, _("Plane Details"));
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.SetWidget(_plane, look, &dialog);
  const int result = dialog.ShowModal();

  if (result != mrOK)
    return false;

  _plane = dialog.GetWidget().GetValue();
  return true;
}
