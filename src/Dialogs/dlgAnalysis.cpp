/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
#include "CrossSection/CrossSectionRenderer.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Computer/Settings.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Look/Look.hpp"
#include "Computer/GlideComputer.hpp"
#include "Protection.hpp"
#include "Util/StringUtil.hpp"
#include "Compiler.h"
#include "Formatter/Units.hpp"
#include "Renderer/FlightStatisticsRenderer.hpp"
#include "Renderer/GlidePolarRenderer.hpp"
#include "Renderer/BarographRenderer.hpp"
#include "Renderer/ClimbChartRenderer.hpp"
#include "Renderer/ThermalBandRenderer.hpp"
#include "Renderer/WindChartRenderer.hpp"
#include "Renderer/CuRenderer.hpp"
#include "UIUtil/GestureManager.hpp"
#include "Blackboard/FullBlackboard.hpp"
#include "Language/Language.hpp"
#include "Engine/Contest/Solvers/Contests.hpp"
#include "Event/LambdaTimer.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#endif

#include <stdio.h>

enum class AnalysisPage: uint8_t {
  BAROGRAPH,
  CLIMB,
  THERMAL_BAND,
  TASK_SPEED,
  WIND,
  POLAR,
  TEMPTRACE,
  TASK,
  OLC,
  AIRSPACE,
  COUNT
};

class ChartControl;

static const Look *look;
static const FullBlackboard *blackboard;
static GlideComputer *glide_computer;
static const ProtectedTaskManager *protected_task_manager;
static const Airspaces *airspaces;
static const RasterTerrain *terrain;

static AnalysisPage page = AnalysisPage::BAROGRAPH;
static WndForm *wf = NULL;
static ChartControl *wGrid = NULL;
static WndFrame *wInfo;
static WndButton *wCalc = NULL;
static GestureManager gestures;

class ChartControl: public PaintWindow
{
  const ChartLook &chart_look;
  const ThermalBandLook &thermal_band_look;
  CrossSectionRenderer cross_section_renderer;

public:
  ChartControl(const ChartLook &_chart_look,
               const ThermalBandLook &_thermal_band_look,
               const CrossSectionLook &cross_section_look,
               const AirspaceLook &airspace_look,
               const Airspaces *airspaces,
               const RasterTerrain *terrain)
    :chart_look(_chart_look),
     thermal_band_look(_thermal_band_look),
     cross_section_renderer(cross_section_look, airspace_look, chart_look) {
    cross_section_renderer.SetAirspaces(airspaces);
    cross_section_renderer.SetTerrain(terrain);
  }

  void UpdateCrossSection();

protected:
  /* virtual methods from class Window */
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys) override;
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y) override;

  /* virtual methods from class PaintWindow */
  virtual void OnPaint(Canvas &canvas) override;
};

static void
SetCalcVisibility(const bool visible)
{
  assert(wCalc != NULL);
  wCalc->SetVisible(visible);
}

static void
SetCalcCaption(const TCHAR* caption)
{
  assert(wCalc != NULL);
  wCalc->SetCaption(caption);
  SetCalcVisibility(!StringIsEmpty(caption));
}

void
ChartControl::OnPaint(Canvas &canvas)
{
  assert(glide_computer != NULL);

  const ComputerSettings &settings_computer = blackboard->GetComputerSettings();
  const MapSettings &settings_map = blackboard->GetMapSettings();
  const MoreData &basic = blackboard->Basic();
  const DerivedInfo &calculated = blackboard->Calculated();

#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLCanvasScissor scissor(canvas);
#endif

  canvas.Clear(COLOR_WHITE);
  canvas.SetTextColor(COLOR_BLACK);

  PixelRect rcgfx = GetClientRect();

  // background is painted in the base-class

  switch (page) {
  case AnalysisPage::BAROGRAPH:
    RenderBarograph(canvas, rcgfx, chart_look, look->cross_section,
                    glide_computer->GetFlightStats(),
                    basic, calculated, protected_task_manager);
    break;
  case AnalysisPage::CLIMB:
    RenderClimbChart(canvas, rcgfx, chart_look,
                     glide_computer->GetFlightStats(),
                     settings_computer.polar.glide_polar_task);
    break;
  case AnalysisPage::THERMAL_BAND:
  {
    OrderedTaskSettings otb;
    if (protected_task_manager != NULL) {
      otb = protected_task_manager->GetOrderedTaskSettings();
    }

    ThermalBandRenderer renderer(thermal_band_look, chart_look);
    renderer.DrawThermalBand(basic,
                             calculated,
                             settings_computer,
                             canvas, rcgfx,
                             settings_computer.task,
                             false,
                             &otb);
  }
    break;
  case AnalysisPage::WIND:
    RenderWindChart(canvas, rcgfx, chart_look,
                    glide_computer->GetFlightStats(),
                    basic, glide_computer->GetWindStore());
    break;
  case AnalysisPage::POLAR:
    RenderGlidePolar(canvas, rcgfx, look->chart,
                     calculated.climb_history,
                     settings_computer,
                     settings_computer.polar.glide_polar_task);
    break;
  case AnalysisPage::TEMPTRACE:
    RenderTemperatureChart(canvas, rcgfx, chart_look,
                           glide_computer->GetCuSonde());
    break;
  case AnalysisPage::TASK:
    if (protected_task_manager != NULL) {
      const TraceComputer *trace_computer = glide_computer != NULL
        ? &glide_computer->GetTraceComputer()
        : NULL;
      const FlightStatisticsRenderer fs(chart_look, look->map);
      fs.RenderTask(canvas, rcgfx, basic,
                    settings_computer, settings_map,
                    *protected_task_manager,
                    trace_computer);
    }
    break;
  case AnalysisPage::OLC:
    if (glide_computer != NULL) {
      const FlightStatisticsRenderer fs(chart_look, look->map);
      fs.RenderOLC(canvas, rcgfx, basic,
                   settings_computer, settings_map,
                   calculated.contest_stats,
                   glide_computer->GetTraceComputer(),
		   glide_computer->GetRetrospective());
    }
    break;
  case AnalysisPage::TASK_SPEED:
    if (protected_task_manager != NULL) {
      ProtectedTaskManager::Lease task(*protected_task_manager);
      RenderSpeed(canvas, rcgfx, chart_look,
                  glide_computer->GetFlightStats(),
                  basic, calculated, task);
    }
    break;

  case AnalysisPage::AIRSPACE:
    cross_section_renderer.Paint(canvas, rcgfx);
    break;

  default:
    // should never get here!
    break;
  }
}

