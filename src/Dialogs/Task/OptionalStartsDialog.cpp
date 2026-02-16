// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Form/Button.hpp"
#include "Widget/ListWidget.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"

#include <cassert>

class OptionStartsWidget : public ListWidget {
  Waypoints &waypoints;
  OrderedTask &task;
  bool modified = false;

  Button *relocate_button, *remove_button;

  TextRowRenderer row_renderer;

public:
  OptionStartsWidget(Waypoints &_waypoints, OrderedTask &_task) noexcept
    :waypoints(_waypoints), task(_task) {}

  void CreateButtons(WidgetDialog &dialog) {
    relocate_button = dialog.AddButton(_("Relocate"), [this](){
      Relocate(GetList().GetCursorIndex());
    });

    remove_button = dialog.AddButton(_("Remove"), [this](){
      Remove(GetList().GetCursorIndex());
    });

    dialog.AddButton(_("Close"), mrCancel);
  }

  bool IsModified() const {
    return modified;
  }

public:
  unsigned GetCursorIndex() const {
    return GetList().GetCursorIndex();
  }

protected:
  void UpdateList() {
    GetList().SetLength(task.GetOptionalStartPointCount() + 2);
    GetList().Invalidate();
  }

  void UpdateButtons() {
    const unsigned current = GetCursorIndex();
    relocate_button->SetEnabled(current >= 1);
    remove_button->SetEnabled(current >= 1 &&
                              current < GetList().GetLength() - 1);
  }

  void RefreshView() {
    UpdateList();
    UpdateButtons();
  }

  void Relocate(unsigned i);
  void Remove(unsigned i);

public:
  /* virtual methods from class Widget */
  void Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept override;

  /* virtual methods from class List::Handler */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    UpdateButtons();
  }

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return index > 0;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override {
    Relocate(index);
  }
};

void
OptionStartsWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                            [[maybe_unused]] const PixelRect &rc) noexcept
{
  CreateList(parent, UIGlobals::GetDialogLook(),
             rc, row_renderer.CalculateLayout(*UIGlobals::GetDialogLook().list.font));

  RefreshView();
}

void
OptionStartsWidget::OnPaintItem(Canvas &canvas, PixelRect rc,
                                unsigned DrawListIndex) noexcept
{
  assert(DrawListIndex < task.GetOptionalStartPointCount() + 2);
  assert(GetList().GetLength() == task.GetOptionalStartPointCount() + 2);

  const unsigned index_optional_starts = DrawListIndex - 1;

  if (DrawListIndex == GetList().GetLength() - 1) {
    row_renderer.DrawTextRow(canvas, rc,
                             _("(Add Alternate Start)"));
  } else {
    const OrderedTaskPoint *tp;
    if (DrawListIndex == 0) {
      tp = &task.GetPoint(0);
      rc.left = row_renderer.DrawColumn(canvas, rc, "*");
    } else
      tp = &task.GetOptionalStartPoint(index_optional_starts);

    assert(tp != nullptr);

    row_renderer.DrawTextRow(canvas, rc, tp->GetWaypoint().name.c_str());
  }
}

inline void
OptionStartsWidget::Relocate(unsigned ItemIndex)
{
  assert(ItemIndex <= task.GetOptionalStartPointCount() + 2);
  assert(GetList().GetLength() == task.GetOptionalStartPointCount() + 2);
  assert(task.TaskSize() > 0);

  if (ItemIndex == 0)
    return;

  const unsigned index_optional_starts = ItemIndex - 1;

  const GeoPoint &location = task.GetPoint(0).GetLocation();
  auto way_point = ShowWaypointListDialog(waypoints, location);
  if (!way_point)
    return;

  if (index_optional_starts < task.GetOptionalStartPointCount()) {
    if (task.RelocateOptionalStart(index_optional_starts, std::move(way_point)))
      modified = true;
  } else {
    AbstractTaskFactory &factory = task.GetFactory();
    if (factory.AppendOptionalStart(std::move(way_point))) {
      modified = true;
    }
  }
  RefreshView();
}

inline void
OptionStartsWidget::Remove(unsigned i)
{
  --i;

  if (task.RemoveOptionalStart(i)) {
    RefreshView();
    modified = true;
  }
}

bool
dlgTaskOptionalStarts(Waypoints &waypoints, OrderedTask &task)
{
  assert(task.TaskSize() > 0);

  TWidgetDialog<OptionStartsWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(),
           _("Alternate Start Points"));
  dialog.SetWidget(waypoints, task);
  dialog.GetWidget().CreateButtons(dialog);
  dialog.EnableCursorSelection();

  dialog.ShowModal();

  return dialog.GetWidget().IsModified();
}
