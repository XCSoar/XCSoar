// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ReplayDialog.hpp"
#include "Dialogs/DataManagement/ExportFlightsPanel.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/PickList.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Replay/Replay.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"

#include <chrono>

namespace {

/**
 * Replay as one group.  Flight opens the log list.  An empty choice
 * is the demo.  Rate is written as soon as a value is chosen.
 * Start, Stop and fast forward stay dialog buttons.
 *
 * The dialog floats over the map, sized to this list, and is fitted
 * again when the list or the screen layout changes.
 */
class ReplaySetup final {
  using ValueState = GroupedListWidget::ValueState;
  GroupedListWidget *list = nullptr;
  WidgetDialog *dialog = nullptr;
  Replay &replay;

  /** nullptr until a log is chosen; empty runs the demo */
  AllocatedPath path;

public:
  explicit ReplaySetup(Replay &_replay) noexcept
    :replay(_replay), path(replay.GetFilename()) {}

  void SetList(GroupedListWidget &_list) noexcept {
    list = &_list;
  }

  void SetDialog(WidgetDialog &_dialog) noexcept {
    dialog = &_dialog;
  }

  void Refresh() noexcept;
  void Start() noexcept;
  void Stop() noexcept;
  void FastForward() noexcept;

private:
  void Build() noexcept;
  void EditFlight();
  void EditRate();

  [[nodiscard]]
  const char *FlightValue() const noexcept;

};

static void
FormatReplayRate(double value, StaticString<64> &text)
{
  text.Format("%.0f x", value);
}

const char *
ReplaySetup::FlightValue() const noexcept
{
  if (path == nullptr || path.empty())
    return _("Demo");

  return path.GetBase().c_str();
}

void
ReplaySetup::Build() noexcept
{
  list->AddGroup(nullptr);

  list->AddValue(_("Flight"),
                 _("Name of file to replay. May be an IGC file (.igc) "
                   "or a raw NMEA log file (.nmea). "
                   "Leave blank to run the demo."),
                 [this](ValueState &state) {
                   state.text = FlightValue();
                 },
                 [this]{ EditFlight(); });

  list->AddValue(_("Rate"),
                 _("Time acceleration of replay. Set to 0 for pause, "
                   "1 for normal real-time replay."),
                 [this](ValueState &state) {
                   StaticString<64> text;
                   FormatReplayRate(replay.GetTimeScale(), text);
                   state.text = text.c_str();
                 },
                 [this]{ EditRate(); });
}

void
ReplaySetup::Refresh() noexcept
{
  if (list->GetItemCount() == 0)
    Build();

  if (list->UpdateValues() && dialog != nullptr)
    dialog->RefitList();
}

void
ReplaySetup::Start() noexcept
{
  const Path start_path = path == nullptr || path.empty()
    ? Path{""}
    : Path{path};

  try {
    replay.Start(start_path,
                 CommonInterface::GetSystemSettings().devices[0]);
  } catch (...) {
    ShowError(std::current_exception(), _("Replay"));
  }
}

void
ReplaySetup::Stop() noexcept
{
  replay.Stop();
}

void
ReplaySetup::FastForward() noexcept
{
  replay.FastForward(std::chrono::minutes{10});
}

void
ReplaySetup::EditFlight()
{
  AllocatedPath chosen = path == nullptr
    ? nullptr
    : AllocatedPath(Path(path));
  switch (PickReplayFlight(_("Flight"), chosen)) {
  case ReplayFlightChoice::CANCEL:
    return;

  case ReplayFlightChoice::DEMO:
    path = nullptr;
    break;

  case ReplayFlightChoice::FILE:
    path = std::move(chosen);
    break;
  }

  Refresh();
}

void
ReplaySetup::EditRate()
{
  double value = replay.GetTimeScale();
  if (!PickList(_("Rate"), value, 0, 10, 1, FormatReplayRate))
    return;

  replay.SetTimeScale(value);
  Refresh();
}

} // namespace

void
ShowReplayDialog(Replay &replay) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  auto *list = new GroupedListWidget(look);
  ReplaySetup setup(replay);
  setup.SetList(*list);

  WidgetDialog dialog(WidgetDialog::Floating{}, UIGlobals::GetMainWindow(),
                      look, _("Replay"), list);
  setup.SetDialog(dialog);
  dialog.AddButton(_("Start"), [&setup]{ setup.Start(); });
  dialog.AddButton(_("Stop"), [&setup]{ setup.Stop(); });
  dialog.AddButton(_("+10'"), [&setup]{ setup.FastForward(); });
  dialog.AddButton(_("Close"), mrOK);

  dialog.PrepareFloatingList();
  setup.Refresh();
  dialog.ShowModal();
}
