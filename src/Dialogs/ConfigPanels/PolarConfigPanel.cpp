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

#include "PolarConfigPanel.hpp"
#include "ConfigPanel.hpp"
#include "UIGlobals.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Form/DataField/FileReader.hpp"
#include "Form/XMLWidget.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Form/Form.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Polar/PolarStore.hpp"
#include "Polar/PolarFileGlue.hpp"
#include "Polar/PolarGlue.hpp"
#include "Polar/Polar.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "OS/FileUtil.hpp"
#include "Language/Language.hpp"
#include "Plane/PlaneGlue.hpp"
#include "Units/Units.hpp"

#include <cstdio>
#include <windef.h> /* for MAX_PATH */

class PolarConfigPanel : public XMLWidget {
  bool loading;
  WndButton *buttonList, *buttonImport, *buttonExport;

  void SetLiftFieldStepAndMax(const TCHAR *control);
  void UpdatePolarPoints(fixed v1, fixed v2, fixed v3,
                         fixed w1, fixed w2, fixed w3);
  void UpdatePolarTitle();
  void UpdatePolarInvalidLabel();
  void SetVisible(bool active);

public:
  void LoadInternal();
  void LoadFromFile();
  void Export();
  void DataChanged();

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static PolarConfigPanel *instance;

void
PolarConfigPanel::SetLiftFieldStepAndMax(const TCHAR *control)
{
  WndProperty *ctl = (WndProperty *)form.FindByName(control);
  DataFieldFloat* df = (DataFieldFloat*)ctl->GetDataField();
  switch (Units::current.vertical_speed_unit) {
    case Unit::FEET_PER_MINUTE:
      df->SetStep(fixed_ten);
      df->SetMin(fixed(-2000));
      break;
    case Unit::KNOTS:
      df->SetStep(fixed(0.1));
      df->SetMin(fixed(-20));
      break;
    case Unit::METER_PER_SECOND:
      df->SetStep(fixed(0.05));
      df->SetMin(fixed(-10));
      break;
    default:
      break;
  }
}

void
PolarConfigPanel::UpdatePolarPoints(fixed v1, fixed v2, fixed v3,
                                    fixed w1, fixed w2, fixed w3)
{
  LoadFormProperty(form, _T("prpPolarV1"), UnitGroup::HORIZONTAL_SPEED, v1);
  LoadFormProperty(form, _T("prpPolarV2"), UnitGroup::HORIZONTAL_SPEED, v2);
  LoadFormProperty(form, _T("prpPolarV3"), UnitGroup::HORIZONTAL_SPEED, v3);

  LoadFormProperty(form, _T("prpPolarW1"), UnitGroup::VERTICAL_SPEED, w1);
  LoadFormProperty(form, _T("prpPolarW2"), UnitGroup::VERTICAL_SPEED, w2);
  LoadFormProperty(form, _T("prpPolarW3"), UnitGroup::VERTICAL_SPEED, w3);
}

void
PolarConfigPanel::UpdatePolarTitle()
{
  StaticString<100> caption(_("Polar"));
  caption += _T(": ");
  caption += CommonInterface::GetComputerSettings().plane.polar_name;

  ((WndFrame *)form.FindByName(_T("lblPolar")))->SetCaption(caption);
}

static bool
SaveFormToPolar(SubForm &form, PolarInfo &polar)
{
  bool changed = SaveFormProperty(form, _T("prpPolarV1"), UnitGroup::HORIZONTAL_SPEED, polar.v1);
  changed |= SaveFormProperty(form, _T("prpPolarV2"), UnitGroup::HORIZONTAL_SPEED, polar.v2);
  changed |= SaveFormProperty(form, _T("prpPolarV3"), UnitGroup::HORIZONTAL_SPEED, polar.v3);

  changed |= SaveFormProperty(form, _T("prpPolarW1"), UnitGroup::VERTICAL_SPEED, polar.w1);
  changed |= SaveFormProperty(form, _T("prpPolarW2"), UnitGroup::VERTICAL_SPEED, polar.w2);
  changed |= SaveFormProperty(form, _T("prpPolarW3"), UnitGroup::VERTICAL_SPEED, polar.w3);

  changed |= SaveFormProperty(form, _T("prpPolarReferenceMass"), polar.reference_mass);
  changed |= SaveFormProperty(form, _T("prpPolarMaxBallast"), polar.max_ballast);

  changed |= SaveFormProperty(form, _T("prpPolarWingArea"), polar.wing_area);
  changed |= SaveFormProperty(form, _T("prpMaxManoeuveringSpeed"),
                              UnitGroup::HORIZONTAL_SPEED, polar.v_no);

  return changed;
}

void
PolarConfigPanel::UpdatePolarInvalidLabel()
{
  fixed v1 = GetFormValueFixed(form, _T("prpPolarV1"));
  fixed v2 = GetFormValueFixed(form, _T("prpPolarV2"));
  fixed v3 = GetFormValueFixed(form, _T("prpPolarV3"));
  fixed w1 = GetFormValueFixed(form, _T("prpPolarW1"));
  fixed w2 = GetFormValueFixed(form, _T("prpPolarW2"));
  fixed w3 = GetFormValueFixed(form, _T("prpPolarW3"));

  PolarCoefficients coeff = PolarCoefficients::From3VW(v1, v2, v3, w1, w2, w3);
  ((WndFrame *)form.FindByName(_T("lblPolarInvalid")))->SetVisible(!coeff.IsValid());
}

void
PolarConfigPanel::LoadInternal()
{
  ComboList list;
  unsigned len = PolarStore::Count();
  for (unsigned i = 0; i < len; i++)
    list.Append(i, PolarStore::GetItem(i).name);

  list.Sort();

  /* let the user select */

  int result = ComboPicker(UIGlobals::GetMainWindow(), _("Load Polar"), list, NULL);
  if (result >= 0) {
    const PolarStore::Item &item = PolarStore::GetItem(list[result].DataFieldIndex);

    UpdatePolarPoints(Units::ToSysUnit(fixed(item.v1), Unit::KILOMETER_PER_HOUR),
                      Units::ToSysUnit(fixed(item.v2), Unit::KILOMETER_PER_HOUR),
                      Units::ToSysUnit(fixed(item.v3), Unit::KILOMETER_PER_HOUR),
                      fixed(item.w1), fixed(item.w2), fixed(item.w3));

    LoadFormProperty(form, _T("prpPolarReferenceMass"), fixed(item.reference_mass));
    LoadFormProperty(form, _T("prpPolarDryMass"), fixed(item.reference_mass));
    LoadFormProperty(form, _T("prpPolarMaxBallast"), fixed(item.max_ballast));

    if (item.wing_area > 0.0)
      LoadFormProperty(form, _T("prpPolarWingArea"), fixed(item.wing_area));

    if (item.v_no > 0.0)
      LoadFormProperty(form, _T("prpMaxManoeuveringSpeed"), UnitGroup::HORIZONTAL_SPEED,
                       fixed(item.v_no));

    if (item.contest_handicap > 0)
      LoadFormProperty(form, _T("prpHandicap"), item.contest_handicap);

    CommonInterface::SetComputerSettings().plane.polar_name = item.name;
    UpdatePolarTitle();
    UpdatePolarInvalidLabel();
  }
}

static void
OnLoadInternal(WndButton &button)
{
  instance->LoadInternal();
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

void
PolarConfigPanel::LoadFromFile()
{
  ComboList list;
  PolarFileVisitor fv(list);

  // Fill list
  VisitDataFiles(_T("*.plr"), fv);

  // Sort list
  list.Sort();

  // Show selection dialog
  int result = ComboPicker(UIGlobals::GetMainWindow(),
                           _("Load Polar From File"), list, NULL);
  if (result >= 0) {
    const TCHAR* path = list[result].StringValue;
    PolarInfo polar;
    PolarGlue::LoadFromFile(polar, path);

    UpdatePolarPoints(polar.v1, polar.v2, polar.v3, polar.w1, polar.w2, polar.w3);

    LoadFormProperty(form, _T("prpPolarReferenceMass"), polar.reference_mass);
    LoadFormProperty(form, _T("prpPolarDryMass"), polar.reference_mass);
    LoadFormProperty(form, _T("prpPolarMaxBallast"), polar.max_ballast);

    if (positive(polar.wing_area))
      LoadFormProperty(form, _T("prpPolarWingArea"), polar.wing_area);

    if (positive(polar.v_no))
      LoadFormProperty(form, _T("prpMaxManoeuveringSpeed"), UnitGroup::HORIZONTAL_SPEED,
                       polar.v_no);

    CommonInterface::SetComputerSettings().plane.polar_name =
        list[result].StringValueFormatted;
    UpdatePolarTitle();
    UpdatePolarInvalidLabel();
  }
}

static void
OnLoadFromFile(WndButton &button)
{
  instance->LoadFromFile();
}

void
PolarConfigPanel::Export()
{
  TCHAR filename[69] = _T("");
  if (!dlgTextEntryShowModal(*(SingleWindow *)GetWindow()->GetRootOwner(),
                             filename, 64, _("Polar name")))
    return;

  TCHAR path[MAX_PATH];
  _tcscat(filename, _T(".plr"));
  LocalPath(path, filename);

  PolarInfo polar;
  SaveFormToPolar(form, polar);
  if (PolarGlue::SaveToFile(polar, path)) {
    CommonInterface::SetComputerSettings().plane.polar_name = filename;
    UpdatePolarTitle();
  }
}

static void
OnExport(WndButton &button)
{
  instance->Export();
}

void
PolarConfigPanel::DataChanged()
{
  if (!loading)
    CommonInterface::SetComputerSettings().plane.polar_name = _T("Custom");

  UpdatePolarTitle();
  UpdatePolarInvalidLabel();
}

static void
OnFieldData(DataField *Sender, DataField::DataAccessMode Mode)
{
  if (Mode == DataField::daChange)
    instance->DataChanged();
}

void
PolarConfigPanel::SetVisible(bool active)
{
  if (buttonList != NULL)
    buttonList->SetVisible(active);

  if (buttonImport != NULL)
    buttonImport->SetVisible(active);

  if (buttonExport != NULL)
    buttonExport->SetVisible(active);
}

void
PolarConfigPanel::Show(const PixelRect &rc)
{
  PolarConfigPanel::SetVisible(true);
  XMLWidget::Show(rc);
}

void
PolarConfigPanel::Hide()
{
  XMLWidget::Hide();
  PolarConfigPanel::SetVisible(false);
}

static gcc_constexpr_data CallBackTableEntry polar_callbacks[] = {
  DeclareCallBackEntry(OnFieldData),
  DeclareCallBackEntry(NULL)
};

void
PolarConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;
  loading = true;

  LoadWindow(polar_callbacks, parent,
             Layout::landscape
             ? _T("IDR_XML_POLARCONFIGPANEL_L")
             : _T("IDR_XML_POLARCONFIGPANEL"));

  buttonList = (WndButton *)ConfigPanel::GetForm()
    .FindByName(_T("cmdLoadInternalPolar"));
  assert(buttonList != NULL);
  buttonList->SetOnClickNotify(OnLoadInternal);

  buttonImport = (WndButton *)ConfigPanel::GetForm()
    .FindByName(_T("cmdLoadPolarFile"));
  assert(buttonImport != NULL);
  buttonImport->SetOnClickNotify(OnLoadFromFile);

  buttonExport = (WndButton *)ConfigPanel::GetForm()
    .FindByName(_T("cmdSavePolarFile"));
  assert(buttonExport != NULL);
  buttonExport->SetOnClickNotify(OnExport);

  SetLiftFieldStepAndMax(_T("prpPolarW1"));
  SetLiftFieldStepAndMax(_T("prpPolarW2"));
  SetLiftFieldStepAndMax(_T("prpPolarW3"));

  const ComputerSettings &settings = XCSoarInterface::GetComputerSettings();
  UpdatePolarPoints(settings.plane.v1, settings.plane.v2, settings.plane.v3,
                    settings.plane.w1, settings.plane.w2, settings.plane.w3);

  LoadFormProperty(form, _T("prpPolarReferenceMass"),
                   settings.plane.reference_mass);
  LoadFormProperty(form, _T("prpPolarDryMass"), settings.plane.dry_mass);
  LoadFormProperty(form, _T("prpPolarMaxBallast"), settings.plane.max_ballast);

  LoadFormProperty(form, _T("prpPolarWingArea"), settings.plane.wing_area);
  LoadFormProperty(form, _T("prpMaxManoeuveringSpeed"), UnitGroup::HORIZONTAL_SPEED,
                   settings.plane.max_speed);

  UpdatePolarTitle();
  UpdatePolarInvalidLabel();

  const ComputerSettings &settings_computer = XCSoarInterface::GetComputerSettings();

  LoadFormProperty(form, _T("prpHandicap"),
                   settings_computer.plane.handicap);

  LoadFormProperty(form, _T("prpBallastSecsToEmpty"),
                   settings_computer.plane.dump_time);

  loading = false;
}

bool
PolarConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false;

  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();

