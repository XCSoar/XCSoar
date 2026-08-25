// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/ListPicker.hpp"
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
#include "Renderer/TextRowListItemRenderer.hpp"
#include "util/StaticString.hxx"
#include "Language/Language.hpp"
#include "ActionInterface.hpp"
#include "Message.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Protection.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <optional>

namespace {
class AlternatesListWidget final
  : public ListWidget {
  const DialogLook &dialog_look;
  const bool select_mode;

  TwoTextRowsRenderer row_renderer;

  /**
   * The waypoints the alternate InfoBox slots currently refer to,
   * indexed by slot; nullptr for a slot without a target.
   */
  std::array<WaypointPtr, alternate_info_box_slot_count> slot_waypoints;

  Button *details_button = nullptr;
  Button *cancel_button = nullptr;
  Button *goto_button = nullptr;
  Button *select_button = nullptr;
  Button *auto_button = nullptr;
  Button *manual_button = nullptr;
  Button *set_active_freq_button = nullptr;
  Button *set_standby_freq_button = nullptr;

public:
  AlternateList alternates;

public:
  void CreateButtons(WidgetDialog &dialog, Waypoints *waypoints_for_details = nullptr) noexcept;

public:
  explicit
  AlternatesListWidget(const DialogLook &_dialog_look,
                       bool _select_mode = false) noexcept
    :dialog_look(_dialog_look), select_mode(_select_mode) {}

  unsigned GetCursorIndex() const {
    return GetList().GetCursorIndex();
  }

  bool Update() {
    {
      ProtectedTaskManager::Lease lease(*backend_components->protected_task_manager);
      alternates = lease->GetAlternates();
    }

    /* a manually pinned alternate is not necessarily among the
       computed ones; append it so the pilot sees what the InfoBox
       refers to */
    for (const auto slot : all_alternate_info_box_slots) {
      auto waypoint = GetAlternateSlotWaypoint(slot);
      slot_waypoints[ToAlternateInfoBoxSlotIndex(slot)] = waypoint;

      if (waypoint != nullptr && !Contains(*waypoint))
        alternates.emplace_back(waypoint, SolveAlternateGlide(*waypoint));
    }

    return !alternates.empty();
  }

private:
  [[nodiscard]] [[gnu::pure]]
  bool
  HasValidSelection() const noexcept {
    return !alternates.empty() && GetCursorIndex() < alternates.size();
  }

  [[nodiscard]] [[gnu::pure]]
  bool
  Contains(const Waypoint &waypoint) const noexcept {
    return std::any_of(alternates.begin(), alternates.end(),
                       [&waypoint](const AlternatePoint &alternate){
                         return *alternate.waypoint == waypoint;
                       });
  }

  [[nodiscard]] [[gnu::pure]]
  static bool
  HasManualAlternate() noexcept {
    return std::any_of(all_alternate_info_box_slots.begin(),
                       all_alternate_info_box_slots.end(),
                       [](AlternateInfoBoxSlot slot){
                         return GetAlternateInfoBoxMode(slot) ==
                           AlternateInfoBoxMode::MANUAL;
                       });
  }

  /**
   * Formats the "ALT1"/"ALT2" markers of the alternate slots
   * referring to the specified waypoint.
   *
   * @return false if no slot refers to it
   */
  [[nodiscard]]
  bool
  FormatSlotMarkers(const Waypoint &waypoint,
                    StaticString<16> &buffer) const noexcept {
    buffer.clear();

    for (const auto slot : all_alternate_info_box_slots) {
      const auto &slot_waypoint =
        slot_waypoints[ToAlternateInfoBoxSlotIndex(slot)];
      if (slot_waypoint == nullptr || !(*slot_waypoint == waypoint))
        continue;

      if (!buffer.empty())
        buffer.push_back(' ');

      buffer.AppendFormat("ALT%u", GetAlternateInfoBoxSlotDisplayNumber(slot));
    }

    return !buffer.empty();
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

    PixelRect row_rc = rc;
    StaticString<16> markers;
    if (FormatSlotMarkers(waypoint, markers))
      row_rc.right = row_renderer.DrawRightFirstRow(canvas, rc, markers.c_str());

    if (!solution.IsDefined()) {
      /* a manually pinned alternate while the glide computer has no
         solution (yet); the glide figures would be undefined */
      WaypointListRenderer::Draw(canvas, row_rc, waypoint, row_renderer,
                                 UIGlobals::GetMapLook().waypoint,
                                 CommonInterface::GetMapSettings().waypoint);
      return;
    }

    WaypointListRenderer::Draw(canvas, row_rc, waypoint, solution.vector.distance,
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
  /**
   * Rebuilds the list after the alternate slots have been modified.
   */
  void RefreshList() noexcept {
    Update();

    auto &list = GetList();
    list.SetLength(alternates.size());
    list.Invalidate();

    UpdateButtons();
  }

  void UpdateButtons() const noexcept {

    // Check if window is initialized (widget is prepared)
    if (!IsDefined())
      return;

    if (set_active_freq_button == nullptr || set_standby_freq_button == nullptr)
      return;

    if (auto_button != nullptr)
      auto_button->SetEnabled(HasManualAlternate());

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
    if (manual_button != nullptr)
      manual_button->SetEnabled(true);
  }
};
}

void
AlternatesListWidget::CreateButtons(WidgetDialog &dialog,
                                    Waypoints *waypoints_for_details) noexcept
{
  if (!select_mode) {
    goto_button = dialog.AddButton(_("GoTo"), [this](){
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

  if (!select_mode) {
    auto_button = dialog.AddButton(C_("Button", "Alternate AUTO"), [this](){
      const auto slot =
        dlgAlternateSlotShowModal(C_("Button", "Alternate AUTO"));
      if (!slot.has_value() ||
          GetAlternateInfoBoxMode(*slot) != AlternateInfoBoxMode::MANUAL)
        return;

      SetAlternateInfoBoxMode(*slot, AlternateInfoBoxMode::AUTO);
      RefreshList();
    });

    manual_button = dialog.AddButton(C_("Button", "Select as Alternate"), [this](){
      if (!HasValidSelection())
        return;

      const auto slot =
        dlgAlternateSlotShowModal(C_("Button", "Select as Alternate"));
      if (!slot.has_value())
        return;

      SelectManualAlternateWaypoint(*slot, GetSelectedWaypointPtr());
      cancel_button->Click();
    });
  }

  if (!select_mode) {
    details_button = dialog.AddButton(
      _("Details"),
      [this, &dialog, waypoints_for_details]() noexcept {
        if (!HasValidSelection())
          return;

        Waypoints *wpts = waypoints_for_details;
        if (wpts == nullptr && data_components != nullptr)
          wpts = data_components->waypoints.get();
        if (wpts == nullptr)
          return;

        WaypointPtr w(GetSelectedWaypointPtr());
        if (w == nullptr)
          return;

        /* allow_navigation + allow_edit: like the persistent list;
         * alternates are task-adjacent; editing user waypoints
         * (e.g. user.cup) stays available from this entry point. */
        if (dlgWaypointDetailsShowModalForBrowseParent(
              wpts, std::move(w), true, true))
          dialog.SetModalResult(mrOK);
      });
  }

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

namespace {

/**
 * Renders one row of the alternate slot picker, showing the slot
 * name, its current target and its mode.
 */
class AlternateSlotListItemRenderer final : public TextRowListItemRenderer {
  std::array<StaticString<128>, alternate_info_box_slot_count> rows;

public:
  AlternateSlotListItemRenderer() noexcept {
    for (const auto slot : all_alternate_info_box_slots) {
      const auto waypoint = GetAlternateSlotWaypoint(slot);
      const auto alternate_mode_short_label =
        GetAlternateModeShortLabel(GetAlternateInfoBoxMode(slot));

      rows[ToAlternateInfoBoxSlotIndex(slot)]
        .Format("%s - %s (%s)", GetAlternateSlotName(slot),
                waypoint != nullptr ? waypoint->name.c_str() : _("None"),
                alternate_mode_short_label);
    }
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   const unsigned index) noexcept override {
    assert(index < rows.size());

    row_renderer.DrawTextRow(canvas, rc, rows[index].c_str());
  }
};

} // namespace

std::optional<AlternateInfoBoxSlot>
dlgAlternateSlotShowModal(const char *caption) noexcept
{
  AlternateSlotListItemRenderer renderer;

  const int i = ListPicker(caption, alternate_info_box_slot_count, 0,
                           renderer.CalculateLayout(UIGlobals::GetDialogLook()),
                           renderer);
  if (i < 0)
    return std::nullopt;

  return all_alternate_info_box_slots[i];
}

void
dlgAlternatesListShowModal(Waypoints *waypoints) noexcept
{
  if (!backend_components->protected_task_manager)
    return;

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<AlternatesListWidget>(dialog_look);
  if (!widget->Update()) {
    /* no alternates: don't show the dialog */
    Message::AddMessage(_("No alternates available"));
    return;
  }

  TWidgetDialog<AlternatesListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(), dialog_look,
           _("Alternates"));
  widget->CreateButtons(dialog, waypoints);
  dialog.FinishPreliminary(std::move(widget));
  dialog.EnableCursorSelection();

  dialog.ShowModal();
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
