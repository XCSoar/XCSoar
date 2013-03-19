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

/**
 * @file
 * The FLARM Traffic dialog displaying a radar screen with the moving
 * FLARM targets in track up orientation. The target can be selected and basic
 * information is given on the selected target. When a warning/alarm is present
 * the target with the highest alarm level is automatically selected and
 * highlighted in orange or red (depending on the level)
 */

#include "Dialogs/Traffic/TrafficDialogs.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Math/Screen.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Canvas.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Profile/Profile.hpp"
#include "Compiler.h"
#include "FLARM/Friends.hpp"
#include "Look/FlarmTrafficLook.hpp"
#include "Gauge/FlarmTrafficWindow.hpp"
#include "Gauge/FlarmTrafficControl.hpp"
#include "Language/Language.hpp"
#include "UIUtil/GestureManager.hpp"
#include "Formatter/UserUnits.hpp"
#include "Units/Units.hpp"
#include "Renderer/UnitSymbolRenderer.hpp"
#include "Interface.hpp"
#include "Event/LambdaTimer.hpp"

static WndForm *wf = NULL;
static FlarmTrafficControl *wdf;
static CheckBox *auto_zoom, *north_up;

static void
OpenDetails()
{
  // If warning is displayed -> prevent from opening details dialog
  if (wdf->WarningMode())
    return;

  // Don't open the details dialog if no plane selected
  const FlarmTraffic *traffic = wdf->GetTarget();
  if (traffic == NULL)
    return;

  // Show the details dialog
  dlgFlarmTrafficDetailsShowModal(traffic->id);
}

/**
 * This event handler is called when the "Details" button is pressed
 */
static void
OnDetailsClicked(gcc_unused WndButton &button)
{
  OpenDetails();
}

/**
 * This event handler is called when the "ZoomIn (+)" button is pressed
 */
static void
OnZoomInClicked(gcc_unused WndButton &button)
{
  wdf->ZoomIn();
}

/**
 * This event handler is called when the "ZoomOut (-)" button is pressed
 */
static void
OnZoomOutClicked(gcc_unused WndButton &button)
{
  wdf->ZoomOut();
}

/**
 * This event handler is called when the "Prev (<)" button is pressed
 */
static void
OnPrevClicked(gcc_unused WndButton &button)
{
  wdf->PrevTarget();
}

/**
 * This event handler is called when the "Next (>)" button is pressed
 */
static void
OnNextClicked(gcc_unused WndButton &button)
{
  wdf->NextTarget();
}

/**
 * This event handler is called when the "Close" button is pressed
 */
static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static void
SwitchData()
{
  if (wdf->side_display_type == FlarmTrafficWindow::SIDE_INFO_VARIO)
    wdf->side_display_type = FlarmTrafficWindow::SIDE_INFO_RELATIVE_ALTITUDE;
  else
    wdf->side_display_type = FlarmTrafficWindow::SIDE_INFO_VARIO;

  Profile::SetEnum(ProfileKeys::FlarmSideData, wdf->side_display_type);
}

/**
 * This event handler is called when the "Avg/Alt" button is pressed
 */
static void
OnSwitchDataClicked(gcc_unused WndButton &button)
{
  SwitchData();
}

static void
OnAutoZoom(CheckBoxControl &control)
{
  wdf->SetAutoZoom(control.GetState());
}

static void
OnNorthUp(CheckBoxControl &control)
{
  wdf->SetNorthUp(control.GetState());
}

/**
 * This event handler is called when a key is pressed
 * @param key_code The key code of the pressed key
 * @return True if the event was handled, False otherwise
 */
static bool
FormKeyDown(unsigned key_code)
{
  switch (key_code) {
  case KEY_UP:
    if (!HasPointer())
      break;

    wdf->ZoomIn();
    return true;
  case KEY_DOWN:
    if (!HasPointer())
      break;

    wdf->ZoomOut();
    return true;
  case KEY_LEFT:
#ifdef GNAV
  case '6':
#endif
    wdf->PrevTarget();
    return true;
  case KEY_RIGHT:
#ifdef GNAV
  case '7':
#endif
    wdf->NextTarget();
    return true;
  }

  return false;
}

static void
SetButtonsEnabled(bool enabled)
{
  SetFormControlEnabled(*wf, _T("cmdSwitchData"), enabled);
  SetFormControlEnabled(*wf, _T("cmdDetails"), enabled);
  SetFormControlEnabled(*wf, _T("cmdZoomIn"), enabled);
  SetFormControlEnabled(*wf, _T("cmdZoomOut"), enabled);
  SetFormControlEnabled(*wf, _T("cmdPrev"), enabled);
  SetFormControlEnabled(*wf, _T("cmdNext"), enabled);
}

static void
Update()
{
  if (CommonInterface::GetUISettings().traffic.auto_close_dialog &&
      CommonInterface::Basic().flarm.traffic.IsEmpty())
    wf->SetModalResult(mrOK);

  wdf->Update(CommonInterface::Basic().track,
              CommonInterface::Basic().flarm.traffic,
              CommonInterface::GetComputerSettings().team_code);

  wdf->UpdateTaskDirection(CommonInterface::Calculated().task_stats.task_valid &&
                           CommonInterface::Calculated().task_stats.current_leg.solution_remaining.IsOk(),
                           CommonInterface::Calculated().task_stats.
                           current_leg.solution_remaining.cruise_track_bearing);

  SetButtonsEnabled(!wdf->WarningMode());
}

static Window *
OnCreateFlarmTrafficControl(ContainerWindow &parent, PixelRect rc,
                            const WindowStyle style)
{
  const Look &look = UIGlobals::GetLook();
  wdf = new FlarmTrafficControl(look.flarm_dialog);
  wdf->Create(parent, rc, style);

  return wdf;
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCreateFlarmTrafficControl),
  DeclareCallBackEntry(OnDetailsClicked),
  DeclareCallBackEntry(OnZoomInClicked),
  DeclareCallBackEntry(OnZoomOutClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnSwitchDataClicked),
  DeclareCallBackEntry(OnAutoZoom),
  DeclareCallBackEntry(OnNorthUp),
  DeclareCallBackEntry(NULL)
};

/**
 * The function opens the FLARM Traffic dialog
 */
void
dlgFlarmTrafficShowModal()
{
  if (wf)
    return;

  // Load dialog from XML
  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  Layout::landscape ? _T("IDR_XML_FLARMTRAFFIC_L") :
                                      _T("IDR_XML_FLARMTRAFFIC"));
  assert(wf != NULL);

  // Set dialog events
  wf->SetKeyDownFunction(FormKeyDown);

  auto update_timer = MakeLambdaTimer([](){ Update(); });
  update_timer.Schedule(500);

  wdf->SetAutoZoomChangeFunction([](bool enabled) {
    auto_zoom->SetState(enabled);
  });

  wdf->SetNorthUpChangeFunction([](bool enabled) {
    north_up->SetState(enabled);
  });

  // Update Radar and Selection for the first time
  Update();

  // Get the last chosen Side Data configuration
  auto_zoom = (CheckBox *)wf->FindByName(_T("AutoZoom"));
  auto_zoom->SetState(wdf->GetAutoZoom());

  north_up = (CheckBox *)wf->FindByName(_T("NorthUp"));
  north_up->SetState(wdf->GetNorthUp());

  // Show the dialog
  wf->ShowModal();

  update_timer.Cancel();

  // After dialog closed -> delete it
  delete wf;
  wf = NULL;
}
