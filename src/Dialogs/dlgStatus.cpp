/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Internal.hpp"
#include "StatusPanels/FlightStatusPanel.hpp"
#include "StatusPanels/TaskStatusPanel.hpp"
#include "StatusPanels/RulesStatusPanel.hpp"
#include "StatusPanels/SystemStatusPanel.hpp"
#include "StatusPanels/TimesStatusPanel.hpp"
#include "Screen/Key.h"
#include "Protection.hpp"
#include "Blackboard.hpp"
#include "Math/Earth.hpp"
#include "Hardware/Battery.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Logger/Logger.hpp"
#include "Math/FastMath.h"
#include "LocalTime.hpp"
#include "MainWindow.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "Compiler.h"

#include "Form/TabBar.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"

#include <assert.h>
#include <stdio.h>

#include <algorithm>

static WndForm *wf = NULL;
static TabBarControl *wTabBar;
static StatusPanel *system_panel;
static int status_page = 0;

static void
SetTitle()
{
  TCHAR title[99];
  _stprintf(title, _T("%s: %s"), _("Status"),
            wTabBar->GetButtonCaption((wTabBar->GetCurrentPage())));
  wf->SetCaption(title);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  if (wTabBar->GetCurrentWidget() == system_panel)
    system_panel->Refresh();
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgStatusShowModal(int start_page)
{
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ?
                  _T("IDR_XML_STATUS_L") : _T("IDR_XML_STATUS"));
  assert(wf);

  wTabBar = ((TabBarControl *)wf->FindByName(_T("TabBar")));
  assert(wTabBar != NULL);
  wTabBar->SetPageFlippedCallback(SetTitle);

  const Waypoint *nearest_waypoint =
    way_points.get_nearest(CommonInterface::Basic().location, fixed(100000));

  /* setup tabs */

  const DialogTabStyle_t IconsStyle =
    CommonInterface::GetUISettings().dialog.tab_style;

  const Bitmap *FlightIcon = ((IconsStyle == dtIcon) ?
                              &Graphics::hBmpTabFlight : NULL);
  const Bitmap *SystemIcon = ((IconsStyle == dtIcon) ?
                               &Graphics::hBmpTabSystem : NULL);
  const Bitmap *TaskIcon = ((IconsStyle == dtIcon) ?
                              &Graphics::hBmpTabTask : NULL);
  const Bitmap *RulesIcon = ((IconsStyle == dtIcon) ?
                             &Graphics::hBmpTabRules : NULL);
  const Bitmap *TimesIcon = ((IconsStyle == dtIcon) ?
                             &Graphics::hBmpTabTimes : NULL);

  Widget *flight_panel = new FlightStatusPanel(nearest_waypoint);
  wTabBar->AddTab(flight_panel, _T("Flight"), false, FlightIcon);

  system_panel = new SystemStatusPanel();
  wTabBar->AddTab(system_panel, _T("System"), false, SystemIcon);

  Widget *task_panel = new TaskStatusPanel();
  wTabBar->AddTab(task_panel, _T("Task"), false, TaskIcon);

  Widget *rules_panel = new RulesStatusPanel();
  wTabBar->AddTab(rules_panel, _T("Rules"), false, RulesIcon);

  Widget *times_panel = new TimesStatusPanel();
  wTabBar->AddTab(times_panel, _T("Times"), false, TimesIcon);

  /* restore previous page */

  if (start_page != -1) {
    status_page = start_page;
  }

  wTabBar->SetCurrentPage(status_page);

  wf->SetTimerNotify(OnTimerNotify);

  SetTitle();

  wf->ShowModal();

  /* save page number for next time this dialog is opened */
  status_page = wTabBar->GetCurrentPage();

  delete wf;
}
