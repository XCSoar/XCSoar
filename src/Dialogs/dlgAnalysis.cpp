/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "TaskClientUI.hpp"
#include "SettingsComputer.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "GlideComputer.hpp"
#include "Atmosphere.h"
#include "Blackboard.hpp"
#include "Components.hpp"
#include "Protection.hpp"
#include "StringUtil.hpp"
#include "Compiler.h"

#define MAXPAGE 8

static int page = 0;
static WndForm *wf = NULL;
static WndOwnerDrawFrame *wGrid = NULL;
static WndFrame *wInfo;
static WndButton *wCalc = NULL;

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

  wCalc->SetCaption(gettext(caption));
  SetCalcVisibility(!string_is_empty(caption));
}

#define ANALYSIS_PAGE_BAROGRAPH    0
#define ANALYSIS_PAGE_CLIMB        1
#define ANALYSIS_PAGE_TASK_SPEED   2
#define ANALYSIS_PAGE_WIND         3
#define ANALYSIS_PAGE_POLAR        4
#define ANALYSIS_PAGE_TEMPTRACE    5
#define ANALYSIS_PAGE_TASK         6
#define ANALYSIS_PAGE_OLC          7
#define ANALYSIS_PAGE_AIRSPACE     8

static void
OnAnalysisPaint(WindowControl *Sender, Canvas &canvas)
{
  RECT rcgfx = Sender->get_client_rect();

  // background is painted in the base-class

  const FlightStatistics &fs = glide_computer.GetFlightStats();
  const TracePointVector trace = task_ui.get_trace_points();
  const TracePointVector olc = task_ui.get_olc_points();
  const GlidePolar glide_polar = task_ui.get_glide_polar();

  switch (page) {
  case ANALYSIS_PAGE_BAROGRAPH:
    fs.RenderBarograph(canvas, rcgfx, XCSoarInterface::Basic(), 
                       XCSoarInterface::Calculated(), task_ui);
    break;
  case ANALYSIS_PAGE_CLIMB:
    fs.RenderClimb(canvas, rcgfx, glide_polar);
    break;
  case ANALYSIS_PAGE_WIND:
    fs.RenderWind(canvas, rcgfx, XCSoarInterface::Basic(),
                  glide_computer.windanalyser.windstore);
    break;
  case ANALYSIS_PAGE_POLAR:
    fs.RenderGlidePolar(canvas, rcgfx, XCSoarInterface::Calculated(),
                        XCSoarInterface::SettingsComputer(),
                        glide_polar);
    break;
  case ANALYSIS_PAGE_TEMPTRACE:
    fs.RenderTemperature(canvas, rcgfx);
    break;
  case ANALYSIS_PAGE_TASK:
    fs.RenderTask(canvas, rcgfx, XCSoarInterface::Basic(),
                  XCSoarInterface::SettingsComputer(),
                  XCSoarInterface::SettingsMap(),
                  task_ui,
                  trace);
    break;
  case ANALYSIS_PAGE_OLC:
    fs.RenderOLC(canvas, rcgfx, XCSoarInterface::Basic(),
                 XCSoarInterface::SettingsComputer(),
                 XCSoarInterface::SettingsMap(),
                 olc, trace);
    break;
  case ANALYSIS_PAGE_AIRSPACE:
    fs.RenderAirspace(canvas, rcgfx, XCSoarInterface::Basic(),
                      XCSoarInterface::Calculated(),
                      XCSoarInterface::SettingsMap(),
                      airspace_ui, terrain);
    break;
  case ANALYSIS_PAGE_TASK_SPEED:
    fs.RenderSpeed(canvas, rcgfx, XCSoarInterface::Basic(), 
                   XCSoarInterface::Calculated(), task_ui);
    break;
  default:
    // should never get here!
    break;
  }
}

