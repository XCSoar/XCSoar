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
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Listener.hpp"
#include "Plane/Plane.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

class PlaneEditWidget final
  : public RowFormWidget, DataFieldListener, ActionListener {
  enum Controls {
    REGISTRATION,
    COMPETITION_ID,
    POLAR,
    TYPE,
    HANDICAP,
    WING_AREA,
    MAX_BALLAST,
    DUMP_TIME,
    MAX_SPEED,
  };

  WndForm *dialog;

  Plane plane;

public:
  PlaneEditWidget(const Plane &_plane, const DialogLook &_look,
                  WndForm *_dialog)
    :RowFormWidget(_look), dialog(_dialog), plane(_plane) {}

  const Plane &GetValue() const {
    return plane;
  }

  void UpdateCaption();
  void UpdatePolarButton();
  void PolarButtonClicked();

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;

  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;
};

void
PlaneEditWidget::UpdateCaption()
{
  if (dialog == nullptr)
    return;

  StaticString<128> tmp;
  tmp.Format(_T("%s: %s"), _("Plane Details"), GetValueString(REGISTRATION));
  dialog->SetCaption(tmp);
}

void
PlaneEditWidget::UpdatePolarButton()
{
  const TCHAR *caption = _("Polar");
  StaticString<64> buffer;
  if (!plane.polar_name.empty()) {
    buffer.Format(_T("%s: %s"), caption, plane.polar_name.c_str());
    caption = buffer;
  }

  Button &polar_button = (Button &)GetRow(POLAR);
  polar_button.SetCaption(caption);
}

void
PlaneEditWidget::OnModified(DataField &df)
{
  if (IsDataField(REGISTRATION, df))
    UpdateCaption();
}

void
PlaneEditWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddText(_("Registration"), nullptr, plane.registration, this);
  AddText(_("Comp. ID"), nullptr, plane.competition_id);
  AddButton(_("Polar"), *this, POLAR);
  AddText(_("Type"), nullptr, plane.type);
  AddInteger(_("Handicap"), nullptr,
             _T("%u %%"), _T("%u"),
             50, 150, 1,
             plane.handicap);
  AddFloat(_("Wing Area"), nullptr,
           _T("%.1f m²"), _T("%.1f"),
           0, 40, 0.1,
           false, plane.wing_area);
  AddFloat(_("Max. Ballast"), nullptr,
           _T("%.0f l"), _T("%.0f"),
           0, 500, 5,
           false, plane.max_ballast);
  AddInteger(_("Dump Time"), nullptr,
             _T("%u s"), _T("%u"),
             10, 300, 5,
             plane.dump_time);
  AddFloat(_("Max. Cruise Speed"), nullptr,
           _T("%.0f %s"), _T("%.0f"), 0, 300, 5,
           false, UnitGroup::HORIZONTAL_SPEED, plane.max_speed);

  UpdateCaption();
  UpdatePolarButton();
}

bool
PlaneEditWidget::Save(bool &_changed)
{
  bool changed = false;

  changed |= SaveValue(REGISTRATION, plane.registration);
  changed |= SaveValue(COMPETITION_ID, plane.competition_id);
  changed |= SaveValue(TYPE, plane.type);
  changed |= SaveValue(HANDICAP, plane.handicap);
  changed |= SaveValue(WING_AREA, plane.wing_area);
  changed |= SaveValue(MAX_BALLAST, plane.max_ballast);
  changed |= SaveValue(DUMP_TIME, plane.dump_time);
  changed |= SaveValue(MAX_SPEED, UnitGroup::HORIZONTAL_SPEED,
                       plane.max_speed);

  _changed |= changed;
  return true;
}

void
PlaneEditWidget::OnAction(int id)
{
  switch (id) {
  case POLAR:
    PolarButtonClicked();
    break;
  }
}

inline void
PlaneEditWidget::PolarButtonClicked()
{
  bool changed = false;
  if (!Save(changed))
    return;

  dlgPlanePolarShowModal(plane);
  UpdatePolarButton();
  if (plane.polar_name != _T("Custom"))
    LoadValue(TYPE, plane.polar_name.c_str());

  /* reload attributes that may have been modified */
  LoadValue(WING_AREA, plane.wing_area);
  LoadValue(MAX_BALLAST, plane.max_ballast);
  LoadValue(MAX_SPEED, plane.max_speed, UnitGroup::HORIZONTAL_SPEED);
}

bool
dlgPlaneDetailsShowModal(Plane &_plane)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  PlaneEditWidget widget(_plane, look, &dialog);
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _("Plane Details"), &widget);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);
  const int result = dialog.ShowModal();
  dialog.StealWidget();

  if (result != mrOK)
    return false;

  _plane = widget.GetValue();
  return true;
}
