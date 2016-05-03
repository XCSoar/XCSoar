/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Dialogs/Dialogs.h"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/Widget.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
#include "CrossSection/CrossSectionRenderer.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Computer/Settings.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Event/KeyCode.hpp"
#include "Look/Look.hpp"
#include "Computer/GlideComputer.hpp"
#include "Renderer/FlightStatisticsRenderer.hpp"
#include "Renderer/GlidePolarRenderer.hpp"
#include "Renderer/BarographRenderer.hpp"
#include "Renderer/ClimbChartRenderer.hpp"
#include "Renderer/ThermalBandRenderer.hpp"
#include "Renderer/WindChartRenderer.hpp"
#include "Renderer/CuRenderer.hpp"
#include "Renderer/MacCreadyRenderer.hpp"
#include "Renderer/VarioHistogramRenderer.hpp"
#include "Renderer/TaskSpeedRenderer.hpp"
#include "UIUtil/GestureManager.hpp"
#include "Blackboard/FullBlackboard.hpp"
#include "Language/Language.hpp"
#include "Engine/Contest/Solvers/Contests.hpp"
#include "Event/Timer.hpp"
#include "Util/StringCompare.hxx"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#endif

#include <stdio.h>

static AnalysisPage page = AnalysisPage::BAROGRAPH;

class AnalysisWidget;

class ChartControl: public PaintWindow
{
  AnalysisWidget &analysis_widget;

  const ChartLook &chart_look;
  const CrossSectionLook &cross_section_look;
  ThermalBandRenderer thermal_band_renderer;
  FlightStatisticsRenderer fs_renderer;
  CrossSectionRenderer cross_section_renderer;
  GestureManager gestures;
  bool dragging;

  const FullBlackboard &blackboard;
  const GlideComputer &glide_computer;

public:
  ChartControl(AnalysisWidget &_analysis_widget,
               const ChartLook &_chart_look,
               const MapLook &map_look,
               const CrossSectionLook &_cross_section_look,
               const ThermalBandLook &_thermal_band_look,
               const CrossSectionLook &cross_section_look,
               const AirspaceLook &airspace_look,
               const Airspaces *airspaces,
               const RasterTerrain *terrain,
               const FullBlackboard &_blackboard,
               const GlideComputer &_glide_computer)
    :analysis_widget(_analysis_widget),
     chart_look(_chart_look),
     cross_section_look(_cross_section_look),
     thermal_band_renderer(_thermal_band_look, chart_look),
     fs_renderer(chart_look, map_look),
     cross_section_renderer(cross_section_look, airspace_look, chart_look, false),
     dragging(false),
     blackboard(_blackboard), glide_computer(_glide_computer) {
    cross_section_renderer.SetAirspaces(airspaces);
    cross_section_renderer.SetTerrain(terrain);
  }

  void UpdateCrossSection(const MoreData &basic,
                          const DerivedInfo &calculated,
                          const GlideSettings &glide_settings,
                          const GlidePolar &glide_polar,
                          const MapSettings &map_settings);

protected:
  /* virtual methods from class Window */
  bool OnMouseMove(PixelPoint p, unsigned keys) override;
  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseUp(PixelPoint p) override;

  void OnCancelMode() override {
    PaintWindow::OnCancelMode();
    dragging = false;
  }

  /* virtual methods from class PaintWindow */
  virtual void OnPaint(Canvas &canvas) override;
};

class AnalysisWidget final : public NullWidget, ActionListener, Timer {
  enum Buttons {
    PREVIOUS,
    NEXT,
    DETAILS,
  };

  struct Layout {
    PixelRect info;
    PixelRect details_button, previous_button, next_button, close_button;
    PixelRect main;

    explicit Layout(const PixelRect rc);
  };

  const FullBlackboard &blackboard;
  GlideComputer &glide_computer;

  WndForm &dialog;

