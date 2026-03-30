// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Form/Form.hpp"
#include "InfoBoxes/Content/Alternate.hpp"
#include "Widget/ListWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Unordered/AlternateList.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Renderer/WaypointListRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Language/Language.hpp"
#include "ActionInterface.hpp"
#include "Message.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Protection.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#include <cassert>
#include <optional>

static constexpr int DETAILS_MODAL_RESULT = 100;

class AlternatesListWidget final
  : public ListWidget {
  const DialogLook &dialog_look;
  const bool select_mode;
  const std::optional<AlternateInfoBoxSlot> slot;

  TwoTextRowsRenderer row_renderer;

  Button *details_button = nullptr;
  Button *cancel_button = nullptr;
  Button *goto_button = nullptr;
  Button *select_button = nullptr;
  Button *mode_button = nullptr;
  Button *manual_button = nullptr;
  Button *set_active_freq_button = nullptr;
  Button *set_standby_freq_button = nullptr;

public:
  AlternateList alternates;

public:
  void CreateButtons(WidgetDialog &dialog);

public:
  explicit
  AlternatesListWidget(const DialogLook &_dialog_look,
                       bool _select_mode = false,
                       std::optional<AlternateInfoBoxSlot> _slot =
                         std::nullopt) noexcept
    :dialog_look(_dialog_look), select_mode(_select_mode), slot(_slot) {}

  unsigned GetCursorIndex() const {
    return GetList().GetCursorIndex();
  }

  bool Update() {
    ProtectedTaskManager::Lease lease(*backend_components->protected_task_manager);
    alternates = lease->GetAlternates();
    return !alternates.empty();
  }

private:
  [[nodiscard]] [[gnu::pure]]
  bool
  HasValidSelection() const noexcept {
    return !alternates.empty() && GetCursorIndex() < alternates.size();
  }

  /**
   * Returns the configured alternate slot for the slot-aware dialog
   * path.  This must only be used when slot-specific controls have
   * been created.
   */
  [[nodiscard]] [[gnu::pure]]
  AlternateInfoBoxSlot
  GetConfiguredSlot() const noexcept {
    assert(slot.has_value());
    return *slot;
  }

  [[nodiscard]] [[gnu::pure]]
  const auto &GetSelectedWaypointPtr() const noexcept {
    const unsigned index = GetCursorIndex();
    assert(index < alternates.size());

    auto const &item = alternates[index];
    return item.waypoint;
  }

  [[nodiscard]] [[gnu::pure]]
  const auto &GetSelectedWaypoint() const noexcept {
    return *GetSelectedWaypointPtr();
  }

public:
  /* virtual methods from class Widget */
  void Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept override;

  /* virtual methods from class List::Handler */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned index) noexcept override {
    assert(index < alternates.size());

    const ComputerSettings &settings = CommonInterface::GetComputerSettings();
    const Waypoint &waypoint = *alternates[index].waypoint;
    const GlideResult& solution = alternates[index].solution;

    WaypointListRenderer::Draw(canvas, rc, waypoint, solution.vector.distance,
                               solution.SelectAltitudeDifference(settings.task.glide),
                               row_renderer,
                               UIGlobals::GetMapLook().waypoint,
                               CommonInterface::GetMapSettings().waypoint);
  }

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    UpdateButtons();
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override;

private:
  void UpdateButtons() noexcept {

    // Check if window is initialized (widget is prepared)
    if (!IsDefined())
      return;

    if (set_active_freq_button == nullptr || set_standby_freq_button == nullptr)
      return;

    if (mode_button != nullptr) {
      mode_button->SetCaption(GetAlternateInfoBoxMode(GetConfiguredSlot()) ==
                                  AlternateInfoBoxMode::MANUAL
                                ? _("Altn mode: MANUAL")
                                : _("Altn mode: AUTO"));
    }

    if (!HasValidSelection()) {
      if (goto_button != nullptr)
        goto_button->SetEnabled(false);
      if (details_button != nullptr)
        details_button->SetEnabled(false);
      set_active_freq_button->SetEnabled(false);
      set_standby_freq_button->SetEnabled(false);
      if (manual_button != nullptr)
        manual_button->SetEnabled(false);
      return;
    }

    const auto &waypoint = GetSelectedWaypoint();
    const bool has_freq = waypoint.radio_frequency.IsDefined();
    if (goto_button != nullptr)
      goto_button->SetEnabled(true);
    if (details_button != nullptr)
      details_button->SetEnabled(true);
    set_active_freq_button->SetEnabled(has_freq);
    set_standby_freq_button->SetEnabled(has_freq);
    if (manual_button != nullptr) {
      manual_button->SetEnabled(GetAlternateInfoBoxMode(GetConfiguredSlot()) ==
                                  AlternateInfoBoxMode::MANUAL);
    }
  }
};

