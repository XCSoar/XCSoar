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

#include "RouteConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;

static void
ShowRouteControls(bool show)
{
  ShowFormControl(*wf, _T("prpRoutePlannerAllowClimb"), show);
  ShowFormControl(*wf, _T("prpRoutePlannerUseCeiling"), show);
}

static void
ShowReachControls(bool show)
{
  ShowFormControl(*wf, _T("prpFinalGlideTerrain"), show);
  ShowFormControl(*wf, _T("prpReachPolarMode"), show);
}

void
RouteConfigPanel::OnRouteMode(DataField *Sender,
                              DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  RoutePlannerConfig::Mode mode = (RoutePlannerConfig::Mode)df.GetAsInteger();
  ShowRouteControls(mode != RoutePlannerConfig::rpNone);
}

void
RouteConfigPanel::OnReachMode(DataField *Sender,
                              DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  RoutePlannerConfig::ReachMode mode =
    (RoutePlannerConfig::ReachMode)df.GetAsInteger();
  ShowReachControls(mode != RoutePlannerConfig::rmOff);
}

void
RouteConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;
  const SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SettingsComputer();

  wp = (WndProperty*)wf->FindByName(_T("prpFinalGlideTerrain"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Off"), SETTINGS_COMPUTER::FGT_OFF);
    dfe->addEnumText(_("Line"), SETTINGS_COMPUTER::FGT_LINE);
    dfe->addEnumText(_("Shade"), SETTINGS_COMPUTER::FGT_SHADE);
    dfe->Set(settings_computer.FinalGlideTerrain);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRoutePlannerMode"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("None"));
    dfe->addEnumText(_("Terrain"));
    dfe->addEnumText(_("Airspace"));
    dfe->addEnumText(_("Both"));
    dfe->Set(settings_computer.route_planner.mode);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpRoutePlannerAllowClimb"),
                   settings_computer.route_planner.allow_climb);

  LoadFormProperty(*wf, _T("prpRoutePlannerUseCeiling"),
                   settings_computer.route_planner.use_ceiling);

  wp = (WndProperty*)wf->FindByName(_T("prpTurningReach"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Off"));
    dfe->addEnumText(_("Straight"));
    dfe->addEnumText(_("Turning"));
    dfe->Set(settings_computer.route_planner.reach_calc_mode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpReachPolarMode"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Task"));
    dfe->addEnumText(_("Safety MC"));
    dfe->Set(settings_computer.route_planner.reach_polar_mode);
    wp->RefreshDisplay();
  }

  ShowRouteControls(settings_computer.route_planner.mode != RoutePlannerConfig::rpNone);
  ShowReachControls(settings_computer.route_planner.reach_calc_mode != RoutePlannerConfig::rmOff);
}


bool
RouteConfigPanel::Save()
{
  bool changed = false;
  WndProperty *wp;
  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();

  wp = (WndProperty*)wf->FindByName(_T("prpRoutePlannerMode"));
  if (wp) {
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
    if ((int)settings_computer.route_planner.mode != df.GetAsInteger()) {
      settings_computer.route_planner.mode = (RoutePlannerConfig::Mode)df.GetAsInteger();
      Profile::Set(szProfileRoutePlannerMode,
                   (unsigned)settings_computer.route_planner.mode);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpReachPolarMode"));
  if (wp) {
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
    if ((int)settings_computer.route_planner.reach_polar_mode != df.GetAsInteger()) {
      settings_computer.route_planner.reach_polar_mode =
        (RoutePlannerConfig::PolarMode)df.GetAsInteger();
      Profile::Set(szProfileReachPolarMode,
                   (unsigned)settings_computer.route_planner.reach_polar_mode);
      changed = true;
    }
  }

  changed |= SaveFormPropertyEnum(*wf, _T("prpFinalGlideTerrain"),
                                  szProfileFinalGlideTerrain,
                                  settings_computer.FinalGlideTerrain);

  changed |= SaveFormProperty(*wf, _T("prpRoutePlannerAllowClimb"),
                              szProfileRoutePlannerAllowClimb,
                              settings_computer.route_planner.allow_climb);

  changed |= SaveFormProperty(*wf, _T("prpRoutePlannerUseCeiling"),
                              szProfileRoutePlannerUseCeiling,
                              settings_computer.route_planner.use_ceiling);

  wp = (WndProperty*)wf->FindByName(_T("prpTurningReach"));
  if (wp) {
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
    if ((int)settings_computer.route_planner.reach_calc_mode != df.GetAsInteger()) {
      settings_computer.route_planner.reach_calc_mode =
        (RoutePlannerConfig::ReachMode)df.GetAsInteger();
      Profile::Set(szProfileTurningReach,
                   (unsigned)settings_computer.route_planner.reach_calc_mode);
      changed = true;
    }
  }

  return changed;
}
