// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
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
#include "Components.hpp"
#include "BackendComponents.hpp"

#include <cassert>

class AlternatesListWidget final
  : public ListWidget {
  enum Buttons {
    SETTINGS,
    GOTO,
  };

  const DialogLook &dialog_look;

  TwoTextRowsRenderer row_renderer;

  Button *details_button, *cancel_button, *goto_button;

public:
  AlternateList alternates;

public:
  void CreateButtons(WidgetDialog &dialog);

public:
  AlternatesListWidget(const DialogLook &_dialog_look)
    :dialog_look(_dialog_look) {}

  unsigned GetCursorIndex() const {
    return GetList().GetCursorIndex();
  }

  bool Update() {
    ProtectedTaskManager::Lease lease(*backend_components->protected_task_manager);
    alternates = lease->GetAlternates();
    return !alternates.empty();
  }

private:
  [[gnu::pure]]
  const auto &GetSelectedWaypointPtr() const noexcept {
    unsigned index = GetCursorIndex();
    assert(index < alternates.size());

    auto const &item = alternates[index];
    return item.waypoint;
  }

  [[gnu::pure]]
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

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override;
};

void
AlternatesListWidget::CreateButtons(WidgetDialog &dialog)
{
  goto_button = dialog.AddButton(_("Goto"), [this](){
    backend_components->protected_task_manager->DoGoto(GetSelectedWaypointPtr());
    cancel_button->Click();
  });

  details_button = dialog.AddButton(_("Details"), mrOK);

  dialog.AddButton(_("Set Active Frequency"), [this](){
    auto const &waypoint = GetSelectedWaypoint();
    ActionInterface::SetActiveFrequency(waypoint.radio_frequency,
                                        waypoint.name.c_str());
  });

  dialog.AddButton(_("Set Standby Frequency"), [this](){
    auto const &waypoint = GetSelectedWaypoint();
    ActionInterface::SetStandbyFrequency(waypoint.radio_frequency,
                                         waypoint.name.c_str());
  });

  cancel_button = dialog.AddButton(_("Close"), mrCancel);
}

void
AlternatesListWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                              [[maybe_unused]] const PixelRect &rc) noexcept
{
  CreateList(parent, dialog_look, rc,
             row_renderer.CalculateLayout(*dialog_look.list.font_bold,
                                          dialog_look.small_font));

  GetList().SetLength(alternates.size());
}

void
AlternatesListWidget::OnActivateItem([[maybe_unused]] unsigned index) noexcept
{
  details_button->Click();
}

void
dlgAlternatesListShowModal(Waypoints *waypoints) noexcept
{
  if (!backend_components->protected_task_manager)
    return;

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<AlternatesListWidget>(dialog_look);
  if (!widget->Update())
    /* no alternates: don't show the dialog */
    return;

  TWidgetDialog<AlternatesListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           dialog_look, _("Alternates"));
  widget->CreateButtons(dialog);
  dialog.FinishPreliminary(std::move(widget));
  dialog.EnableCursorSelection();

  int i = dialog.ShowModal() == mrOK
    ? (int)dialog.GetWidget().GetCursorIndex()
    : -1;

  if (i < 0 || (unsigned)i >= dialog.GetWidget().alternates.size())
    return;

  dlgWaypointDetailsShowModal(waypoints,
                              dialog.GetWidget().alternates[i].waypoint, true);
}
