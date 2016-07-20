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

#include "TaskDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Widget/ListWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Unordered/AlternateList.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Renderer/WaypointListRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Language/Language.hpp"

class AlternatesListWidget final
  : public ListWidget, private ActionListener {
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
    ProtectedTaskManager::Lease lease(*protected_task_manager);
    alternates = lease->GetAlternates();
    return !alternates.empty();
  }

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  void Unprepare() override {
    DeleteWindow();
  }

  /* virtual methods from class List::Handler */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned index) override {
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

  bool CanActivateItem(unsigned index) const  override{
    return true;
  }

  void OnActivateItem(unsigned index) override;

  /* virtual methods from class ActionListener */
  void OnAction(int id) override;
};

void
AlternatesListWidget::CreateButtons(WidgetDialog &dialog)
{
  goto_button = dialog.AddButton(_("Goto"), *this, GOTO);
  details_button = dialog.AddButton(_("Details"), mrOK);
  cancel_button = dialog.AddButton(_("Close"), mrCancel);
}

void
AlternatesListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  CreateList(parent, dialog_look, rc,
             row_renderer.CalculateLayout(*dialog_look.list.font_bold,
                                          dialog_look.small_font));

  GetList().SetLength(alternates.size());
}

void
AlternatesListWidget::OnActivateItem(unsigned index)
{
  details_button->Click();
}

void
AlternatesListWidget::OnAction(int id)
{
  switch (id) {
  case GOTO:
    unsigned index = GetCursorIndex();
    assert(index < alternates.size());

    auto const &item = alternates[index];
    auto const &waypoint = item.waypoint;

    protected_task_manager->DoGoto(waypoint);
    cancel_button->Click();

    break;
  }
}

void
dlgAlternatesListShowModal()
{
  if (protected_task_manager == nullptr)
    return;

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  AlternatesListWidget widget(dialog_look);
  if (!widget.Update())
    /* no alternates: don't show the dialog */
    return;

  WidgetDialog dialog(dialog_look);
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Alternates"), &widget);
  widget.CreateButtons(dialog);
  dialog.EnableCursorSelection();

  int i = dialog.ShowModal() == mrOK
    ? (int)widget.GetCursorIndex()
    : -1;
  dialog.StealWidget();

  if (i < 0 || (unsigned)i >= widget.alternates.size())
    return;

  dlgWaypointDetailsShowModal(widget.alternates[i].waypoint, false);
}
