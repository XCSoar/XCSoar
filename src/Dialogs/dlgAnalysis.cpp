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

#include "Dialogs/Internal.hpp"
#include "CrossSection/CrossSectionWindow.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "SettingsComputer.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "MainWindow.hpp"
#include "GlideComputer.hpp"
#include "Blackboard.hpp"
#include "Components.hpp"
#include "Protection.hpp"
#include "StringUtil.hpp"
#include "Compiler.h"
#include "UnitsFormatter.hpp"
#include "FlightStatisticsRenderer.hpp"
#include "ThermalBandRenderer.hpp"

#include <stdio.h>

enum analysis_page {
  ANALYSIS_PAGE_BAROGRAPH,
  ANALYSIS_PAGE_CLIMB,
  ANALYSIS_PAGE_THERMAL_BAND,
  ANALYSIS_PAGE_TASK_SPEED,
  ANALYSIS_PAGE_WIND,
  ANALYSIS_PAGE_POLAR,
  ANALYSIS_PAGE_TEMPTRACE,
  ANALYSIS_PAGE_TASK,
  ANALYSIS_PAGE_OLC,
  ANALYSIS_PAGE_AIRSPACE,
  ANALYSIS_PAGE_COUNT
};

static enum analysis_page page = ANALYSIS_PAGE_BAROGRAPH;
static WndForm *wf = NULL;
static WndOwnerDrawFrame *wGrid = NULL;
static WndFrame *wInfo;
static WndButton *wCalc = NULL;
static CrossSectionWindow *csw = NULL;

static void
SetCalcVisibility(const bool visible)
{
  if (!wCalc)
    return;

  wCalc->set_visible(visible);
}

static void
SetCalcCaption(const TCHAR* caption)
{
  if (!wCalc)
    return;

  wCalc->SetCaption(caption);
  SetCalcVisibility(!string_is_empty(caption));
}

static void
OnAnalysisPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  assert(glide_computer != NULL);

  canvas.clear(Color::WHITE);
  canvas.set_text_color(Color::BLACK);
  canvas.select(Fonts::Map);

  PixelRect rcgfx = Sender->get_client_rect();

  // background is painted in the base-class

  const FlightStatisticsRenderer fs(glide_computer->GetFlightStats());

  switch (page) {
  case ANALYSIS_PAGE_BAROGRAPH:
    fs.RenderBarograph(canvas, rcgfx, XCSoarInterface::Basic(), 
                       XCSoarInterface::Calculated(), protected_task_manager);
    break;
  case ANALYSIS_PAGE_CLIMB:
    fs.RenderClimb(canvas, rcgfx, XCSoarInterface::Calculated().glide_polar_task);
    break;
  case ANALYSIS_PAGE_THERMAL_BAND:
  {
    OrderedTaskBehaviour otb;
    if (protected_task_manager != NULL) {
      otb = protected_task_manager->get_ordered_task_behaviour();
    }
    ThermalBandRenderer::DrawThermalBand(XCSoarInterface::Basic(),
                                         XCSoarInterface::Calculated(),
                                         canvas, rcgfx, 
                                         XCSoarInterface::SettingsComputer(),
                                         &otb);
  }
    break;
  case ANALYSIS_PAGE_WIND:
    fs.RenderWind(canvas, rcgfx, XCSoarInterface::Basic(),
                  glide_computer->windanalyser.windstore);
    break;
  case ANALYSIS_PAGE_POLAR:
    fs.RenderGlidePolar(canvas, rcgfx, XCSoarInterface::Calculated(),
                        XCSoarInterface::SettingsComputer(),
                        XCSoarInterface::Calculated().glide_polar_task);
    break;
  case ANALYSIS_PAGE_TEMPTRACE:
    fs.RenderTemperature(canvas, rcgfx);
    break;
  case ANALYSIS_PAGE_TASK:
    if (protected_task_manager != NULL) {
      ProtectedTaskManager::Lease task(*protected_task_manager);
      fs.RenderTask(canvas, rcgfx, XCSoarInterface::Basic(),
                    CommonInterface::Calculated(),
                    XCSoarInterface::SettingsComputer(),
                    XCSoarInterface::SettingsMap(),
                    task);
    }
    break;
  case ANALYSIS_PAGE_OLC:
    if (protected_task_manager != NULL) {
      TracePointVector trace;
      {
        ProtectedTaskManager::Lease task(*protected_task_manager);
        task->get_trace_points(trace);
      }

      fs.RenderOLC(canvas, rcgfx, XCSoarInterface::Basic(),
                   CommonInterface::Calculated(),
                   XCSoarInterface::SettingsComputer(),
                   XCSoarInterface::Calculated().contest_stats, trace);
    }
    break;
  case ANALYSIS_PAGE_TASK_SPEED:
    if (protected_task_manager != NULL) {
      ProtectedTaskManager::Lease task(*protected_task_manager);
      fs.RenderSpeed(canvas, rcgfx, XCSoarInterface::Basic(),
                     XCSoarInterface::Calculated(), task);
    }
    break;
  default:
    // should never get here!
    break;
  }
}