void
AlternatesListWidget::CreateButtons(WidgetDialog &dialog)
{
  if (!select_mode) {
    goto_button = dialog.AddButton(_("Goto"), [this](){
      if (!HasValidSelection())
        return;

      // Remove old temporary goto waypoint when selecting a regular waypoint
      if (data_components != nullptr && data_components->waypoints != nullptr) {
        auto &way_points = *data_components->waypoints;
        {
          ScopeSuspendAllThreads suspend;
          way_points.EraseTempGoto();
        }
      }

      backend_components->protected_task_manager->DoGoto(GetSelectedWaypointPtr());
      cancel_button->Click();
    });
  } else {
    select_button = dialog.AddButton(_("Select"), mrOK);
  }

  if (!select_mode && slot.has_value()) {
    mode_button = dialog.AddButton("", [this](){
      const auto slot = GetConfiguredSlot();
      const auto new_mode = GetAlternateInfoBoxMode(slot) ==
        AlternateInfoBoxMode::MANUAL
        ? AlternateInfoBoxMode::AUTO
        : AlternateInfoBoxMode::MANUAL;
      SetAlternateInfoBoxMode(slot, new_mode);
      UpdateButtons();
    });

    manual_button = dialog.AddButton(_("Select as Alternate"), [this](){
      if (!HasValidSelection())
        return;

      const auto slot = GetConfiguredSlot();
      SetManualAlternateWaypoint(slot, GetSelectedWaypointPtr());
      SetAlternateInfoBoxMode(slot, AlternateInfoBoxMode::MANUAL);
      cancel_button->Click();
    });
  }

  if (!select_mode)
    details_button = dialog.AddButton(_("Details"), DETAILS_MODAL_RESULT);

  set_active_freq_button = dialog.AddButton(_("Set Active Frequency"), [this](){
    if (!HasValidSelection())
      return;

    auto const &waypoint = GetSelectedWaypoint();
    ActionInterface::SetActiveFrequency(waypoint.radio_frequency,
                                        waypoint.name.c_str());
  });

  set_standby_freq_button = dialog.AddButton(_("Set Standby Frequency"), [this](){
    if (!HasValidSelection())
      return;

    auto const &waypoint = GetSelectedWaypoint();
    ActionInterface::SetStandbyFrequency(waypoint.radio_frequency,
                                         waypoint.name.c_str());
  });

  cancel_button = dialog.AddButton(_("Close"), mrCancel);
  
  // Update button states now that buttons are created
  UpdateButtons();
}

void
AlternatesListWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                              [[maybe_unused]] const PixelRect &rc) noexcept
{
  CreateList(parent, dialog_look, rc,
             row_renderer.CalculateLayout(*dialog_look.list.font_bold,
                                          dialog_look.small_font));

  GetList().SetLength(alternates.size());
  UpdateButtons();
}

void
AlternatesListWidget::OnActivateItem([[maybe_unused]] unsigned index) noexcept
{
  if (select_mode && select_button != nullptr)
    select_button->Click();
  else if (details_button != nullptr)
    details_button->Click();
}

void
dlgAlternatesListShowModal(Waypoints *waypoints,
                           std::optional<AlternateInfoBoxSlot> slot) noexcept
{
  if (!backend_components->protected_task_manager)
    return;

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<AlternatesListWidget>(dialog_look, false,
                                                       slot);
  const bool has_alternates = widget->Update();
  if (!has_alternates && !slot.has_value()) {
    /* no alternates: don't show the dialog */
    Message::AddMessage(_("No alternates available"));
    return;
  }

  const auto *title = _("Alternates");
  if (slot.has_value()) {
    title = _("Alternates 1");
    if (*slot == AlternateInfoBoxSlot::SECOND)
      title = _("Alternates 2");
  }

  TWidgetDialog<AlternatesListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(), dialog_look,
           title);
  widget->CreateButtons(dialog);
  dialog.FinishPreliminary(std::move(widget));
  dialog.EnableCursorSelection();

  const int result = dialog.ShowModal();
  if (result != DETAILS_MODAL_RESULT)
    return;

  int i = (int)dialog.GetWidget().GetCursorIndex();
  if (i < 0 || (unsigned)i >= dialog.GetWidget().alternates.size())
    return;

  dlgWaypointDetailsShowModal(waypoints,
                              dialog.GetWidget().alternates[i].waypoint, true);
}

WaypointPtr
dlgAlternatesListSelectWaypoint() noexcept
{
  if (!backend_components->protected_task_manager)
    return nullptr;

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<AlternatesListWidget>(dialog_look, true);
  if (!widget->Update()) {
    Message::AddMessage(_("No alternates available"));
    return nullptr;
  }

  TWidgetDialog<AlternatesListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           dialog_look, _("Alternates"));
  widget->CreateButtons(dialog);
  dialog.FinishPreliminary(std::move(widget));
  dialog.EnableCursorSelection();

  const int result = dialog.ShowModal();
  if (result != mrOK)
    return nullptr;

  const auto &dialog_widget = dialog.GetWidget();
  const unsigned i = dialog_widget.GetCursorIndex();
  if (i >= dialog_widget.alternates.size())
    return nullptr;

  return dialog_widget.alternates[i].waypoint;
}
