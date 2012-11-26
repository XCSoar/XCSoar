/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Form/ListWidget.hpp"
#include "Profile/Profile.hpp"
#include "Profile/AirspaceConfig.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Features.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Components.hpp"
#include "Computer/GlideComputer.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

#include <assert.h>

enum Buttons {
  LOOKUP = 100,
};

class AirspaceSettingsListWidget : public ListWidget {
  const bool color_mode;
  bool changed;

public:
  AirspaceSettingsListWidget(bool _color_mode)
    :color_mode(_color_mode), changed(false) {}

  bool IsModified() const {
    return changed;
  }

  /* virtual methods from class Widget */

  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) gcc_override {
    ListControl &list = CreateList(parent, UIGlobals::GetDialogLook(), rc,
                                   Layout::Scale(18u));
    list.SetLength(AIRSPACECLASSCOUNT);
  }

  virtual void Unprepare() gcc_override {
    DeleteWindow();
  }

  /* virtual methods from class ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) gcc_override;

  /* virtual methods from class ListCursorHandler */
  virtual bool CanActivateItem(unsigned index) const gcc_override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) gcc_override;
};

void
AirspaceSettingsListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                         unsigned i)
{
  assert(i < AIRSPACECLASSCOUNT);

  const AirspaceComputerSettings &computer =
    CommonInterface::GetComputerSettings().airspace;
  const AirspaceRendererSettings &renderer =
    CommonInterface::GetMapSettings().airspace;
  const AirspaceLook &look = CommonInterface::main_window->GetLook().map.airspace;

  PixelScalar w0 = rc.right - rc.left - Layout::FastScale(4);

  PixelScalar w1 = canvas.CalcTextWidth(_("Warn")) + Layout::FastScale(10);
  PixelScalar w2 = canvas.CalcTextWidth(_("Display")) + Layout::FastScale(10);
  PixelScalar x0 = w0 - w1 - w2;

  if (color_mode) {
    if (AirspacePreviewRenderer::PrepareFill(
        canvas, (AirspaceClass)i, look, renderer)) {
      canvas.Rectangle(rc.left + x0, rc.top + Layout::FastScale(2),
                       rc.right - Layout::FastScale(2),
                       rc.bottom - Layout::FastScale(2));
      AirspacePreviewRenderer::UnprepareFill(canvas);
    }
    if (AirspacePreviewRenderer::PrepareOutline(
        canvas, (AirspaceClass)i, look, renderer)) {
      canvas.Rectangle(rc.left + x0, rc.top + Layout::FastScale(2),
                       rc.right - Layout::FastScale(2),
                       rc.bottom - Layout::FastScale(2));
    }
  } else {
    if (computer.warnings.class_warnings[i])
      canvas.DrawText(rc.left + w0 - w1 - w2, rc.top + Layout::FastScale(2),
                      _("Warn"));

    if (renderer.classes[i].display)
      canvas.DrawText(rc.left + w0 - w2, rc.top + Layout::FastScale(2),
                      _("Display"));
  }

  canvas.DrawClippedText(rc.left + Layout::FastScale(2),
                         rc.top + Layout::FastScale(2),
                         x0 - Layout::FastScale(10),
                         AirspaceFormatter::GetClass((AirspaceClass)i));
}

void
AirspaceSettingsListWidget::OnActivateItem(unsigned index)
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
    look.Initialise(renderer);
  } else {
    renderer.classes[index].display = !renderer.classes[index].display;
    if (!renderer.classes[index].display)
      computer.warnings.class_warnings[index] =
        !computer.warnings.class_warnings[index];

    Profile::SetAirspaceMode(index, renderer.classes[index].display,
                             computer.warnings.class_warnings[index]);
    changed = true;
    ActionInterface::SendMapSettings();
  }

  GetList().Invalidate();
}

void
dlgAirspaceShowModal(bool color_mode)
{
  AirspaceSettingsListWidget widget(color_mode);
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Airspace"), &widget);
  dialog.AddButton(_("Lookup"), LOOKUP);
  dialog.AddButton(_("Close"), mrOK);

  const int result = dialog.ShowModal();
  dialog.StealWidget();

  // now retrieve back the properties...
  if (widget.IsModified()) {
    if (!color_mode && glide_computer != NULL) {
      ProtectedAirspaceWarningManager::ExclusiveLease awm(glide_computer->GetAirspaceWarnings());
      awm->SetConfig(CommonInterface::SetComputerSettings().airspace.warnings);
    }

    Profile::Save();
  }

  if (result == LOOKUP)
    ShowAirspaceListDialog(airspace_database, GetAirspaceWarnings());
}
