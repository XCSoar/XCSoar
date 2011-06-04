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

#include "Dialogs/dlgAnalysis.hpp"
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
#include "Protection.hpp"
#include "StringUtil.hpp"
#include "Compiler.h"
#include "Units/UnitsFormatter.hpp"
#include "FlightStatisticsRenderer.hpp"
#include "ThermalBandRenderer.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/Globals.hpp"
#endif

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

class ChartControl;

static const GlideComputer *glide_computer;
static const ProtectedTaskManager *protected_task_manager;
static const Airspaces *airspaces;
static const RasterTerrain *terrain;

static enum analysis_page page = ANALYSIS_PAGE_BAROGRAPH;
static WndForm *wf = NULL;
static ChartControl *wGrid = NULL;
static WndFrame *wInfo;
static WndButton *wCalc = NULL;
static CrossSectionWindow *csw = NULL;
static GestureManager gestures;

class CrossSectionControl: public CrossSectionWindow
{
public:
  CrossSectionControl(const CrossSectionLook &look)
    :CrossSectionWindow(look) {}

protected:
  virtual bool on_mouse_move(int x, int y, unsigned keys);
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);
};

class ChartControl: public PaintWindow
{
public:
  ChartControl(ContainerWindow &parent, int X, int Y, int Width, int Height,
               const WindowStyle style);

protected:
  virtual bool on_mouse_move(int x, int y, unsigned keys);
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);

  virtual void on_paint(Canvas &canvas);
};

ChartControl::ChartControl(ContainerWindow &parent, int X, int Y,
                           int Width, int Height, const WindowStyle style)
  :PaintWindow()
{
  set(parent, X, Y, Width, Height, style);
}

static void
SetCalcVisibility(const bool visible)
{
  assert(wCalc != NULL);
  wCalc->set_visible(visible);
}

static void
SetCalcCaption(const TCHAR* caption)
{
  assert(wCalc != NULL);
  wCalc->SetCaption(caption);
  SetCalcVisibility(!string_is_empty(caption));
}

void
ChartControl::on_paint(Canvas &canvas)
{
  assert(glide_computer != NULL);

#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLScissor scissor(OpenGL::translate_x,
                    OpenGL::screen_height - OpenGL::translate_y - canvas.get_height() - 1,
                    canvas.get_width(), canvas.get_height());
#endif

  canvas.clear(COLOR_WHITE);
  canvas.set_text_color(COLOR_BLACK);
  canvas.select(Fonts::Map);

  PixelRect rcgfx = get_client_rect();

  // background is painted in the base-class

  const FlightStatisticsRenderer fs(glide_computer->GetFlightStats());

  switch (page) {
  case ANALYSIS_PAGE_BAROGRAPH:
    fs.RenderBarograph(canvas, rcgfx, XCSoarInterface::Basic(), 
                       XCSoarInterface::Calculated(), protected_task_manager);
    break;
  case ANALYSIS_PAGE_CLIMB:
    fs.RenderClimb(canvas, rcgfx, CommonInterface::SettingsComputer().glide_polar_task);
    break;
  case ANALYSIS_PAGE_THERMAL_BAND:
  {
    OrderedTaskBehaviour otb;
    if (protected_task_manager != NULL) {
      otb = protected_task_manager->get_ordered_task_behaviour();
    }

    ThermalBandRenderer renderer(Graphics::thermal_band, Graphics::chart);
    renderer.DrawThermalBand(XCSoarInterface::Basic(),
                             XCSoarInterface::Calculated(),
                             CommonInterface::SettingsComputer(),
                             canvas, rcgfx,
                             XCSoarInterface::SettingsComputer(),
                             false,
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
                        CommonInterface::SettingsComputer().glide_polar_task);
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
                   XCSoarInterface::SettingsMap(),
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
  assert(csw != NULL);
  csw->ReadBlackboard(XCSoarInterface::Basic(), XCSoarInterface::Calculated(),
                      XCSoarInterface::SettingsMap());
  csw->set_direction(XCSoarInterface::Basic().track);
  csw->set_start(XCSoarInterface::Basic().Location);
}

static void
Update(void)
{
  TCHAR sTmp[1000];

  assert(wf != NULL);
  assert(wInfo != NULL);
  assert(wGrid != NULL);
  assert(csw != NULL);
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
              (int)CommonInterface::SettingsComputer().glide_polar_task.get_all_up_weight());
    wf->SetCaption(sTmp);
    fs.CaptionPolar(sTmp, CommonInterface::SettingsComputer().glide_polar_task);
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
OnGesture(const TCHAR* gesture)
{
  if (_tcscmp(gesture, _T("L")) == 0)
    NextPage(-1);
  else if (_tcscmp(gesture, _T("R")) == 0)
    NextPage(+1);
}

bool
ChartControl::on_mouse_down(int x, int y)
{
  gestures.Start(x, y, Layout::Scale(20));
  return true;
}

bool
ChartControl::on_mouse_move(int x, int y, unsigned keys)
{
  gestures.Update(x, y);
  return true;
}

bool
ChartControl::on_mouse_up(int x, int y)
{
  const TCHAR* gesture = gestures.Finish();
  if (gesture != NULL)
    OnGesture(gesture);

  return true;
}

bool
CrossSectionControl::on_mouse_down(int x, int y)
{
  gestures.Start(x, y, Layout::Scale(20));
  return true;
}

bool
CrossSectionControl::on_mouse_move(int x, int y, unsigned keys)
{
  gestures.Update(x, y);
  return true;
}

bool
CrossSectionControl::on_mouse_up(int x, int y)
{
  const TCHAR* gesture = gestures.Finish();
  if (gesture != NULL)
    OnGesture(gesture);

  return true;
}

static void
OnNextClicked(gcc_unused WndButton &Sender)
{
  NextPage(+1);
}

static void
OnPrevClicked(gcc_unused WndButton &Sender)
{
  NextPage(-1);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static bool
FormKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  assert(wf != NULL);

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
OnCalcClicked(gcc_unused WndButton &Sender)
{
  assert(wf != NULL);

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
OnCreateCrossSectionControl(ContainerWindow &parent, int left, int top,
                            unsigned width, unsigned height,
                            const WindowStyle style)
{
  csw = new CrossSectionControl(Graphics::cross_section);
  csw->set(parent, left, top, width, height, style);
  csw->set_airspaces(airspaces);
  csw->set_terrain(terrain);
  UpdateCrossSection();
  return csw;
}

static Window *
OnCreateChartControl(ContainerWindow &parent, int left, int top,
                     unsigned width, unsigned height,
                     const WindowStyle style)
{
  return new ChartControl(parent, left, top, width, height, style);
}

static void
OnTimer(WndForm &Sender)
{
  Update();
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCreateCrossSectionControl),
  DeclareCallBackEntry(OnCreateChartControl),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnCalcClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgAnalysisShowModal(SingleWindow &parent,
                     const GlideComputer &_glide_computer,
                     const ProtectedTaskManager *_protected_task_manager,
                     const Airspaces *_airspaces,
                     const RasterTerrain *_terrain,
                     int _page)
{
  glide_computer = &_glide_computer;
  protected_task_manager = _protected_task_manager;
  airspaces = _airspaces;
  terrain = _terrain;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_ANALYSIS_L") :
                                      _T("IDR_XML_ANALYSIS"));
  assert(wf != NULL);

  wf->SetKeyDownNotify(FormKeyDown);

  wGrid = (ChartControl*)wf->FindByName(_T("frmGrid"));
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
