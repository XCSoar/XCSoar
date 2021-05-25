/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Airspace.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Profile/Current.hpp"
#include "Profile/Profile.hpp"
#include "Profile/AirspaceConfig.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "MainWindow.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"

#include <cassert>

class AirspaceSettingsListWidget : public ListWidget {
  const bool color_mode;
  bool changed;

  TextRowRenderer row_renderer;

public:
  AirspaceSettingsListWidget(bool _color_mode)
    :color_mode(_color_mode), changed(false) {}

  bool IsModified() const {
    return changed;
  }

  /* virtual methods from class Widget */

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    const auto &look = UIGlobals::GetDialogLook();
    ListControl &list = CreateList(parent, look, rc,
                                   row_renderer.CalculateLayout(*look.list.font));
    list.SetLength(AIRSPACECLASSCOUNT);
  }

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from class ListCursorHandler */
  bool CanActivateItem(unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override;
};

void
AirspaceSettingsListWidget::OnPaintItem(Canvas &canvas, PixelRect rc,
                                        unsigned i) noexcept
{
  assert(i < AIRSPACECLASSCOUNT);

  const AirspaceComputerSettings &computer =
    CommonInterface::GetComputerSettings().airspace;
  const AirspaceRendererSettings &renderer =
    CommonInterface::GetMapSettings().airspace;
  const AirspaceLook &look = CommonInterface::main_window->GetLook().map.airspace;

  const TCHAR *const name = AirspaceFormatter::GetClass((AirspaceClass)i);

  if (color_mode) {
    int second_x = row_renderer.NextColumn(canvas, rc, name);

    const int padding = Layout::GetTextPadding();

    if (AirspacePreviewRenderer::PrepareFill(
        canvas, (AirspaceClass)i, look, renderer)) {
      canvas.DrawRectangle({second_x, rc.top + padding, rc.right - padding, rc.bottom - padding});
      AirspacePreviewRenderer::UnprepareFill(canvas);
    }
    if (AirspacePreviewRenderer::PrepareOutline(
        canvas, (AirspaceClass)i, look, renderer)) {
      canvas.DrawRectangle({second_x, rc.top + padding, rc.right - padding, rc.bottom - padding});
    }
  } else {
    rc.right = renderer.classes[i].display
      ? row_renderer.DrawRightColumn(canvas, rc, _("Display"))
      : row_renderer.PreviousRightColumn(canvas, rc, _("Display"));

    rc.right = computer.warnings.class_warnings[i]
      ? row_renderer.DrawRightColumn(canvas, rc, _("Warn"))
      : row_renderer.PreviousRightColumn(canvas, rc, _("Warn"));
  }

  row_renderer.DrawTextRow(canvas, rc, name);
}

void
AirspaceSettingsListWidget::OnActivateItem(unsigned index) noexcept
{
  assert(index < AIRSPACECLASSCOUNT);

  AirspaceComputerSettings &computer =
    CommonInterface::SetComputerSettings().airspace;
  AirspaceRendererSettings &renderer =
    CommonInterface::SetMapSettings().airspace;

  if (color_mode) {
    AirspaceLook &look =
      CommonInterface::main_window->SetLook().map.airspace;

    if (!ShowAirspaceClassRendererSettingsDialog((AirspaceClass)index))
      return;

    ActionInterface::SendMapSettings();
    look.Reinitialise(renderer);
  } else {
    renderer.classes[index].display = !renderer.classes[index].display;
    if (!renderer.classes[index].display)
      computer.warnings.class_warnings[index] =
        !computer.warnings.class_warnings[index];

    Profile::SetAirspaceMode(Profile::map,
                             index, renderer.classes[index].display,
                             computer.warnings.class_warnings[index]);
    changed = true;
    ActionInterface::SendMapSettings();
  }

  GetList().Invalidate();
}

void
dlgAirspaceShowModal(bool color_mode)
{
  TWidgetDialog<AirspaceSettingsListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(),
           _("Airspace"));
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget(color_mode);

  dialog.ShowModal();

  // now retrieve back the properties...
  if (dialog.GetWidget().IsModified()) {
    Profile::Save();
  }
}