static void
UpdateCrossSection()
{
  csw->ReadBlackboard(XCSoarInterface::Basic(), XCSoarInterface::Calculated(),
                      XCSoarInterface::SettingsMap());
  csw->set_direction(XCSoarInterface::Basic().TrackBearing);
  csw->set_start(XCSoarInterface::Basic().Location);
}

static void
Update(void)
{
  TCHAR sTmp[1000];

  assert(glide_computer != NULL);

  FlightStatisticsRenderer fs(glide_computer->GetFlightStats());

  switch (page) {
  case ANALYSIS_PAGE_BAROGRAPH:
    _stprintf(sTmp, _T("%s: %s"), _("Analysis"),
              _("Barograph"));
    wf->SetCaption(sTmp);
    fs.CaptionBarograph(sTmp);
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_("Settings"));
    break;

  case ANALYSIS_PAGE_CLIMB:
    _stprintf(sTmp, _T("%s: %s"), _("Analysis"),
              _("Climb"));
    wf->SetCaption(sTmp);
    fs.CaptionClimb(sTmp);
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_("Task Calc"));
    break;

  case ANALYSIS_PAGE_THERMAL_BAND:
    _stprintf(sTmp, _T("%s: %s"), _("Analysis"),
              _("Thermal Band"));
    wf->SetCaption(sTmp);
    fs.CaptionClimb(sTmp);
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_T(""));
    break;

  case ANALYSIS_PAGE_WIND:
    _stprintf(sTmp, _T("%s: %s"), _("Analysis"),
              _("Wind At Altitude"));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(_T(""));
    SetCalcCaption(_("Set Wind"));
    break;

  case ANALYSIS_PAGE_POLAR:
    _stprintf(sTmp, _T("%s: %s (%s %d kg)"), _("Analysis"),
              _("Glide Polar"), _("Mass"),
              (int)XCSoarInterface::Calculated().glide_polar_task.get_all_up_weight());
    wf->SetCaption(sTmp);
    fs.CaptionPolar(sTmp, XCSoarInterface::Calculated().glide_polar_task);
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_("Settings"));
   break;

  case ANALYSIS_PAGE_TEMPTRACE:
    _stprintf(sTmp, _T("%s: %s"), _("Analysis"),
              _("Temp Trace"));
    wf->SetCaption(sTmp);
    fs.CaptionTempTrace(sTmp);
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_("Settings"));
    break;

  case ANALYSIS_PAGE_TASK_SPEED:
    _stprintf(sTmp, _T("%s: %s"), _("Analysis"),
              _("Task Speed"));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(_T(""));
    SetCalcCaption(_("Task Calc"));
    break;

  case ANALYSIS_PAGE_TASK:
    _stprintf(sTmp, _T("%s: %s"), _("Analysis"),
              _("Task"));
    wf->SetCaption(sTmp);
    fs.CaptionTask(sTmp, XCSoarInterface::Calculated());
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_("Task calc"));
    break;

  case ANALYSIS_PAGE_OLC:
    _stprintf(sTmp, _T("%s: %s"), _("Analysis"),
        ContestToString(XCSoarInterface::SettingsComputer().contest));
    wf->SetCaption(sTmp);
    SetCalcCaption(_T(""));
    fs.CaptionOLC(sTmp, XCSoarInterface::SettingsComputer(),
                  XCSoarInterface::Calculated());
    wInfo->SetCaption(sTmp);
    break;

  case ANALYSIS_PAGE_AIRSPACE:
    _stprintf(sTmp, _T("%s: %s"), _("Analysis"),
              _("Airspace"));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(_T(""));
    SetCalcCaption(_("Warnings"));
    break;

  case ANALYSIS_PAGE_COUNT:
    assert(false);
    break;
  }

  switch (page) {
  case ANALYSIS_PAGE_AIRSPACE:
    UpdateCrossSection();
    csw->invalidate();
    csw->show();
    wGrid->hide();
    break;

  default:
    csw->hide();
    wGrid->show();
    wGrid->invalidate();
    break;
  }
}

