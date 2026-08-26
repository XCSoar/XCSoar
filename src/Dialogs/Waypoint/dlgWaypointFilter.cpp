// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointDialogs.hpp"
#include "WaypointTypeName.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Profile/Profile.hpp"
#include "Renderer/WaypointRendererSettings.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "Engine/Waypoint/Waypoint.hpp"

#include <cassert>

class WaypointFilterListWidget : public ListWidget {
  bool changed;
  TextRowRenderer row_renderer;

public:
  WaypointFilterListWidget() noexcept
    :changed(false) {}

  bool IsModified() const noexcept {
    return changed;
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    const auto &look = UIGlobals::GetDialogLook();
    ListControl &list =
      CreateList(parent, look, rc,
                 row_renderer.CalculateLayout(*look.list.font));
    list.SetLength(unsigned(Waypoint::Type::COUNT));
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  bool CanActivateItem([[maybe_unused]] unsigned index) const
    noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override;
};

void
WaypointFilterListWidget::OnPaintItem(Canvas &canvas, PixelRect rc,
                                      unsigned i) noexcept
{
  assert(i < unsigned(Waypoint::Type::COUNT));
  const auto type = Waypoint::Type(i);
  const WaypointRendererSettings &settings =
    CommonInterface::GetMapSettings().waypoint;

  const bool display = settings.IsTypeDisplayed(type);

  /* "Visible" / "Hidden" so a hidden row is not a blank column.
     Reserve the wider word so the name does not shift. */
  const char *visible_label = _("Visible");
  const char *hidden_label = _("Hidden");
  const char *label = display ? visible_label : hidden_label;
  const char *reserve =
    canvas.CalcTextWidth(visible_label) >=
    canvas.CalcTextWidth(hidden_label)
    ? visible_label : hidden_label;

  row_renderer.DrawRightColumn(canvas, rc, label);
  rc.right = row_renderer.PreviousRightColumn(canvas, rc, reserve);

  row_renderer.DrawTextRow(canvas, rc, GetWaypointTypeName(type));
}

void
WaypointFilterListWidget::OnActivateItem(unsigned index) noexcept
{
  assert(index < unsigned(Waypoint::Type::COUNT));

  WaypointRendererSettings &settings =
    CommonInterface::SetMapSettings().waypoint;

  const auto type = Waypoint::Type(index);
  settings.SaveTypeDisplay(type, !settings.IsTypeDisplayed(type));

  changed = true;
  ActionInterface::SendMapSettings();
  GetList().Invalidate();
}

void
dlgWaypointFilterShowModal() noexcept
{
  TWidgetDialog<WaypointFilterListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(),
           _("Waypoints"));
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget();

  dialog.ShowModal();

  if (dialog.GetWidget().IsModified())
    Profile::Save();
}