  WndFrame info;
  Button details_button, previous_button, next_button, close_button;
  ChartControl chart;

public:
  AnalysisWidget(WndForm &_dialog, const Look &look,
                 const Airspaces *airspaces,
                 const RasterTerrain *terrain,
                 const FullBlackboard &_blackboard,
                 GlideComputer &_glide_computer)
    :blackboard(_blackboard), glide_computer(_glide_computer),
     dialog(_dialog),
     info(look.dialog),
     chart(*this, look.chart, look.map, look.cross_section,
           look.thermal_band, look.cross_section,
           look.map.airspace, airspaces, terrain,
           _blackboard, _glide_computer) {
  }

  void SetCalcVisibility(bool visible);
  void SetCalcCaption(const TCHAR *caption);

  void NextPage(int step);
  void Update();

  void OnGesture(const TCHAR *gesture);

private:
  void OnCalcClicked();

protected:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

  void Show(const PixelRect &rc) override {
    const Layout layout(rc);

    info.MoveAndShow(layout.info);
    details_button.MoveAndShow(layout.details_button);
    previous_button.MoveAndShow(layout.previous_button);
    next_button.MoveAndShow(layout.next_button);
    close_button.MoveAndShow(layout.close_button);
    chart.MoveAndShow(layout.main);

    Update();
    Timer::Schedule(2500);
  }

  void Hide() override {
    Timer::Cancel();

    info.Hide();
    details_button.Hide();
    previous_button.Hide();
    next_button.Hide();
    close_button.Hide();
    chart.Hide();
  }

  void Move(const PixelRect &rc) override {
    const Layout layout(rc);

    info.Move(layout.info);
    details_button.Move(layout.details_button);
    previous_button.Move(layout.previous_button);
    next_button.Move(layout.next_button);
    close_button.Move(layout.close_button);
    chart.Move(layout.main);
  }

  bool SetFocus() override {
    close_button.SetFocus();
    return true;
  }

  bool KeyPress(unsigned key_code) override;

private:
  /* virtual methods from class ActionListener */
  void OnAction(int id) override {
    switch (id) {
    case PREVIOUS:
      NextPage(-1);
      break;

    case NEXT:
      NextPage(1);
      break;

    case DETAILS:
      OnCalcClicked();
      break;
    }
  }

  /* virtual methods from class Timer */
  void OnTimer() override {
    Update();
  }
};

AnalysisWidget::Layout::Layout(const PixelRect rc)
{
  const unsigned width = rc.GetWidth(), height = rc.GetHeight();
  const unsigned button_height = ::Layout::GetMaximumControlHeight();

  main = rc;

  /* close button on the bottom left */

  close_button.left = rc.left;
  close_button.right = rc.left + ::Layout::Scale(70);
  close_button.bottom = rc.bottom;
  close_button.top = close_button.bottom - button_height;

  /* previous/next buttons above the close button */

  previous_button = close_button;
  previous_button.bottom = previous_button.top;
  previous_button.top = previous_button.bottom - button_height;
  previous_button.right = (previous_button.left + previous_button.right) / 2;

  next_button = previous_button;
  next_button.left = next_button.right;
  next_button.right = close_button.right;

  /* "details" button above "previous/next" */

  details_button = close_button;
  details_button.bottom = previous_button.top;
  details_button.top = details_button.bottom - button_height;

  if (width > height) {
    info = close_button;
    info.top = rc.top;
    info.bottom = details_button.top;

    main.left = close_button.right;
  } else {
    main.bottom = details_button.top;
    info.left = close_button.right;
    info.right = rc.right;
    info.top = main.bottom;
    info.bottom = rc.bottom;
  }
}

void
AnalysisWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const Layout layout(rc);

  WindowStyle button_style;
  button_style.Hide();
  button_style.TabStop();

  info.Create(parent, layout.info);

  const auto &button_look = dialog.GetLook().button;
  details_button.Create(parent, button_look, _T("Calc"), layout.details_button,
                        button_style, *this, DETAILS);
  previous_button.Create(parent, button_look, _T("<"), layout.previous_button,
                         button_style, *this, PREVIOUS);
  next_button.Create(parent, button_look, _T(">"), layout.next_button,
                     button_style, *this, NEXT);
  close_button.Create(parent, button_look, _("Close"), layout.close_button,
                      button_style, dialog, mrOK);

  WindowStyle style;
  style.Hide();

  chart.Create(parent, layout.main, style);
}