static void
NextPage(int Step)
{
  int new_page = (int)page + Step;

  if (new_page >= ANALYSIS_PAGE_COUNT)
    new_page = 0;
  if (new_page < 0)
    new_page = ANALYSIS_PAGE_COUNT - 1;
  page = (enum analysis_page)new_page;

  Update();
}

static void
OnNextClicked(WndButton &Sender)
{
  (void)Sender;
  NextPage(+1);
}

static void
OnPrevClicked(WndButton &Sender)
{
  (void)Sender;
  NextPage(-1);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static bool
FormKeyDown(WndForm &Sender, unsigned key_code)
{
  (void)Sender;

  switch (key_code) {
  case VK_LEFT:
#ifdef GNAV
  case '6':
#endif
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    NextPage(-1);
    return true;

  case VK_RIGHT:
#ifdef GNAV
  case '7':
#endif
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    NextPage(+1);
    return true;

  default:
    return false;
  }
}

static void
OnCalcClicked(WndButton &Sender)
{
  (void)Sender;
  if (page == ANALYSIS_PAGE_BAROGRAPH)
    dlgBasicSettingsShowModal();

  if (page == ANALYSIS_PAGE_CLIMB) {
    wf->hide();
    dlgTaskManagerShowModal(*(SingleWindow *)wf->get_root_owner());
    wf->show();
  }

  if (page == ANALYSIS_PAGE_WIND)
    dlgWindSettingsShowModal();

  if (page == ANALYSIS_PAGE_POLAR)
    dlgBasicSettingsShowModal();

  if (page == ANALYSIS_PAGE_TEMPTRACE)
    dlgBasicSettingsShowModal();

  if ((page == ANALYSIS_PAGE_TASK) || (page == ANALYSIS_PAGE_TASK_SPEED)) {
    wf->hide();
    dlgTaskManagerShowModal(*(SingleWindow *)wf->get_root_owner());
    wf->show();
  }

  if (page == ANALYSIS_PAGE_AIRSPACE)
    dlgAirspaceWarningsShowModal(wf->GetMainWindow());

  Update();
}

static Window *
OnCreateCrossSectionWindow(ContainerWindow &parent, int left, int top,
                           unsigned width, unsigned height,
                           const WindowStyle style)
{
  csw = new CrossSectionWindow();
  csw->set(parent, left, top, width, height, style);
  csw->set_airspaces(&airspace_database);
  csw->set_terrain(terrain);
  csw->set_background_color(Color::WHITE);
  csw->set_text_color(Color::BLACK);
  UpdateCrossSection();
  return csw;
}

static void
OnTimer(WndForm &Sender)
{
  Update();
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCreateCrossSectionWindow),
  DeclareCallBackEntry(OnAnalysisPaint),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnCalcClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgAnalysisShowModal(SingleWindow &parent, int _page)
{
  wf = NULL;
  wGrid = NULL;
  wInfo = NULL;
  wCalc = NULL;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_ANALYSIS_L") :
                                      _T("IDR_XML_ANALYSIS"));
  if (!wf)
    return;

  wf->SetKeyDownNotify(FormKeyDown);

  wGrid = (WndOwnerDrawFrame*)wf->FindByName(_T("frmGrid"));
  wInfo = (WndFrame *)wf->FindByName(_T("frmInfo"));
  wCalc = (WndButton *)wf->FindByName(_T("cmdCalc"));

  ((WndButton *)wf->FindByName(_T("cmdClose")))->
      SetOnClickNotify(OnCloseClicked);

  if (_page >= 0)
    page = (analysis_page)_page;

  Update();

  wf->SetTimerNotify(OnTimer, 2500);
  wf->ShowModal();

  delete wf;
}
