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

#include "PolarConfigPanel.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Float.hpp"
#include "DataField/ComboList.hpp"
#include "DataField/FileReader.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Screen/SingleWindow.hpp"
#include "Polar/PolarStore.hpp"
#include "Polar/PolarFileGlue.hpp"
#include "Polar/PolarGlue.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarCoefficients.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "MainWindow.hpp"
#include "OS/FileUtil.hpp"
#include "Language/Language.hpp"
#include "Plane/PlaneGlue.hpp"

#include <cstdio>

static WndForm* wf = NULL;
static WndButton* buttonList = NULL;
static WndButton* buttonImport = NULL;
static WndButton* buttonExport = NULL;
static bool loading = false;

static void
SetLiftFieldStepAndMax(const TCHAR *control)
{
  WndProperty *ctl = (WndProperty *)wf->FindByName(control);
  DataFieldFloat* df = (DataFieldFloat*)ctl->GetDataField();
  switch (Units::Current.VerticalSpeedUnit) {
    case unFeetPerMinute:
      df->SetStep(fixed_ten);
      df->SetMin(fixed(-2000));
      break;
    case unKnots:
      df->SetStep(fixed(0.1));
      df->SetMin(fixed(-20));
      break;
    case unMeterPerSecond:
      df->SetStep(fixed(0.05));
      df->SetMin(fixed(-10));
      break;
    default:
      break;
  }
}

static void
UpdatePolarPoints(fixed v1, fixed v2, fixed v3,
                  fixed w1, fixed w2, fixed w3)
{
  LoadFormProperty(*wf, _T("prpPolarV1"), ugHorizontalSpeed, v1);
  LoadFormProperty(*wf, _T("prpPolarV2"), ugHorizontalSpeed, v2);
  LoadFormProperty(*wf, _T("prpPolarV3"), ugHorizontalSpeed, v3);

  LoadFormProperty(*wf, _T("prpPolarW1"), ugVerticalSpeed, w1);
  LoadFormProperty(*wf, _T("prpPolarW2"), ugVerticalSpeed, w2);
  LoadFormProperty(*wf, _T("prpPolarW3"), ugVerticalSpeed, w3);
}

static void
UpdatePolarFields(const PolarInfo &polar)
{
  UpdatePolarPoints(polar.v1, polar.v2, polar.v3, polar.w1, polar.w2, polar.w3);

  LoadFormProperty(*wf, _T("prpPolarReferenceMass"), polar.reference_mass);
  LoadFormProperty(*wf, _T("prpPolarDryMass"),
                   (positive(polar.dry_mass) ? polar.dry_mass :
                                               polar.reference_mass));
  LoadFormProperty(*wf, _T("prpPolarMaxBallast"), polar.max_ballast);

  LoadFormProperty(*wf, _T("prpPolarWingArea"), polar.wing_area);

  WndProperty *ctl = (WndProperty *)wf->FindByName(_T("prpMaxManoeuveringSpeed"));
  assert(ctl != NULL);
  Units_t unit = Units::GetUserSpeedUnit();

  DataFieldFloat *df = (DataFieldFloat *)ctl->GetDataField();
  df->SetUnits(Units::GetUnitName(unit));
  if (positive(polar.v_no))
    df->SetAsFloat(Units::ToUserUnit(polar.v_no, unit));

  ctl->RefreshDisplay();
}

static void
UpdatePolarTitle()
{
  StaticString<100> caption(_("Polar"));
  caption += _T(": ");
  caption += CommonInterface::SettingsComputer().plane.polar_name;

  ((WndFrame *)wf->FindByName(_T("lblPolar")))->SetCaption(caption);
}

