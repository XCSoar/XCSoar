// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
  : public ListWidget {

  Waypoints &way_points;

  Button *new_button, *edit_button, *save_button, *delete_button;

  WaypointList items;

  TwoTextRowsRenderer row_renderer;

  bool modified = false;

public:
  explicit WaypointManagerWidget(Waypoints &_waypoints) noexcept
    :way_points(_waypoints) {}

  void CreateButtons(WidgetDialog &dialog);

  bool IsModified() const {
    return modified;
  }

  void SaveWaypoints();

  /* virtual methods from Widget */
  void Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    ListWidget::Show(rc);
    Update();
  }

private:
  /* virtual methods from ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override;

private:
  void UpdateList();
  void UpdateButtons();

  void Update() noexcept {
    UpdateList();
    UpdateButtons();
  }

  void OnWaypointNewClicked();
  void OnWaypointImportClicked();
  void OnWaypointEditClicked(unsigned i);
  void OnWaypointSaveClicked();
  void OnWaypointDeleteClicked(unsigned i);
};

void
WaypointManagerWidget::CreateButtons(WidgetDialog &dialog)
{
  new_button = dialog.AddButton(_("New"), [this](){
    OnWaypointNewClicked();
  });

  edit_button = dialog.AddButton(_("Import"), [this](){
    OnWaypointImportClicked();
  });

  edit_button = dialog.AddButton(_("Edit"), [this](){
    OnWaypointEditClicked(GetList().GetCursorIndex());
  });

  save_button = dialog.AddButton(_("Save"), [this](){
    OnWaypointSaveClicked();
  });

  delete_button = dialog.AddButton(_("Delete"), [this](){
    OnWaypointDeleteClicked(GetList().GetCursorIndex());
  });
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
WaypointManagerWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font_bold,
                                          look.small_font));
}

void
WaypointManagerWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                   unsigned i) noexcept
{
  assert(i < items.size());

  const auto &info = items[i];

  WaypointListRenderer::Draw(canvas, rc, *info.waypoint,
                             row_renderer,
                             UIGlobals::GetMapLook().waypoint,
                             CommonInterface::GetMapSettings().waypoint);
}

void
WaypointManagerWidget::OnActivateItem(unsigned i) noexcept
{
  OnWaypointEditClicked(i);
}

inline void
WaypointManagerWidget::OnWaypointNewClicked()
{
  Waypoint edit_waypoint = way_points.Create(CommonInterface::Basic().location);

  if (CommonInterface::Calculated().terrain_valid) {
    edit_waypoint.elevation = CommonInterface::Calculated().terrain_altitude;
    edit_waypoint.has_elevation = true;
  } else if (CommonInterface::Basic().NavAltitudeAvailable()) {
    edit_waypoint.elevation = CommonInterface::Basic().nav_altitude;
    edit_waypoint.has_elevation = true;
  }

  if (dlgWaypointEditShowModal(edit_waypoint) == WaypointEditResult::MODIFIED &&
      edit_waypoint.name.size()) {
    modified = true;

    {
      ScopeSuspendAllThreads suspend;
      way_points.Append(std::move(edit_waypoint));
      way_points.Optimise();
    }

    Update();
  }
}

inline void
WaypointManagerWidget::OnWaypointImportClicked()
{
  const auto way_point =
    ShowWaypointListDialog(way_points, CommonInterface::Basic().location);
  if (way_point) {
    Waypoint wp_copy = *way_point;

    /* move to user.cup */
    wp_copy.origin = WaypointOrigin::USER;

    if (dlgWaypointEditShowModal(wp_copy) != WaypointEditResult::CANCEL) {
      modified = true;

      {
        ScopeSuspendAllThreads suspend;
        way_points.Replace(way_point, std::move(wp_copy));
        way_points.Optimise();
      }

      Update();
    }
  }
}

inline void
WaypointManagerWidget::OnWaypointEditClicked(unsigned i)
{
  const WaypointPtr &wp = items[i].waypoint;
  Waypoint wp_copy = *wp;
  if (dlgWaypointEditShowModal(wp_copy) == WaypointEditResult::MODIFIED) {
    modified = true;

    {
      ScopeSuspendAllThreads suspend;
      way_points.Replace(wp, std::move(wp_copy));
      way_points.Optimise();
    }

    Update();
  }
}

void
WaypointManagerWidget::SaveWaypoints()
{
  try {
    WaypointGlue::SaveWaypoints(way_points);
    WaypointFileChanged = true;
  } catch (...) {
    ShowError(std::current_exception(), _("Failed to save waypoints"));
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

    Update();
  }
}

void
dlgConfigWaypointsShowModal(Waypoints &waypoints) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<WaypointManagerWidget>
    dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
           look, _("Waypoint Editor"));
  dialog.SetWidget(waypoints);
  dialog.GetWidget().CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrCancel);
  dialog.EnableCursorSelection();

  dialog.ShowModal();

  if (dialog.GetWidget().IsModified() &&
      ShowMessageBox(_("Save changes to waypoint file?"), _("Waypoints edited"),
                  MB_YESNO | MB_ICONQUESTION) == IDYES)
      dialog.GetWidget().SaveWaypoints();
}