void
ChartControl::UpdateCrossSection()
{
  const MoreData &basic = blackboard->Basic();
  const DerivedInfo &calculated = blackboard->Calculated();

  cross_section_renderer.ReadBlackboard(basic, calculated,
                                        blackboard->GetComputerSettings().task.glide,
                                        blackboard->GetComputerSettings().polar.glide_polar_task,
                                        blackboard->GetMapSettings());

  if (basic.location_available && basic.track_available) {
    cross_section_renderer.SetDirection(basic.track);
    cross_section_renderer.SetStart(basic.location);
  } else
    cross_section_renderer.SetInvalid();
}

static void
Update()
{
  TCHAR sTmp[1000];

  assert(wf != NULL);
  assert(wInfo != NULL);
  assert(wGrid != NULL);
  assert(glide_computer != NULL);

  const ComputerSettings &settings_computer = blackboard->GetComputerSettings();
  const DerivedInfo &calculated = blackboard->Calculated();

  switch (page) {
  case AnalysisPage::BAROGRAPH:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Barograph"));
    wf->SetCaption(sTmp);
    BarographCaption(sTmp, glide_computer->GetFlightStats());
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_("Settings"));
    break;

  case AnalysisPage::CLIMB:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Climb"));
    wf->SetCaption(sTmp);
    ClimbChartCaption(sTmp, glide_computer->GetFlightStats());
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_("Task Calc"));
    break;

  case AnalysisPage::THERMAL_BAND:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Thermal Band"));
    wf->SetCaption(sTmp);
    ClimbChartCaption(sTmp, glide_computer->GetFlightStats());
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_T(""));
    break;

  case AnalysisPage::WIND:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Wind at Altitude"));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(_T(""));
    SetCalcCaption(_("Set Wind"));
    break;

  case AnalysisPage::POLAR:
    StringFormatUnsafe(sTmp, _T("%s: %s (%s %d kg)"), _("Analysis"),
                       _("Glide Polar"), _("Mass"),
                       (int)settings_computer.polar.glide_polar_task.GetTotalMass());
    wf->SetCaption(sTmp);
    GlidePolarCaption(sTmp, settings_computer.polar.glide_polar_task);
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_("Settings"));
    break;

  case AnalysisPage::TEMPTRACE:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Temp Trace"));
    wf->SetCaption(sTmp);
    TemperatureChartCaption(sTmp, glide_computer->GetCuSonde());
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_("Settings"));
    break;

  case AnalysisPage::TASK_SPEED:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Task Speed"));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(_T(""));
    SetCalcCaption(_("Task Calc"));
    break;

  case AnalysisPage::TASK:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Task"));
    wf->SetCaption(sTmp);
    FlightStatisticsRenderer::CaptionTask(sTmp, calculated);
    wInfo->SetCaption(sTmp);
    SetCalcCaption(_("Task calc"));
    break;

  case AnalysisPage::OLC:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       ContestToString(settings_computer.contest.contest));
    wf->SetCaption(sTmp);
    SetCalcCaption(_T(""));
    FlightStatisticsRenderer::CaptionOLC(sTmp, settings_computer.contest,
                                         calculated);
    wInfo->SetCaption(sTmp);
    break;

  case AnalysisPage::AIRSPACE:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Airspace"));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(_T(""));
    SetCalcCaption(_("Warnings"));
    break;

  case AnalysisPage::COUNT:
    gcc_unreachable();
  }

  if (page == AnalysisPage::AIRSPACE)
    wGrid->UpdateCrossSection();

  wGrid->Invalidate();
}