static void
Update(void)
{
  TCHAR sTmp[1000];

  FlightStatistics &fs = glide_computer.GetFlightStats();
  GlidePolar polar = task_ui.get_glide_polar();
  const CommonStats& stats = XCSoarInterface::Calculated().common_stats;

  switch (page) {
  case ANALYSIS_PAGE_BAROGRAPH:
    _stprintf(sTmp, _T("%s: %s"), gettext(_T("Analysis")),
              gettext(_T("Barograph")));
    wf->SetCaption(sTmp);
    fs.CaptionBarograph(sTmp);
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_T("Settings"));
    break;

  case ANALYSIS_PAGE_CLIMB:
    _stprintf(sTmp, _T("%s: %s"), gettext(_T("Analysis")),
              gettext(_T("Climb")));
    wf->SetCaption(sTmp);
    fs.CaptionClimb(sTmp);
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_T("Task calc"));
    break;

  case ANALYSIS_PAGE_WIND:
    _stprintf(sTmp, _T("%s: %s"), gettext(_T("Analysis")),
              gettext(_T("Wind at Altitude")));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(_T(""));
    SetCalcCaption(_T("Set wind"));
    break;

  case ANALYSIS_PAGE_POLAR:
    _stprintf(sTmp, _T("%s: %s (Mass %d kg)"), gettext(_T("Analysis")),
              gettext(_T("Glide Polar")),
              (int)polar.get_all_up_weight());
    wf->SetCaption(sTmp);
    fs.CaptionPolar(sTmp, polar);
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_T("Settings"));
   break;

  case ANALYSIS_PAGE_TEMPTRACE:
    _stprintf(sTmp, _T("%s: %s"), gettext(_T("Analysis")),
              gettext(_T("Temp trace")));
    wf->SetCaption(sTmp);
    fs.CaptionTempTrace(sTmp);
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_T("Settings"));
    break;

  case ANALYSIS_PAGE_TASK_SPEED:
    _stprintf(sTmp, _T("%s: %s"), gettext(_T("Analysis")),
              gettext(_T("Task speed")));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(_T(""));
    SetCalcCaption(_T("Task calc"));
    break;

  case ANALYSIS_PAGE_TASK:
    _stprintf(sTmp, _T("%s: %s"), gettext(_T("Analysis")),
              gettext(_T("Task")));
    wf->SetCaption(sTmp);
    fs.CaptionTask(sTmp, XCSoarInterface::Calculated());
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_T("Task calc"));
    break;

  case ANALYSIS_PAGE_OLC:
    _stprintf(sTmp, _T("%s: %s"), gettext(_T("Analysis")),
              gettext(_T("OnLine Contest")));
    wf->SetCaption(sTmp);

    TCHAR timetext1[100];
    Units::TimeToText(timetext1, (int)stats.time_olc);
    _stprintf(sTmp,
              (Layout::landscape
              ? _T("%s:\r\n  %d %s\r\n%s: %s\r\n%s: %d %s\r\n")
              : _T("%s: %d %s\r\n%s: %s\r\n%s: %d %s\r\n")),
              gettext(_T("Distance")),
              (int)Units::ToUserUnit(stats.distance_olc, Units::DistanceUnit),
              Units::GetDistanceName(),
              gettext(_T("Time")),
              timetext1,
              gettext(_T("Speed")),
              (int)Units::ToUserUnit(stats.speed_olc, Units::TaskSpeedUnit),
              Units::GetTaskSpeedName());
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_T(""));
    break;

  case ANALYSIS_PAGE_AIRSPACE:
    _stprintf(sTmp, _T("%s: %s"), gettext(_T("Analysis")),
              gettext(_T("Airspace")));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(_T(""));
    SetCalcCaption(_T("Warnings"));
    break;
  }

  wGrid->set_visible(page < MAXPAGE + 1);

  if (wGrid != NULL)
    wGrid->invalidate();
}

static void
NextPage(int Step)
{
  page += Step;

  if (page > MAXPAGE)
    page = 0;
  if (page < 0)
    page = MAXPAGE;

  Update();
}

static void
OnNextClicked(WindowControl * Sender)
{
  (void)Sender;
  NextPage(+1);
}

static void
OnPrevClicked(WindowControl * Sender)
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
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  (void)Sender;

  switch (key_code) {
  case VK_LEFT:
  case '6':
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    NextPage(-1);
    return true;

  case VK_RIGHT:
  case '7':
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    NextPage(+1);
    return true;

  default:
    return false;
  }
}

static void
OnCalcClicked(WindowControl *Sender)
{
  (void)Sender;
  if (page == ANALYSIS_PAGE_BAROGRAPH)
    dlgBasicSettingsShowModal();

  if (page == ANALYSIS_PAGE_CLIMB) {
    wf->hide();
    dlgTaskCalculatorShowModal(XCSoarInterface::main_window);
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
    dlgTaskCalculatorShowModal(XCSoarInterface::main_window);
    wf->show();
  }

  if (page == ANALYSIS_PAGE_AIRSPACE)
    airspaceWarningEvent.trigger();

  Update();
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnAnalysisPaint),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnCalcClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgAnalysisShowModal(void)
{
  wf = NULL;
  wGrid = NULL;
  wInfo = NULL;
  wCalc = NULL;

  if (!Layout::landscape)
    wf = dlgLoadFromXML(CallBackTable, _T("dlgAnalysis_L.xml"),
                        XCSoarInterface::main_window, _T("IDR_XML_ANALYSIS_L"));
  else
    wf = dlgLoadFromXML(CallBackTable, _T("dlgAnalysis.xml"),
                        XCSoarInterface::main_window, _T("IDR_XML_ANALYSIS"));

  if (!wf)
    return;

  wf->SetKeyDownNotify(FormKeyDown);

  wGrid = (WndOwnerDrawFrame*)wf->FindByName(_T("frmGrid"));
  wInfo = (WndFrame *)wf->FindByName(_T("frmInfo"));
  wCalc = (WndButton *)wf->FindByName(_T("cmdCalc"));

  ((WndButton *)wf->FindByName(_T("cmdClose")))->
      SetOnClickNotify(OnCloseClicked);

  Update();

  wf->ShowModal();

  delete wf;
}
