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

#include "WaypointDialogs.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Renderer/WaypointListRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Protection.hpp"
#include "UtilsSettings.hpp"
#include "Components.hpp"
#include "Waypoint/WaypointList.hpp"
#include "Waypoint/WaypointListBuilder.hpp"
#include "Waypoint/WaypointFilter.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

/* this macro exists in the WIN32 API */
#ifdef DELETE
#undef DELETE
#endif

class WaypointManagerWidget final
  : public ListWidget, private ActionListener {
  enum Buttons {
    NEW,
    IMPORT,
    EDIT,
    SAVE,
    DELETE,
  };

  Button *new_button, *edit_button, *save_button, *delete_button;

  WaypointList items;

  TwoTextRowsRenderer row_renderer;

  bool modified;

public:
  WaypointManagerWidget():modified(false) {}

  void CreateButtons(WidgetDialog &dialog);

  bool IsModified() const {
    return modified;
  }

  void SaveWaypoints();

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

  void Unprepare() override {
    DeleteWindow();
  }

  void Show(const PixelRect &rc) override {
    ListWidget::Show(rc);
    UpdateList();
    UpdateButtons();
  }

private:
  /* virtual methods from ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) override;

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem(unsigned index) const override {
    return true;
  }

  void OnActivateItem(unsigned index) override;

  /* virtual methods from ActionListener */
  void OnAction(int id) override;

private:
  void UpdateList();
  void UpdateButtons();

  void OnWaypointNewClicked();
  void OnWaypointImportClicked();
  void OnWaypointEditClicked(unsigned i);
  void OnWaypointSaveClicked();
  void OnWaypointDeleteClicked(unsigned i);
};

void
WaypointManagerWidget::CreateButtons(WidgetDialog &dialog)
{
  new_button = dialog.AddButton(_("New"), *this, NEW);
  edit_button = dialog.AddButton(_("Import"), *this, IMPORT);
  edit_button = dialog.AddButton(_("Edit"), *this, EDIT);
  save_button = dialog.AddButton(_("Save"), *this, SAVE);
  delete_button = dialog.AddButton(_("Delete"), *this, DELETE);
}

void
WaypointManagerWidget::UpdateButtons()
{
  const bool non_empty = !items.empty();
  edit_button->SetEnabled(non_empty);
  delete_button->SetEnabled(non_empty);
}

void
WaypointManagerWidget::UpdateList()
{
  items.clear();

  WaypointFilter filter;
  filter.Clear();
  filter.type_index = TypeFilter::USER;

  WaypointListBuilder builder(filter, GeoPoint::Invalid(),
                              items, nullptr, 0);
  builder.Visit(way_points);

  auto &list = GetList();
  list.SetLength(items.size());
  list.SetOrigin(0);
  list.SetCursorIndex(0);
  list.Invalidate();
}

void
WaypointManagerWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font_bold,
                                          look.small_font));
}

void
WaypointManagerWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                   unsigned i)
{
  assert(i < items.size());

  const auto &info = items[i];

  WaypointListRenderer::Draw(canvas, rc, *info.waypoint,
                             row_renderer,
                             UIGlobals::GetMapLook().waypoint,
                             CommonInterface::GetMapSettings().waypoint);
}

void
WaypointManagerWidget::OnActivateItem(unsigned i)
{
  OnWaypointEditClicked(i);
}

inline void
WaypointManagerWidget::OnWaypointNewClicked()
{
  Waypoint edit_waypoint = way_points.Create(CommonInterface::Basic().location);
  edit_waypoint.elevation = CommonInterface::Calculated().terrain_valid
    ? CommonInterface::Calculated().terrain_altitude
    : CommonInterface::Basic().nav_altitude;

  if (dlgWaypointEditShowModal(edit_waypoint) &&
      edit_waypoint.name.size()) {
    modified = true;

    {
      ScopeSuspendAllThreads suspend;
      way_points.Append(std::move(edit_waypoint));
      way_points.Optimise();
    }

    UpdateList();
  }
}

inline void
WaypointManagerWidget::OnWaypointImportClicked()
{
  const auto way_point =
    ShowWaypointListDialog(CommonInterface::Basic().location);
  if (way_point) {
    Waypoint wp_copy = *way_point;

    /* move to user.cup */
    wp_copy.origin = WaypointOrigin::USER;

    if (dlgWaypointEditShowModal(wp_copy)) {
      modified = true;

      {
        ScopeSuspendAllThreads suspend;
        way_points.Replace(way_point, std::move(wp_copy));
        way_points.Optimise();
      }

      UpdateList();
    }
  }
}

inline void
WaypointManagerWidget::OnWaypointEditClicked(unsigned i)
{
  const WaypointPtr &wp = items[i].waypoint;
  Waypoint wp_copy = *wp;
  if (dlgWaypointEditShowModal(wp_copy)) {
    modified = true;

    ScopeSuspendAllThreads suspend;
    way_points.Replace(wp, std::move(wp_copy));
    way_points.Optimise();
  }
}

void
WaypointManagerWidget::SaveWaypoints()
{
  try {
    WaypointGlue::SaveWaypoints(way_points);
    WaypointFileChanged = true;
  } catch (const std::runtime_error &e) {
    ShowError(e, _("Failed to save waypoints"));
  }

  modified = false;
}

inline void
WaypointManagerWidget::OnWaypointSaveClicked()
{
  SaveWaypoints();
}

inline void
WaypointManagerWidget::OnWaypointDeleteClicked(unsigned i)
{
  WaypointPtr &&wp = std::move(items[i].waypoint);

  if (ShowMessageBox(wp->name.c_str(), _("Delete waypoint?"),
                     MB_YESNO | MB_ICONQUESTION) == IDYES) {
    modified = true;

    {
      ScopeSuspendAllThreads suspend;
      way_points.Erase(std::move(wp));
      way_points.Optimise();
    }

    UpdateList();
  }
}

void
WaypointManagerWidget::OnAction(int id)
{
  switch (Buttons(id)) {
  case NEW:
    OnWaypointNewClicked();
    break;

  case IMPORT:
    OnWaypointImportClicked();
    break;

  case EDIT:
    OnWaypointEditClicked(GetList().GetCursorIndex());
    break;

  case SAVE:
    OnWaypointSaveClicked();
    break;

  case DELETE:
    OnWaypointDeleteClicked(GetList().GetCursorIndex());
    break;
  }
}

void
dlgConfigWaypointsShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WaypointManagerWidget widget;
  WidgetDialog dialog(look);
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _("Waypoints Editor"),
                    &widget);
  widget.CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrCancel);
  dialog.EnableCursorSelection();

  dialog.ShowModal();
  dialog.StealWidget();

  if (widget.IsModified() &&
      ShowMessageBox(_("Save changes to waypoint file?"), _("Waypoints edited"),
                  MB_YESNO | MB_ICONQUESTION) == IDYES)
      widget.SaveWaypoints();
}