void
AnalysisWidget::SetCalcVisibility(bool visible)
{
  details_button.SetVisible(visible);
}

void
AnalysisWidget::SetCalcCaption(const TCHAR *caption)
{
  details_button.SetCaption(caption);
  SetCalcVisibility(!StringIsEmpty(caption));
}

void
ChartControl::OnPaint(Canvas &canvas)
{
  const ComputerSettings &settings_computer = blackboard.GetComputerSettings();
  const MapSettings &settings_map = blackboard.GetMapSettings();
  const MoreData &basic = blackboard.Basic();
  const DerivedInfo &calculated = blackboard.Calculated();

  const ProtectedTaskManager *const protected_task_manager =
    &glide_computer.GetProtectedTaskManager();

#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLCanvasScissor scissor(canvas);
#endif

  canvas.SetTextColor(COLOR_BLACK);

  PixelRect rcgfx = GetClientRect();

  // background is painted in the base-class

  switch (page) {
  case AnalysisPage::BAROGRAPH:
    RenderBarograph(canvas, rcgfx, chart_look, cross_section_look,
                    glide_computer.GetFlightStats(),
                    basic, calculated, protected_task_manager);
    break;
  case AnalysisPage::CLIMB:
    if (protected_task_manager != NULL) {
      ProtectedTaskManager::Lease task(*protected_task_manager);
      RenderClimbChart(canvas, rcgfx, chart_look,
                       glide_computer.GetFlightStats(),
                       settings_computer.polar.glide_polar_task,
                       basic, calculated, task);
    }
    break;
  case AnalysisPage::VARIO_HISTOGRAM:
    RenderVarioHistogram(canvas, rcgfx, chart_look,
                         glide_computer.GetFlightStats(),
                         settings_computer.polar.glide_polar_task);
    break;
  case AnalysisPage::THERMAL_BAND:
  {
    OrderedTaskSettings otb;
    if (protected_task_manager != NULL) {
      otb = protected_task_manager->GetOrderedTaskSettings();
    }

    thermal_band_renderer.DrawThermalBand(basic,
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
                    glide_computer.GetFlightStats(),
                    basic, glide_computer.GetWindStore());
    break;
  case AnalysisPage::POLAR:
    RenderGlidePolar(canvas, rcgfx, chart_look,
                     calculated.climb_history,
                     settings_computer.polar.glide_polar_task);
    break;
    case AnalysisPage::MACCREADY:
      RenderMacCready(canvas, rcgfx, chart_look,
                      settings_computer.polar.glide_polar_task);
    break;
  case AnalysisPage::TEMPTRACE:
    RenderTemperatureChart(canvas, rcgfx, chart_look,
                           glide_computer.GetCuSonde());
    break;
  case AnalysisPage::TASK:
    if (protected_task_manager != NULL) {
      const auto &trace_computer = glide_computer.GetTraceComputer();
      fs_renderer.RenderTask(canvas, rcgfx, basic,
                             settings_computer, settings_map,
                             *protected_task_manager,
                             &trace_computer);
    }
    break;

  case AnalysisPage::OLC:
    fs_renderer.RenderOLC(canvas, rcgfx, basic,
                          settings_computer, settings_map,
                          calculated.contest_stats,
                          glide_computer.GetTraceComputer(),
                          glide_computer.GetRetrospective());
    break;
  case AnalysisPage::TASK_SPEED:
    if (protected_task_manager != NULL) {
      ProtectedTaskManager::Lease task(*protected_task_manager);
      RenderSpeed(canvas, rcgfx, chart_look,
                  glide_computer.GetFlightStats(),
                  basic, calculated, task,
                  settings_computer.polar.glide_polar_task);
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

inline void
ChartControl::UpdateCrossSection(const MoreData &basic,
                                 const DerivedInfo &calculated,
                                 const GlideSettings &glide_settings,
                                 const GlidePolar &glide_polar,
                                 const MapSettings &map_settings)
{
  cross_section_renderer.ReadBlackboard(basic, calculated, glide_settings,
                                        glide_polar, map_settings);

  if (basic.location_available && basic.track_available) {
    cross_section_renderer.SetDirection(basic.track);
    cross_section_renderer.SetStart(basic.location);
  } else
    cross_section_renderer.SetInvalid();
}

void
AnalysisWidget::Update()
{
  TCHAR sTmp[1000];

  const ComputerSettings &settings_computer = blackboard.GetComputerSettings();
  const DerivedInfo &calculated = blackboard.Calculated();

  switch (page) {
  case AnalysisPage::BAROGRAPH:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Barograph"));
    dialog.SetCaption(sTmp);
    BarographCaption(sTmp, glide_computer.GetFlightStats());
    info.SetText(sTmp);
    SetCalcCaption(_("Settings"));
    break;

  case AnalysisPage::CLIMB:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Climb"));
    dialog.SetCaption(sTmp);
    ClimbChartCaption(sTmp, glide_computer.GetFlightStats());
    info.SetText(sTmp);
    SetCalcCaption(_("Task Calc"));
    break;

  case AnalysisPage::THERMAL_BAND:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Thermal Band"));
    dialog.SetCaption(sTmp);
    ClimbChartCaption(sTmp, glide_computer.GetFlightStats());
    info.SetText(sTmp);
    SetCalcCaption(_T(""));
    break;

  case AnalysisPage::VARIO_HISTOGRAM:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Vario Histogram"));
    dialog.SetCaption(sTmp);
    info.SetText(_T(""));
    SetCalcCaption(_T(""));
    break;

  case AnalysisPage::WIND:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Wind at Altitude"));
    dialog.SetCaption(sTmp);
    info.SetText(_T(""));
    SetCalcCaption(_("Set Wind"));
    break;

  case AnalysisPage::POLAR:
    StringFormatUnsafe(sTmp, _T("%s: %s (%s %d kg)"), _("Analysis"),
                       _("Glide Polar"), _("Mass"),
                       (int)settings_computer.polar.glide_polar_task.GetTotalMass());
    dialog.SetCaption(sTmp);
    GlidePolarCaption(sTmp, settings_computer.polar.glide_polar_task);
    info.SetText(sTmp);
    SetCalcCaption(_("Settings"));
    break;

  case AnalysisPage::MACCREADY:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("MacCready Speeds"));
    dialog.SetCaption(sTmp);
    MacCreadyCaption(sTmp, settings_computer.polar.glide_polar_task);
    info.SetText(sTmp);
    SetCalcCaption(_("Settings"));
    break;

  case AnalysisPage::TEMPTRACE:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Temperature Trace"));
    dialog.SetCaption(sTmp);
    TemperatureChartCaption(sTmp, glide_computer.GetCuSonde());
    info.SetText(sTmp);
    SetCalcCaption(_("Settings"));
    break;

  case AnalysisPage::TASK_SPEED:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Task Speed"));
    dialog.SetCaption(sTmp);
    TaskSpeedCaption(sTmp, glide_computer.GetFlightStats(),
                     settings_computer.polar.glide_polar_task);
    info.SetText(sTmp);
    SetCalcCaption(_("Task Calc"));
    break;

  case AnalysisPage::TASK:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Task"));
    dialog.SetCaption(sTmp);
    FlightStatisticsRenderer::CaptionTask(sTmp, calculated);
    info.SetText(sTmp);
    SetCalcCaption(_("Task calc"));
    break;

  case AnalysisPage::OLC:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       ContestToString(settings_computer.contest.contest));
    dialog.SetCaption(sTmp);
    SetCalcCaption(_T(""));
    FlightStatisticsRenderer::CaptionOLC(sTmp, settings_computer.contest,
                                         calculated);
    info.SetText(sTmp);
    break;

  case AnalysisPage::AIRSPACE:
    StringFormatUnsafe(sTmp, _T("%s: %s"), _("Analysis"),
                       _("Airspace"));
    dialog.SetCaption(sTmp);
    info.SetText(_T(""));
    SetCalcCaption(_("Warnings"));
    break;

  case AnalysisPage::COUNT:
    gcc_unreachable();
  }

  if (page == AnalysisPage::AIRSPACE)
    chart.UpdateCrossSection(blackboard.Basic(), calculated,
                             settings_computer.task.glide,
                             settings_computer.polar.glide_polar_task,
                             blackboard.GetMapSettings());


  chart.Invalidate();
}

void
AnalysisWidget::NextPage(int Step)
{
  int new_page = (int)page + Step;

  if (new_page >= (int)AnalysisPage::COUNT)
    new_page = 0;
  if (new_page < 0)
    new_page = (int)AnalysisPage::COUNT - 1;
  page = (AnalysisPage)new_page;

  Update();
}

void
AnalysisWidget::OnGesture(const TCHAR *gesture)
{
  if (StringIsEqual(gesture, _T("R")))
    NextPage(-1);
  else if (StringIsEqual(gesture, _T("L")))
    NextPage(+1);
}

bool
ChartControl::OnMouseDown(PixelPoint p)
{
  dragging = true;
  SetCapture();
  gestures.Start(p, Layout::Scale(20));
  return true;
}

bool
ChartControl::OnMouseMove(PixelPoint p, unsigned keys)
{
  if (dragging)
    gestures.Update(p);
  return true;
}

bool
ChartControl::OnMouseUp(PixelPoint p)
{
  if (dragging) {
    dragging = false;
    ReleaseCapture();

    const TCHAR *gesture = gestures.Finish();
    if (gesture != NULL)
      analysis_widget.OnGesture(gesture);
  }

  return true;
}

bool
AnalysisWidget::KeyPress(unsigned key_code)
{
  switch (key_code) {
  case KEY_LEFT:
    previous_button.SetFocus();
    NextPage(-1);
    return true;

  case KEY_RIGHT:
    next_button.SetFocus();
    NextPage(+1);
    return true;

  default:
    return false;
  }
}

inline void
AnalysisWidget::OnCalcClicked()
{
  switch (page) {
  case AnalysisPage::BAROGRAPH:
    dlgBasicSettingsShowModal();
    break;

  case AnalysisPage::CLIMB:
  case AnalysisPage::TASK:
  case AnalysisPage::TASK_SPEED:
    dlgStatusShowModal(2);
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

  case AnalysisPage::MACCREADY:
    dlgBasicSettingsShowModal();
    break;

  case AnalysisPage::AIRSPACE:
    dlgAirspaceWarningsShowModal(glide_computer.GetAirspaceWarnings());
    break;

  case AnalysisPage::THERMAL_BAND:
  case AnalysisPage::VARIO_HISTOGRAM:
  case AnalysisPage::OLC:
  case AnalysisPage::COUNT:
    break;
  }

  Update();
}

void
dlgAnalysisShowModal(SingleWindow &parent, const Look &look,
                     const FullBlackboard &blackboard,
                     GlideComputer &glide_computer,
                     const Airspaces *airspaces,
                     const RasterTerrain *terrain,
                     AnalysisPage _page)
{
  WidgetDialog dialog(look.dialog);
  AnalysisWidget analysis(dialog, look,
                          airspaces, terrain,
                          blackboard, glide_computer);
  dialog.CreateFull(parent, _("Analysis"), &analysis);

  if (_page != AnalysisPage::COUNT)
    page = (AnalysisPage)_page;

  dialog.ShowModal();
  dialog.StealWidget();
}
