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

#include "Dialogs/Airspace.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/List.hpp"
#include "Form/Button.hpp"
#include "Profile/Profile.hpp"
#include "Profile/AirspaceConfig.hpp"
#include "Profile/ProfileKeys.hpp"
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

static WndForm *wf = NULL;
static ListControl *airspace_list = NULL;

static bool color_mode = false;
static bool changed = false;

static void
OnAirspacePaintListItem(Canvas &canvas, const PixelRect rc, unsigned i)
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
      canvas.text(rc.left + w0 - w1 - w2, rc.top + Layout::FastScale(2),
                  _("Warn"));

    if (renderer.classes[i].display)
      canvas.text(rc.left + w0 - w2, rc.top + Layout::FastScale(2),
                  _("Display"));
  }

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2), x0 - Layout::FastScale(10),
                      AirspaceFormatter::GetClass((AirspaceClass)i));
}

static void
OnAirspaceListEnter(unsigned index)
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
    airspace_list->Invalidate();
  } else {
    renderer.classes[index].display = !renderer.classes[index].display;
    if (!renderer.classes[index].display)
      computer.warnings.class_warnings[index] =
        !computer.warnings.class_warnings[index];

    Profile::SetAirspaceMode(index, renderer.classes[index].display,
                             computer.warnings.class_warnings[index]);
    changed = true;
    ActionInterface::SendMapSettings();
    airspace_list->Invalidate();
  }
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
OnLookupClicked(gcc_unused WndButton &Sender)
{
  ShowAirspaceListDialog(airspace_database, GetAirspaceWarnings());
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnLookupClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgAirspaceShowModal(bool _color_mode)
{
  color_mode = _color_mode;

  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  Layout::landscape ? _T("IDR_XML_AIRSPACE_L") :
                                      _T("IDR_XML_AIRSPACE"));
  assert(wf != NULL);

  airspace_list = (ListControl*)wf->FindByName(_T("frmAirspaceList"));
  assert(airspace_list != NULL);
  airspace_list->SetActivateCallback(OnAirspaceListEnter);
  airspace_list->SetPaintItemCallback(OnAirspacePaintListItem);
  airspace_list->SetLength(AIRSPACECLASSCOUNT);

  changed = false;

  wf->ShowModal();
  delete wf;

  // now retrieve back the properties...
  if (changed) {
    if (!color_mode && glide_computer != NULL) {
      ProtectedAirspaceWarningManager::ExclusiveLease awm(glide_computer->GetAirspaceWarnings());
      awm->SetConfig(CommonInterface::SetComputerSettings().airspace.warnings);
    }

    Profile::Save();
  }
}
