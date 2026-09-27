// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Dialogs/Message.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "FLARM/Details.hpp"
#include "FLARM/Glue.hpp"
#include "Computer/Settings.hpp"
#include "Profile/Profile.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Language/Language.hpp"
#include "TeamActions.hpp"
#include "util/StaticString.hxx"
#include "util/StringCompare.hxx"
#include "util/StringStrip.hxx"
#include "util/TruncateString.hpp"
#include "util/Macros.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"

namespace {

/**
 * Team code as one group.  Reference, the mate's code and the FLARM
 * lock open another page.  Own code and the bearings are the position
 * in use, and they follow the calculation while the dialog is open.
 *
 * The dialog floats over the map, sized to this list, and is fitted
 * again when the list or the screen layout changes.
 */
class TeamSetup final : public NullBlackboardListener {
  using ValueState = GroupedListWidget::ValueState;

  GroupedListWidget *list = nullptr;
  WidgetDialog *dialog = nullptr;

  /** kept after the other aircraft drops out, as the old rows did */
  StaticString<64> last_range{"---"};
  StaticString<64> last_bearing{"---"};

public:
  void SetList(GroupedListWidget &_list) noexcept {
    list = &_list;
  }

  void SetDialog(WidgetDialog &_dialog) noexcept {
    dialog = &_dialog;
  }

  void Refresh() noexcept;
  void Start() noexcept;
  void Stop() noexcept;

private:
  void Build() noexcept;

  void EditReference();
  void EditMate();
  void EditFlarm();