static void
NextPage(int Step)
{
  int new_page = (int)page + Step;

  if (new_page >= (int)AnalysisPage::COUNT)
    new_page = 0;
  if (new_page < 0)
    new_page = (int)AnalysisPage::COUNT - 1;
  page = (AnalysisPage)new_page;

  Update();
}

static void
OnGesture(const TCHAR* gesture)
{
  if (_tcscmp(gesture, _T("R")) == 0)
    NextPage(-1);
  else if (_tcscmp(gesture, _T("L")) == 0)
    NextPage(+1);
}

bool
ChartControl::OnMouseDown(PixelScalar x, PixelScalar y)
{
  gestures.Start(x, y, Layout::Scale(20));
  return true;
}

bool
ChartControl::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  gestures.Update(x, y);
  return true;
}

bool
ChartControl::OnMouseUp(PixelScalar x, PixelScalar y)
{
  const TCHAR* gesture = gestures.Finish();
  if (gesture != NULL)
    OnGesture(gesture);

  return true;
}

static void
OnNextClicked()
{
  NextPage(+1);
}

static void
OnPrevClicked()
{
  NextPage(-1);
}

static bool
FormKeyDown(unsigned key_code)
{
  assert(wf != NULL);

  switch (key_code) {
  case KEY_LEFT:
#ifdef GNAV
  case '6':
    // Key F14 added in order to control the Analysis-Pages with the Altair RemoteStick
  case KEY_F14:
#endif
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->SetFocus();
    NextPage(-1);
    return true;

  case KEY_RIGHT:
#ifdef GNAV
  case '7':
    // Key F13 added in order to control the Analysis-Pages with the Altair RemoteStick
  case KEY_F13:
#endif
    ((WndButton *)wf->FindByName(_T("cmdNext")))->SetFocus();
    NextPage(+1);
    return true;

  default:
    return false;
  }
}

static void
OnCalcClicked()
{
  assert(wf != NULL);

  switch (page) {
  case AnalysisPage::BAROGRAPH:
    dlgBasicSettingsShowModal();
    break;

  case AnalysisPage::CLIMB:
  case AnalysisPage::TASK:
  case AnalysisPage::TASK_SPEED:
    dlgTaskManagerShowModal();
    break;

  case AnalysisPage::WIND:
    ShowWindSettingsDialog();
    break;

  case AnalysisPage::POLAR:
    dlgBasicSettingsShowModal();
    break;

  case AnalysisPage::TEMPTRACE:
    dlgBasicSettingsShowModal();
    break;

  case AnalysisPage::AIRSPACE:
    dlgAirspaceWarningsShowModal(wf->GetMainWindow(),
                                 glide_computer->GetAirspaceWarnings());
    break;

  case AnalysisPage::THERMAL_BAND:
  case AnalysisPage::OLC:
  case AnalysisPage::COUNT:
    break;
  }

  Update();
}

static Window *
OnCreateChartControl(ContainerWindow &parent, PixelRect rc,
                     const WindowStyle style)
{
  auto *w = new ChartControl(look->chart,
                             look->thermal_band,
                             look->cross_section, look->map.airspace,
                             airspaces, terrain);
  w->Create(parent, rc, style);
  return w;
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCreateChartControl),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnCalcClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgAnalysisShowModal(SingleWindow &parent, const Look &_look,
                     const FullBlackboard &_blackboard,
                     GlideComputer &_glide_computer,
                     const ProtectedTaskManager *_protected_task_manager,
                     const Airspaces *_airspaces,
                     const RasterTerrain *_terrain,
                     int _page)
{
  look = &_look;
  blackboard = &_blackboard;
  glide_computer = &_glide_computer;
  protected_task_manager = _protected_task_manager;
  airspaces = _airspaces;
  terrain = _terrain;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_ANALYSIS_L") :
                                      _T("IDR_XML_ANALYSIS"));
  assert(wf != NULL);

  wf->SetKeyDownFunction(FormKeyDown);

  wGrid = (ChartControl*)wf->FindByName(_T("frmGrid"));
  wInfo = (WndFrame *)wf->FindByName(_T("frmInfo"));
  wCalc = (WndButton *)wf->FindByName(_T("cmdCalc"));

  if (_page >= 0)
    page = (AnalysisPage)_page;

  Update();

  auto update_timer = MakeLambdaTimer([](){ Update(); });
  update_timer.Schedule(2500);

  wf->ShowModal();
  update_timer.Cancel();

  delete wf;
}