static bool
SaveFormToPolar(PolarInfo &polar)
{
  bool changed = SaveFormProperty(*wf, _T("prpPolarV1"), ugHorizontalSpeed, polar.v1);
  changed |= SaveFormProperty(*wf, _T("prpPolarV2"), ugHorizontalSpeed, polar.v2);
  changed |= SaveFormProperty(*wf, _T("prpPolarV3"), ugHorizontalSpeed, polar.v3);

  changed |= SaveFormProperty(*wf, _T("prpPolarW1"), ugVerticalSpeed, polar.w1);
  changed |= SaveFormProperty(*wf, _T("prpPolarW2"), ugVerticalSpeed, polar.w2);
  changed |= SaveFormProperty(*wf, _T("prpPolarW3"), ugVerticalSpeed, polar.w3);

  changed |= SaveFormProperty(*wf, _T("prpPolarReferenceMass"), polar.reference_mass);
  changed |= SaveFormProperty(*wf, _T("prpPolarDryMass"), polar.dry_mass);
  changed |= SaveFormProperty(*wf, _T("prpPolarMaxBallast"), polar.max_ballast);

  changed |= SaveFormProperty(*wf, _T("prpPolarWingArea"), polar.wing_area);
  changed |= SaveFormProperty(*wf, _T("prpMaxManoeuveringSpeed"),
                              ugHorizontalSpeed, polar.v_no);

  return changed;
}

static void
UpdatePolarInvalidLabel()
{
  fixed v1, v2, v3;
  fixed w1, w2, w3;
  SaveFormProperty(*wf, _T("prpPolarV1"), ugHorizontalSpeed, v1);
  SaveFormProperty(*wf, _T("prpPolarV2"), ugHorizontalSpeed, v2);
  SaveFormProperty(*wf, _T("prpPolarV3"), ugHorizontalSpeed, v3);
  SaveFormProperty(*wf, _T("prpPolarW1"), ugVerticalSpeed, w1);
  SaveFormProperty(*wf, _T("prpPolarW2"), ugVerticalSpeed, w2);
  SaveFormProperty(*wf, _T("prpPolarW3"), ugVerticalSpeed, w3);

  PolarCoefficients coeff = PolarCoefficients::FromVW(v1, v2, v3, w1, w2, w3);
  ((WndFrame *)wf->FindByName(_T("lblPolarInvalid")))->set_visible(!coeff.IsValid());
}

void
PolarConfigPanel::OnLoadInternal(WndButton &button)
{
  ComboList list;
  unsigned len = PolarStore::Count();
  for (unsigned i = 0; i < len; i++)
    list.Append(i, PolarStore::GetItem(i).name);

  list.Sort();

  /* let the user select */

  int result = ComboPicker(XCSoarInterface::main_window, _("Load Polar"), list, NULL);
  if (result >= 0) {
    const PolarStore::Item &item = PolarStore::GetItem(list[result].DataFieldIndex);

    UpdatePolarPoints(Units::ToSysUnit(fixed(item.v1), unKiloMeterPerHour),
                      Units::ToSysUnit(fixed(item.v2), unKiloMeterPerHour),
                      Units::ToSysUnit(fixed(item.v3), unKiloMeterPerHour),
                      fixed(item.w1), fixed(item.w2), fixed(item.w3));

    LoadFormProperty(*wf, _T("prpPolarReferenceMass"), fixed(item.reference_mass));
    LoadFormProperty(*wf, _T("prpPolarDryMass"), fixed(item.reference_mass));
    LoadFormProperty(*wf, _T("prpPolarMaxBallast"), fixed(item.max_ballast));

    if (item.wing_area > 0.0)
      LoadFormProperty(*wf, _T("prpPolarWingArea"), fixed(item.wing_area));

    if (item.v_no > 0.0)
      LoadFormProperty(*wf, _T("prpMaxManoeuveringSpeed"), ugHorizontalSpeed,
                       fixed(item.v_no));

    if (item.contest_handicap > 0)
      LoadFormProperty(*wf, _T("prpHandicap"), item.contest_handicap);

    CommonInterface::SetSettingsComputer().plane.polar_name = item.name;
    UpdatePolarTitle();
    UpdatePolarInvalidLabel();
  }
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
PolarConfigPanel::OnLoadFromFile(WndButton &button)
{
  ComboList list;
  PolarFileVisitor fv(list);

  // Fill list
  VisitDataFiles(_T("*.plr"), fv);

  // Sort list
  list.Sort();

  // Show selection dialog
  int result = ComboPicker(XCSoarInterface::main_window,
                           _("Load Polar From File"), list, NULL);
  if (result >= 0) {
    const TCHAR* path = list[result].StringValue;
    PolarInfo polar;
    PolarGlue::LoadFromFile(polar, path);

    UpdatePolarPoints(polar.v1, polar.v2, polar.v3, polar.w1, polar.w2, polar.w3);

    LoadFormProperty(*wf, _T("prpPolarReferenceMass"), polar.reference_mass);
    LoadFormProperty(*wf, _T("prpPolarDryMass"), polar.reference_mass);
    LoadFormProperty(*wf, _T("prpPolarMaxBallast"), polar.max_ballast);

    if (positive(polar.wing_area))
      LoadFormProperty(*wf, _T("prpPolarWingArea"), polar.wing_area);

    if (positive(polar.v_no))
      LoadFormProperty(*wf, _T("prpMaxManoeuveringSpeed"), ugHorizontalSpeed,
                       polar.v_no);

    CommonInterface::SetSettingsComputer().plane.polar_name =
        list[result].StringValueFormatted;
    UpdatePolarTitle();
    UpdatePolarInvalidLabel();
  }
}

void
PolarConfigPanel::OnExport(WndButton &button)
{
  TCHAR filename[69] = _T("");
  if (!dlgTextEntryShowModal(*(SingleWindow *)button.get_root_owner(),
                             filename, 64))
    return;

  TCHAR path[MAX_PATH];
  _tcscat(filename, _T(".plr"));
  LocalPath(path, filename);

  PolarInfo polar;
  SaveFormToPolar(polar);
  if (PolarGlue::SaveToFile(polar, path)) {
    CommonInterface::SetSettingsComputer().plane.polar_name = filename;
    UpdatePolarTitle();
  }
}

void
PolarConfigPanel::OnFieldData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daChange:
    if (!loading)
      CommonInterface::SetSettingsComputer().plane.polar_name = _T("Custom");

    UpdatePolarTitle();
    UpdatePolarInvalidLabel();
    break;

  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    return;
  }
}