  /* virtual methods from class BlackboardListener */
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;
};

void
TeamSetup::Build() noexcept
{
  list->AddGroup(nullptr);

  list->AddValue(_("Reference"),
                 _("The waypoint the codes are measured from."),
                 [this](ValueState &state) {
                   const int id = CommonInterface::GetComputerSettings()
                     .team_code.team_code_reference_waypoint;
                   const WaypointPtr wp =
                     id >= 0 && data_components != nullptr &&
                     data_components->waypoints != nullptr
                     ? data_components->waypoints->LookupId((unsigned)id)
                     : nullptr;
                   state.text = wp != nullptr ? wp->name.c_str() : "---";
                 },
                 [this]{ EditReference(); });

  list->AddValue(_("Own code"),
                 _("The code for this aircraft's position. "
                   "Read it to the other pilots."),
                 [](ValueState &state) {
                   const char *own = CommonInterface::Calculated()
                     .own_teammate_code.GetCode();
                   state.text = own[0] != '\0' ? own : "---";
                 });

  list->AddValue(_("Mate code"),
                 _("The code the other pilot has reported. "
                   "Entering a code clears a FLARM lock."),
                 [](ValueState &state) {
                   const char *mate = CommonInterface::GetComputerSettings()
                     .team_code.team_code.GetCode();
                   state.text = mate[0] != '\0' ? mate : "---";
                 },
                 [this]{ EditMate(); });

  list->AddValue(_("Range"),
                 _("Range to the team aircraft location at the last "
                   "reported team code."),
                 [this](ValueState &state) {
                   const TeamInfo &info = CommonInterface::Calculated();
                   if (info.teammate_available)
                     last_range = FormatUserDistanceSmart(
                       info.teammate_vector.distance).c_str();
                   state.text = last_range.c_str();
                 });

  list->AddValue(_("Bearing"),
                 _("Bearing to the team aircraft location at the last "
                   "team code report."),
                 [this](ValueState &state) {
                   const TeamInfo &info = CommonInterface::Calculated();
                   if (info.teammate_available)
                     last_bearing = FormatBearing(
                       info.teammate_vector.bearing).c_str();
                   state.text = last_bearing.c_str();
                 });

  list->AddValue(_("Rel. bearing"),
                 _("Relative bearing to the team aircraft location at "
                   "the last reported team code."),
                 [](ValueState &state) {
                   const DerivedInfo &calculated =
                     CommonInterface::Calculated();
                   const MoreData &basic = CommonInterface::Basic();
                   state.text =
                     calculated.teammate_available && basic.track_available
                     ? FormatAngleDelta(
                         calculated.teammate_vector.bearing -
                         basic.track).c_str()
                     : "---";
                 });

  list->AddValue(_("Flarm Lock"),
                 _("The competition number of a FLARM to follow. "
                   "An empty entry clears the lock."),
                 [](ValueState &state) {
                   const TeamCodeSettings &settings =
                     CommonInterface::GetComputerSettings().team_code;
                   state.text =
                     settings.team_flarm_id.IsDefined() &&
                     !settings.team_flarm_callsign.empty()
                     ? settings.team_flarm_callsign.c_str()
                     : "---";
                 },
                 [this]{ EditFlarm(); });
}

void
TeamSetup::Refresh() noexcept
{
  if (list->GetItemCount() == 0)
    Build();

  if (list->UpdateValues() && dialog != nullptr)
    dialog->RefitList();
}

void
TeamSetup::Start() noexcept
{
  CommonInterface::GetLiveBlackboard().AddListener(*this);
}

void
TeamSetup::Stop() noexcept
{
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
}

void
TeamSetup::OnCalculatedUpdate(const MoreData &,
                              const DerivedInfo &)
{
  Refresh();
}

void
TeamSetup::EditReference()
{
  const auto wp =
    ShowWaypointListDialog(*data_components->waypoints,
                           CommonInterface::Basic().location);
  if (wp == nullptr)
    return;

  TeamCodeSettings &settings =
    CommonInterface::SetComputerSettings().team_code;
  settings.team_code_reference_waypoint = wp->id;
  Profile::Set(ProfileKeys::TeamcodeRefWaypoint, wp->id);
  Profile::Save();
  Refresh();
}

void
TeamSetup::EditMate()
{
  char new_code[10];

  const char *code =
    CommonInterface::GetComputerSettings().team_code.team_code.GetCode();
  CopyTruncateString(new_code, ARRAY_SIZE(new_code), code);

  if (!TextEntryDialog(new_code, 7))
    return;

  StripRight(new_code);

  TeamCodeSettings &settings =
    CommonInterface::SetComputerSettings().team_code;
  settings.team_code.Update(new_code);
  if (settings.team_code.IsDefined())
    settings.team_flarm_id.Clear();

  Refresh();
}

void
TeamSetup::EditFlarm()
{
  TeamCodeSettings &settings =
    CommonInterface::SetComputerSettings().team_code;
  char callsign[decltype(settings.team_flarm_callsign)::capacity()];
  CopyTruncateString(callsign, ARRAY_SIZE(callsign),
                     settings.team_flarm_callsign.c_str());

  if (!TextEntryDialog(callsign, 4))
    return;

  if (StringIsEmpty(callsign)) {
    settings.team_flarm_id.Clear();
    settings.team_flarm_callsign.clear();
    Refresh();
    return;
  }

  LoadFlarmDatabases();

  FlarmId ids[30];
  unsigned count =
    FlarmDetails::FindIdsByCallSign(callsign, ids, 30);

  if (count == 0) {
    ShowMessageBox(_("Unknown Competition Number"),
                   _("Not found"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  const FlarmId id = PickFlarmTraffic(_("Set new teammate"), ids, count);
  if (!id.IsDefined())
    return;

  TeamActions::TrackFlarm(id, callsign);
  Refresh();
}

} // namespace

void
dlgTeamCodeShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  auto *list = new GroupedListWidget(look);
  TeamSetup setup;
  setup.SetList(*list);

  WidgetDialog dialog(WidgetDialog::Floating{}, UIGlobals::GetMainWindow(),
                      look, _("Team Code"), list);
  setup.SetDialog(dialog);
  dialog.AddButton(_("Close"), mrOK);

  dialog.PrepareFloatingList();
  setup.Refresh();
  setup.Start();
  dialog.ShowModal();
  setup.Stop();
}