  changed |= SaveFormProperty(form, _T("prpPolarV1"), UnitGroup::HORIZONTAL_SPEED,
                              settings_computer.plane.v1);
  changed |= SaveFormProperty(form, _T("prpPolarV2"), UnitGroup::HORIZONTAL_SPEED,
                              settings_computer.plane.v2);
  changed |= SaveFormProperty(form, _T("prpPolarV3"), UnitGroup::HORIZONTAL_SPEED,
                              settings_computer.plane.v3);

  changed |= SaveFormProperty(form, _T("prpPolarW1"), UnitGroup::VERTICAL_SPEED,
                              settings_computer.plane.w1);
  changed |= SaveFormProperty(form, _T("prpPolarW2"), UnitGroup::VERTICAL_SPEED,
                              settings_computer.plane.w2);
  changed |= SaveFormProperty(form, _T("prpPolarW3"), UnitGroup::VERTICAL_SPEED,
                              settings_computer.plane.w3);

  changed |= SaveFormProperty(form, _T("prpPolarReferenceMass"),
                              settings_computer.plane.reference_mass);

  changed |= SaveFormProperty(form, _T("prpPolarDryMass"),
                              settings_computer.plane.dry_mass);

  changed |= SaveFormProperty(form, _T("prpPolarMaxBallast"),
                              settings_computer.plane.max_ballast);

  changed |= SaveFormProperty(form, _T("prpPolarWingArea"),
                              settings_computer.plane.wing_area);

  changed |= SaveFormProperty(form, _T("prpMaxManoeuveringSpeed"),
                              UnitGroup::HORIZONTAL_SPEED,
                              settings_computer.plane.max_speed);

  changed |= SaveFormProperty(form, _T("prpHandicap"),
                              settings_computer.plane.handicap);

  changed |= SaveFormProperty(form, _T("prpBallastSecsToEmpty"),
                              settings_computer.plane.dump_time);

  if (changed) {
    PlaneGlue::DetachFromPlaneFile();
    PlaneGlue::ToProfile(settings_computer.plane);
    PlaneGlue::Synchronize(settings_computer.plane, settings_computer,
                           settings_computer.polar.glide_polar_task);

    if (protected_task_manager != NULL)
      protected_task_manager->SetGlidePolar(settings_computer.polar.glide_polar_task);
  }

  _changed |= changed;
  return true;
}

Widget *
CreatePolarConfigPanel()
{
  return new PolarConfigPanel();
}
