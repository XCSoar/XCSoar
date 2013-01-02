/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "TaskDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Form/Form.hpp"
#include "Form/List.hpp"
#include "Form/Button.hpp"
#include "Widget/ListWidget.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"

#include <assert.h>

class OptionStartsWidget : public ListWidget, private ActionListener {
  enum Buttons {
    RELOCATE,
    REMOVE,
  };

  OrderedTask &task;
  const bool RealStartExists;
  bool modified;

  WndButton *relocate_button, *remove_button;

public:
  OptionStartsWidget(OrderedTask &_task)
    :task(_task), RealStartExists(task.TaskSize() > 0),
     modified(false) {}

  void CreateButtons(WidgetDialog &dialog) {
    relocate_button = dialog.AddButton(_("Relocate"), *this, RELOCATE);
    remove_button = dialog.AddButton(_("Remove"), *this, REMOVE);
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
    GetList().SetLength(task.GetOptionalStartPointCount()
                        + (RealStartExists ? 2 : 1));
    GetList().Invalidate();
  }

  void UpdateButtons() {
    const unsigned current = GetCursorIndex();
    relocate_button->SetEnabled(!RealStartExists || current >= 1);
    remove_button->SetEnabled((!RealStartExists || current >= 1) &&
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
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) gcc_override;
  virtual void Unprepare() gcc_override {
    DeleteWindow();
  }

  /* virtual methods from class List::Handler */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) gcc_override;

  virtual void OnCursorMoved(unsigned index) gcc_override {
    UpdateButtons();
  }

  virtual bool CanActivateItem(unsigned index) const gcc_override {
    if (index == 0 && RealStartExists)
      return false;

    return true;
  }

  virtual void OnActivateItem(unsigned index) gcc_override {
    Relocate(index);
  }

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) gcc_override;
};

void
OptionStartsWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  CreateList(parent, UIGlobals::GetDialogLook(),
             rc, Layout::GetMaximumControlHeight());

  RefreshView();
}

void
OptionStartsWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                unsigned DrawListIndex)
{
  assert(DrawListIndex <= task.GetOptionalStartPointCount()
         + (RealStartExists ? 2 : 1));
  assert(GetList().GetLength() ==
         task.GetOptionalStartPointCount() + (RealStartExists ? 2 : 1));

  const unsigned index_optional_starts = DrawListIndex - (RealStartExists ? 1 : 0);

  if (DrawListIndex == GetList().GetLength() - 1) {
    canvas.DrawText(rc.left + Layout::FastScale(2),
                    rc.top + Layout::FastScale(2),
                    _("(Add Alternate Start)"));
  } else {
    RasterPoint pt(rc.left + Layout::FastScale(2),
                   rc.top + Layout::FastScale(2));

    const OrderedTaskPoint *tp;
    if (DrawListIndex == 0 && RealStartExists) {
      tp = &task.GetPoint(0);
      canvas.DrawText(pt.x, pt.y, _T("*"));
      pt.x += canvas.CalcTextWidth(_T("*"));
    } else
      tp = &task.GetOptionalStartPoint(index_optional_starts);

    assert(tp != NULL);

    canvas.DrawText(pt.x, pt.y, tp->GetWaypoint().name.c_str());
  }
}

void
OptionStartsWidget::OnAction(int id)
{
  switch (id) {
  case RELOCATE:
    Relocate(GetList().GetCursorIndex());
    break;

  case REMOVE:
    Remove(GetList().GetCursorIndex());
    break;
  }
}

void
OptionStartsWidget::Relocate(unsigned ItemIndex)
{
  assert(ItemIndex <= task.GetOptionalStartPointCount()
      +  (RealStartExists ? 2 : 1));
  assert(GetList().GetLength() ==
         task.GetOptionalStartPointCount() + (RealStartExists ? 2 : 1));

  if (ItemIndex == 0 && RealStartExists)
    return;

  const unsigned index_optional_starts = ItemIndex - (RealStartExists ? 1 : 0);

  if (index_optional_starts < task.GetOptionalStartPointCount()) {
    const GeoPoint &location = task.TaskSize() > 0
      ? task.GetPoint(0).GetLocation()
      : CommonInterface::Basic().location;
    const Waypoint* way_point =
      ShowWaypointListDialog(UIGlobals::GetMainWindow(), location);
    if (!way_point)
      return;

    if (task.RelocateOptionalStart(index_optional_starts, *way_point))
      modified = true;

  } else if (!task.IsFull()) {

    const GeoPoint &location = task.TaskSize() > 0
      ? task.GetPoint(0).GetLocation()
      : CommonInterface::Basic().location;
    const Waypoint* way_point =
      ShowWaypointListDialog(UIGlobals::GetMainWindow(), location);
    if (!way_point)
      return;

    AbstractTaskFactory &factory = task.GetFactory();
    if (factory.AppendOptionalStart(*way_point)) {
      modified = true;
    }
  }
  RefreshView();
}

void
OptionStartsWidget::Remove(unsigned i)
{
  if (RealStartExists)
    --i;

  if (task.RemoveOptionalStart(i)) {
    RefreshView();
    modified = true;
  }
}

bool
dlgTaskOptionalStarts(SingleWindow &parent, OrderedTask** task)
{
  OptionStartsWidget widget(**task);
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateFull(parent, _("Alternate Start Points"), &widget);
  widget.CreateButtons(dialog);

  dialog.ShowModal();
  dialog.StealWidget();

  return widget.IsModified();
}
