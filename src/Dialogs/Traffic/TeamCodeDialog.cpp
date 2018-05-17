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

#include "TrafficDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Dialogs/Message.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "FLARM/Glue.hpp"
#include "Computer/Settings.hpp"
#include "Profile/Profile.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Language/Language.hpp"
#include "TeamActions.hpp"
#include "Util/StringCompare.hxx"
#include "Util/TruncateString.hpp"
#include "Util/Macros.hpp"

class TeamCodeWidget final
  : public RowFormWidget, NullBlackboardListener, ActionListener {
  enum Controls {
    SET_CODE,
    SET_WAYPOINT,
    SET_FLARM_LOCK,
  };

  enum Buttons {
    OWN_CODE,
    MATE_CODE,
    RANGE,
    BEARING,
    RELATIVE_BEARING,
    FLARM_LOCK,
  };

public:
  TeamCodeWidget(const DialogLook &look)
    :RowFormWidget(look) {}

  void CreateButtons(WidgetDialog &buttons);
  void Update(const MoreData &basic, const DerivedInfo &calculated);

private:
  void OnCodeClicked();
  void OnSetWaypointClicked();
  void OnFlarmLockClicked();

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;
  virtual void Show(const PixelRect &rc) override;
  virtual void Hide() override;

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;

  /* virtual methods from class BlackboardListener */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) override;
};

inline void
TeamCodeWidget::CreateButtons(WidgetDialog &buttons)
{
  buttons.AddButton(_("Set code"), *this, SET_CODE);
  buttons.AddButton(_("Set WP"), *this, SET_WAYPOINT);
  buttons.AddButton(_("Flarm Lock"), *this, SET_FLARM_LOCK);
}

void
TeamCodeWidget::Prepare(ContainerWindow &parent,
                        const PixelRect &rc)
{
  AddReadOnly(_("Own code"));
  AddReadOnly(_("Mate code"));
  AddReadOnly(_("Range"));
  AddReadOnly(_("Bearing"));
  AddReadOnly(_("Rel. bearing"));
  AddReadOnly(_("Flarm lock"));
}

void
TeamCodeWidget::Show(const PixelRect &rc)
{
  Update(CommonInterface::Basic(), CommonInterface::Calculated());
  CommonInterface::GetLiveBlackboard().AddListener(*this);
  RowFormWidget::Show(rc);
}

void
TeamCodeWidget::Hide()
{
  RowFormWidget::Hide();
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
}

void
TeamCodeWidget::Update(const MoreData &basic, const DerivedInfo &calculated)
{
  const TeamInfo &teamcode_info = calculated;
  const TeamCodeSettings &settings =
    CommonInterface::GetComputerSettings().team_code;

  SetText(RELATIVE_BEARING,
          teamcode_info.teammate_available && basic.track_available
          ? FormatAngleDelta(teamcode_info.teammate_vector.bearing - basic.track).c_str()
          : _T("---"));

  if (teamcode_info.teammate_available) {
    SetText(BEARING,
            FormatBearing(teamcode_info.teammate_vector.bearing).c_str());

    SetText(RANGE,
            FormatUserDistanceSmart(teamcode_info.teammate_vector.distance));
  }

  SetText(OWN_CODE, teamcode_info.own_teammate_code.GetCode());
  SetText(MATE_CODE, settings.team_code.GetCode());
  SetText(FLARM_LOCK,
          settings.team_flarm_id.IsDefined()
          ? settings.team_flarm_callsign.c_str()
          : _T(""));
}

void
TeamCodeWidget::OnCalculatedUpdate(const MoreData &basic,
                                   const DerivedInfo &calculated)
{
  Update(basic, calculated);
}

inline void
TeamCodeWidget::OnSetWaypointClicked()
{
  const auto wp =
    ShowWaypointListDialog(CommonInterface::Basic().location);
  if (wp != nullptr) {
    CommonInterface::SetComputerSettings().team_code.team_code_reference_waypoint = wp->id;
    Profile::Set(ProfileKeys::TeamcodeRefWaypoint, wp->id);
  }
}

inline void
TeamCodeWidget::OnCodeClicked()
{
  TCHAR newTeammateCode[10];

  CopyTruncateString(newTeammateCode, ARRAY_SIZE(newTeammateCode),
                     CommonInterface::GetComputerSettings().team_code.team_code.GetCode());

  if (!TextEntryDialog(newTeammateCode, 7))
    return;

  StripRight(newTeammateCode);

  TeamCodeSettings &settings =
    CommonInterface::SetComputerSettings().team_code;
  settings.team_code.Update(newTeammateCode);
  if (settings.team_code.IsDefined())
    settings.team_flarm_id.Clear();
}

inline void
TeamCodeWidget::OnFlarmLockClicked()
{
  TeamCodeSettings &settings =
    CommonInterface::SetComputerSettings().team_code;
  TCHAR newTeamFlarmCNTarget[settings.team_flarm_callsign.capacity()];
  _tcscpy(newTeamFlarmCNTarget, settings.team_flarm_callsign.c_str());

  if (!TextEntryDialog(newTeamFlarmCNTarget, 4))
    return;

  if (StringIsEmpty(newTeamFlarmCNTarget)) {
    settings.team_flarm_id.Clear();
    settings.team_flarm_callsign.clear();
    return;
  }

  LoadFlarmDatabases();

  FlarmId ids[30];
  unsigned count =
    FlarmDetails::FindIdsByCallSign(newTeamFlarmCNTarget, ids, 30);

  if (count == 0) {
    ShowMessageBox(_("Unknown Competition Number"),
                   _("Not Found"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  const FlarmId id = PickFlarmTraffic(_("Set new teammate"), ids, count);
  if (!id.IsDefined())
    return;

  TeamActions::TrackFlarm(id, newTeamFlarmCNTarget);
}

void
TeamCodeWidget::OnAction(int id)
{
  switch (id) {
  case SET_CODE:
    OnCodeClicked();
    break;

  case SET_WAYPOINT:
    OnSetWaypointClicked();
    break;

  case SET_FLARM_LOCK:
    OnFlarmLockClicked();
    break;
  }
}

void
dlgTeamCodeShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  TeamCodeWidget widget(look);
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _("Team Code"), &widget);
  widget.CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();
}