void
PolarConfigPanel::SetVisible(bool active)
{
  if (buttonList != NULL)
    buttonList->set_visible(active);

  if (buttonImport != NULL)
    buttonImport->set_visible(active);

  if (buttonExport != NULL)
    buttonExport->set_visible(active);
}

void
PolarConfigPanel::Init(WndForm *_wf)
{
  loading = true;

  assert(_wf != NULL);
  wf = _wf;
  buttonList = ((WndButton *)wf->FindByName(_T("cmdLoadInternalPolar")));
  buttonImport = ((WndButton *)wf->FindByName(_T("cmdLoadPolarFile")));
  buttonExport = ((WndButton *)wf->FindByName(_T("cmdSavePolarFile")));

  SetLiftFieldStepAndMax(_T("prpPolarW1"));
  SetLiftFieldStepAndMax(_T("prpPolarW2"));
  SetLiftFieldStepAndMax(_T("prpPolarW3"));

  PolarInfo polar;
  if (!PolarGlue::LoadFromProfile(polar)) {
    polar = PolarGlue::GetDefault();
    CommonInterface::SetSettingsComputer().plane.polar_name =
        PolarGlue::GetDefaultName();
  }
  UpdatePolarFields(polar);
  UpdatePolarTitle();
  UpdatePolarInvalidLabel();


  const SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SettingsComputer();

  LoadFormProperty(*wf, _T("prpHandicap"),
                   settings_computer.plane.handicap);

  LoadFormProperty(*wf, _T("prpBallastSecsToEmpty"),
                   settings_computer.plane.dump_time);

  loading = false;
}

bool
PolarConfigPanel::Save()
{
  bool changed = false;
  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();

  changed |= SaveFormProperty(*wf, _T("prpHandicap"),
                              settings_computer.plane.handicap);

  changed |= SaveFormProperty(*wf, _T("prpBallastSecsToEmpty"),
                              settings_computer.plane.dump_time);

  PolarInfo polar;
  PolarGlue::LoadFromProfile(polar);
  if (SaveFormToPolar(polar)) {
    PolarGlue::SaveToProfile(polar);
    changed = true;

    if (protected_task_manager != NULL) {
      polar.CopyIntoGlidePolar(settings_computer.glide_polar_task);
      settings_computer.SafetySpeed = fixed(polar.v_no);
      protected_task_manager->set_glide_polar(settings_computer.glide_polar_task);
    }
  }

  if (changed) {
    PlaneGlue::ToProfile(settings_computer.plane);
    PlaneGlue::Synchronize(settings_computer.plane, settings_computer);
  }

  return changed;
}
