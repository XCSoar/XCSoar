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

#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "resource.h"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Interface.hpp"
#include "UIActions.hpp"
#include "Input/InputEvents.hpp"
#include "Menu/ButtonLabel.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Blank.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Audio/Sound.hpp"
#include "Audio/VarioGlue.hpp"
#include "Components.hpp"
#include "ProcessTimer.hpp"
#include "LogFile.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeThermalAssistant.hpp"
#include "Gauge/GlueGaugeVario.hpp"
#include "Menu/MenuBar.hpp"
#include "Form/Form.hpp"
#include "Form/Widget.hpp"
#include "UtilsSystem.hpp"
#include "Look/Fonts.hpp"
#include "Look/Look.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "ProgressGlue.hpp"
#include "UIState.hpp"
#include "DrawThread.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Cache.hpp"
#endif

#if !defined(WIN32) && !defined(ANDROID)
#include <unistd.h>
#endif

#if !defined(WIN32) && !defined(ANDROID)
#include <unistd.h> /* for execl() */
#endif

gcc_pure
static PixelRect
GetBottomWidgetRect(const PixelRect &rc, const Widget *bottom_widget)
{
  if (bottom_widget == nullptr) {
    /* no bottom widget: return empty rectangle, map uses the whole
       main area */
    PixelRect result = rc;
    result.top = result.bottom;
    return result;
  }

  const UPixelScalar requested_height = bottom_widget->GetMinimumSize().cy;
  UPixelScalar height;
  if (requested_height > 0) {
    const UPixelScalar max_height = (rc.bottom - rc.top) / 2;
    height = std::min(max_height, requested_height);
  } else {
    const UPixelScalar recommended_height = (rc.bottom - rc.top) / 3;
    height = recommended_height;
  }

  PixelRect result = rc;
  result.top = result.bottom - height;
  return result;
}

gcc_pure
static PixelRect
GetMapRectAbove(const PixelRect &rc, const PixelRect &bottom_rect)
{
  PixelRect result = rc;
  result.bottom = bottom_rect.top;
  return result;
}

MainWindow::MainWindow(const StatusMessageList &status_messages)
  :look(NULL),
   map(nullptr), bottom_widget(nullptr), widget(nullptr), vario(*this),
   traffic_gauge(*this),
   suppress_traffic_gauge(false), force_traffic_gauge(false),
   thermal_assistant(*this),
   popup(status_messages, *this, CommonInterface::GetUISettings()),
   timer(*this),
   FullScreen(false),
#ifndef ENABLE_OPENGL
   draw_suspended(false),
#endif
   activate_map_pending(false),
   airspace_warning_pending(false)
{
}

/**
 * Destructor of the MainWindow-Class
 * @return
 */
MainWindow::~MainWindow()
{
  Destroy();
}

void
MainWindow::Create(PixelRect rc, TopWindowStyle style)
{
  SingleWindow::Create(title, rc, style);
}

gcc_noreturn
static void
NoFontsAvailable()
{
  const TCHAR *msg = _T("Font initialisation failed");

  /* log the error */
  LogStartUp(_T("%s"), msg);

  /* now try to get a GUI error message out to the user */
#ifdef WIN32
  MessageBox(NULL, msg, _T("XCSoar"), MB_ICONEXCLAMATION|MB_OK);
#elif !defined(ANDROID)
  execl("/usr/bin/xmessage", "xmessage", msg, NULL);
  execl("/usr/X11/bin/xmessage", "xmessage", msg, NULL);
#endif
  exit(EXIT_FAILURE);
}

void
MainWindow::Initialise()
{
  PixelRect rc = GetClientRect();

  Layout::Initialize(rc.right - rc.left, rc.bottom - rc.top);

  LogFormat("Initialise fonts");
  if (!Fonts::Initialize()) {
    Destroy();
    NoFontsAvailable();
  }

  if (look == NULL)
    look = new Look();

  look->Initialise(Fonts::map, Fonts::map_bold, Fonts::map_label);
}

void
MainWindow::InitialiseConfigured()
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  PixelRect rc = GetClientRect();

  const InfoBoxLayout::Layout ib_layout =
    InfoBoxLayout::Calculate(rc, ui_settings.info_boxes.geometry);

  Fonts::SizeInfoboxFont(ib_layout.control_width);

  if (ui_settings.custom_fonts) {
    LogFormat("Load custom fonts");
    if (!Fonts::LoadCustom()) {
      LogFormat("Failed to load custom fonts");
      if (!Fonts::Initialize()) {
        Destroy();
        NoFontsAvailable();
      }
    }

#ifdef ENABLE_OPENGL
    /* fonts may have changed, discard all pre-rendered font
       textures */
    TextCache::Flush();
#endif
  }

  assert(look != NULL);
  look->InitialiseConfigured(CommonInterface::GetUISettings(),
                             Fonts::map, Fonts::map_bold, Fonts::map_label,
                             Fonts::cdi, Fonts::monospace,
                             Fonts::infobox, Fonts::infobox_small,
#ifndef GNAV
                             Fonts::infobox_units,
#endif
                             Fonts::title);

  InfoBoxManager::Create(*this, ib_layout, look->info_box, look->units);
  map_rect = ib_layout.remaining;

  ButtonLabel::CreateButtonLabels(*this);
  ButtonLabel::SetFont(Fonts::map_bold);

  ReinitialiseLayout_vario(ib_layout);

  ReinitialiseLayoutTA(rc, ib_layout);

  WindowStyle hidden_border;
  hidden_border.Hide();
  hidden_border.Border();

  ReinitialiseLayout_flarm(rc, ib_layout);

  map = new GlueMapWindow(*look);
  map->SetComputerSettings(CommonInterface::GetComputerSettings());
  map->SetMapSettings(CommonInterface::GetMapSettings());
  map->SetUIState(CommonInterface::GetUIState());
  map->Create(*this, map_rect);
  map->SetFont(Fonts::map);

  popup.Create(rc);
}

void
MainWindow::Deinitialise()
{
  InfoBoxManager::Destroy();
  ButtonLabel::Destroy();

  popup.Destroy();

  // During destruction of GlueMapWindow WM_SETFOCUS gets called for
  // MainWindow which tries to set the focus to GlueMapWindow. Prevent
  // this issue by setting map to NULL before calling delete.
  GlueMapWindow *temp_map = map;
  map = NULL;
  delete temp_map;

  vario.Clear();
  traffic_gauge.Clear();
  thermal_assistant.Clear();

  delete look;
  look = NULL;
}

void
MainWindow::ReinitialiseLayout_vario(const InfoBoxLayout::Layout &layout)
{
  if (!layout.HasVario()) {
    vario.Clear();
    return;
  }

  if (!vario.IsDefined())
    vario.Set(new GlueGaugeVario(CommonInterface::GetLiveBlackboard(),
                                 look->vario, look->units));

  vario.Move(layout.vario);
  vario.Show();

  // XXX vario->BringToTop();
}

void
MainWindow::ReinitialiseLayoutTA(PixelRect rc,
                                 const InfoBoxLayout::Layout &layout)
{
  UPixelScalar sz = std::min(layout.control_height,
                             layout.control_width) * 2;
  rc.right = rc.left + sz;
  rc.top = rc.bottom - sz;
  thermal_assistant.Move(rc);
}

void
MainWindow::ReinitialiseLayout()
{
  if (map == NULL) {
#ifdef ANDROID
    if (HasDialog())
      dialogs.top()->ReinitialiseLayout();  // adapt simulator prompt
#endif
    /* without the MapWindow, it is safe to assume that the MainWindow
       is just being initialized, and the InfoBoxes aren't initialized
       yet either, so there is nothing to do here */
    return;
  }

#ifndef ENABLE_OPENGL
  if (draw_thread == NULL)
    /* no layout changes during startup */
    return;
#endif

  InfoBoxManager::Destroy();

  const UISettings &ui_settings = CommonInterface::GetUISettings();

  const PixelRect rc = GetClientRect();
  const InfoBoxLayout::Layout ib_layout =
    InfoBoxLayout::Calculate(rc, ui_settings.info_boxes.geometry);

  Fonts::SizeInfoboxFont(ib_layout.control_width);

  InfoBoxManager::Create(*this, ib_layout, look->info_box, look->units);
  InfoBoxManager::ProcessTimer();
  map_rect = ib_layout.remaining;

  popup.Destroy();
  popup.Create(rc);

  ReinitialiseLayout_vario(ib_layout);

  ReinitialiseLayout_flarm(rc, ib_layout);

  ReinitialiseLayoutTA(rc, ib_layout);

  if (map != NULL) {
    if (FullScreen)
      InfoBoxManager::Hide();
    else
      InfoBoxManager::Show();

    const PixelRect main_rect = GetMainRect();
    const PixelRect bottom_rect = GetBottomWidgetRect(main_rect,
                                                      bottom_widget);

    if (bottom_widget != nullptr)
      bottom_widget->Move(bottom_rect);

    map->Move(GetMapRectAbove(main_rect, bottom_rect));
    map->FullRedraw();
  }

  if (widget != NULL)
    widget->Move(GetMainRect(rc));

#ifdef ANDROID
  // move topmost dialog to fit into the current layout, or close it
  if (HasDialog())
    dialogs.top()->ReinitialiseLayout();
#endif

  if (map != NULL)
    map->BringToBottom();
}

void 
MainWindow::ReinitialiseLayout_flarm(PixelRect rc, const InfoBoxLayout::Layout ib_layout)
{
  TrafficSettings::GaugeLocation val =
    CommonInterface::GetUISettings().traffic.gauge_location;

  // Automatic mode - follow info boxes
  if (val == TrafficSettings::GaugeLocation::Auto) {
    switch (InfoBoxManager::layout.geometry) {
    case InfoBoxSettings::Geometry::TOP_8:
      val = TrafficSettings::GaugeLocation::TopRight;
      break;
    case InfoBoxSettings::Geometry::LEFT_8:
      val = TrafficSettings::GaugeLocation::BottomLeft;
      break;
    case InfoBoxSettings::Geometry::TOP_12:
      val = TrafficSettings::GaugeLocation::TopLeft;
      break;
    default:
      val = TrafficSettings::GaugeLocation::BottomRight;    // Assume bottom right unles...
      break;
    }
  }

  switch (val) {
  case TrafficSettings::GaugeLocation::TopLeft:
    rc.right = rc.left + ib_layout.control_width * 2;
    ++rc.left;
    rc.bottom = rc.top + ib_layout.control_height * 2;
    ++rc.top;
    break;

  case TrafficSettings::GaugeLocation::TopRight:
    rc.left = rc.right - ib_layout.control_width * 2 + 1;
    rc.bottom = rc.top + ib_layout.control_height * 2;
    ++rc.top;
    break;

  case TrafficSettings::GaugeLocation::BottomLeft:
    rc.right = rc.left + ib_layout.control_width * 2;
    ++rc.left;
    rc.top = rc.bottom - ib_layout.control_height * 2 + 1;
    break;

  case TrafficSettings::GaugeLocation::CentreTop:
    rc.left = (rc.left + rc.right) / 2 - ib_layout.control_width;
    rc.right = rc.left + ib_layout.control_width * 2 - 1;
    rc.bottom = rc.top + ib_layout.control_height * 2;
    ++rc.top;
    break;

  case TrafficSettings::GaugeLocation::CentreBottom:
    rc.left = (rc.left + rc.right) / 2 - ib_layout.control_width;
    rc.right = rc.left + ib_layout.control_width * 2 - 1;
    rc.top = rc.bottom - ib_layout.control_height * 2 + 1;
    break;

  default:    // aka flBottomRight
    rc.left = rc.right - ib_layout.control_width * 2 + 1;
    rc.top = rc.bottom - ib_layout.control_height * 2 + 1;
    break;
  }

  traffic_gauge.Move(rc);
}

void
MainWindow::Destroy()
{
  Deinitialise();

  TopWindow::Destroy();
}

void
MainWindow::FinishStartup()
{
  timer.Schedule(500); // 2 times per second

  ResumeThreads();
}

void
MainWindow::BeginShutdown()
{
  timer.Cancel();
}

void
MainWindow::SuspendThreads()
{
  if (map != NULL)
    map->SuspendThreads();
}

void
MainWindow::ResumeThreads()
{
  if (map != NULL)
    map->ResumeThreads();
}

void
MainWindow::SetDefaultFocus()
{
  if (map != NULL && widget == NULL)
    map->SetFocus();
  else if (widget == NULL || !widget->SetFocus())
    SetFocus();
}

void
MainWindow::FullRedraw()
{
  if (map != NULL)
    map->FullRedraw();
}

// Windows event handlers

void
MainWindow::OnResize(UPixelScalar width, UPixelScalar height)
{
  SingleWindow::OnResize(width, height);

  Layout::Initialize(width, height);

  ReinitialiseLayout();

  if (map != NULL) {
    /* the map being created already is an indicator that XCSoar is
       running already, and so we assume the menu buttons have been
       created, too */
    map->BringToBottom();
  }

  ButtonLabel::OnResize(GetClientRect());

  ProgressGlue::Resize(width, height);
}

bool
MainWindow::OnActivate()
{
  SingleWindow::OnActivate();

  Fullscreen();

  return true;
}

void
MainWindow::OnSetFocus()
{
  SingleWindow::OnSetFocus();

  if (!HasDialog()) {
    /* the main window should never have the keyboard focus; if we
       happen to get the focus despite of that, forward it to the map
       window to make keyboard shortcuts work */
    if (map != NULL && widget == NULL)
      map->SetFocus();
    else if (widget != NULL)
      widget->SetFocus();
  } else
    /* recover the dialog focus if it got lost */
    GetTopDialog().FocusFirstControl();
}

bool
MainWindow::OnKeyDown(unsigned key_code)
{
  return InputEvents::processKey(key_code) ||
    SingleWindow::OnKeyDown(key_code);
}

bool
MainWindow::OnTimer(WindowTimer &_timer)
{
  if (_timer != timer)
    return SingleWindow::OnTimer(_timer);

  ProcessTimer();

  UpdateGaugeVisibility();

  if (!CommonInterface::GetUISettings().enable_thermal_assistant_gauge) {
    thermal_assistant.Clear();
  } else if (!CommonInterface::Calculated().circling ||
             InputEvents::IsFlavour(_T("TA"))) {
    thermal_assistant.Hide();
  } else if (!HasDialog()) {
    if (!thermal_assistant.IsDefined())
      thermal_assistant.Set(new GaugeThermalAssistant(CommonInterface::GetLiveBlackboard(),
                                                      look->thermal_assistant_gauge));

    if (!thermal_assistant.IsVisible()) {
      thermal_assistant.Show();

      GaugeThermalAssistant *widget =
        (GaugeThermalAssistant *)thermal_assistant.Get();
      widget->Raise();
    }
  }

  battery_timer.Process();

  return true;
}

bool
MainWindow::OnUser(unsigned id)
{
  ProtectedAirspaceWarningManager *airspace_warnings;

  switch ((Command)id) {
  case Command::AIRSPACE_WARNING:
    airspace_warnings = GetAirspaceWarnings();
    if (!airspace_warning_pending || airspace_warnings == NULL)
      return true;

    airspace_warning_pending = false;
    if (dlgAirspaceWarningVisible())
      /* already visible */
      return true;

    /* un-blank the display, play a sound and show the dialog */
    ResetDisplayTimeOut();
    PlayResource(_T("IDR_WAV_BEEPBWEEP"));
    dlgAirspaceWarningsShowModal(*this, *airspace_warnings, true);
    return true;

  case Command::GPS_UPDATE:
    XCSoarInterface::ReceiveGPS();

    /*
     * Update the infoboxes if no location is available
     *
     * (if the location is available the CalculationThread will send the
     * Command::CALCULATED_UPDATE message which will update them)
     */
    if (!CommonInterface::Basic().location_available) {
      InfoBoxManager::SetDirty();
      InfoBoxManager::ProcessTimer();
    }

    if (CommonInterface::Basic().brutto_vario_available)
      AudioVarioGlue::SetValue(CommonInterface::Basic().brutto_vario);
    else
      AudioVarioGlue::NoValue();

    return true;

  case Command::CALCULATED_UPDATE:
    XCSoarInterface::ReceiveCalculated();

    CommonInterface::SetUIState().display_mode =
      GetNewDisplayMode(CommonInterface::GetUISettings().info_boxes,
                        CommonInterface::GetUIState(),
                        CommonInterface::Calculated());
    CommonInterface::SetUIState().panel_name =
      InfoBoxManager::GetCurrentPanelName();

    if (map != NULL) {
      map->SetUIState(CommonInterface::GetUIState());
      map->FullRedraw();
    }

    InfoBoxManager::SetDirty();
    InfoBoxManager::ProcessTimer();

    return true;

  case Command::ACTIVATE_MAP:
    if (activate_map_pending)
      ActivateMap();
    return true;
  }

  return false;
}

void
MainWindow::OnDestroy()
{
  timer.Cancel();

  KillWidget();

  SingleWindow::OnDestroy();
}

bool MainWindow::OnClose() {
  if (HasDialog() || !IsRunning())
    /* no shutdown dialog if XCSoar hasn't completed initialization
       yet (e.g. if we are in the simulator prompt) */
    return SingleWindow::OnClose();

  if (UIActions::CheckShutdown()) {
    XCSoarInterface::Shutdown();
  }
  return true;
}

void
MainWindow::SetFullScreen(bool _full_screen)
{
  if (_full_screen == FullScreen)
    return;

  FullScreen = _full_screen;

  if (FullScreen)
    InfoBoxManager::Hide();
  else
    InfoBoxManager::Show();

  if (widget != NULL)
    widget->Move(GetMainRect());

  if (map != NULL)
    map->FastMove(GetMainRect());

  // the repaint will be triggered by the DrawThread
}

void
MainWindow::SetTerrain(RasterTerrain *terrain)
{
  if (map != NULL)
    map->SetTerrain(terrain);
}

void
MainWindow::SetTopography(TopographyStore *topography)
{
  if (map != NULL)
    map->SetTopography(topography);
}

void
MainWindow::SetComputerSettings(const ComputerSettings &settings_computer)
{
  if (map != NULL)
    map->SetComputerSettings(settings_computer);
}

void
MainWindow::SetMapSettings(const MapSettings &settings_map)
{
  if (map != NULL)
    map->SetMapSettings(settings_map);
}

GlueMapWindow *
MainWindow::GetMapIfActive()
{
  return IsMapActive() ? map : NULL;
}

GlueMapWindow *
MainWindow::ActivateMap()
{
  activate_map_pending = false;

  if (map == NULL)
    return NULL;

  if (widget != NULL) {
    KillWidget();
    map->Show();
    map->SetFocus();

    if (bottom_widget != nullptr)
      bottom_widget->Show(GetBottomWidgetRect(GetMainRect(),
                                              bottom_widget));

#ifndef ENABLE_OPENGL
    if (draw_suspended) {
      draw_suspended = false;
      draw_thread->Resume();
    }
#endif
  }

  return map;
}

void
MainWindow::DeferredActivateMap()
{
  if (IsMapActive() || activate_map_pending)
    return;

  activate_map_pending = true;
  SendUser((unsigned)Command::ACTIVATE_MAP);
}

void
MainWindow::KillWidget()
{
  if (widget == NULL)
    return;

  widget->Leave();
  widget->Hide();
  widget->Unprepare();
  delete widget;
  widget = NULL;

  InputEvents::SetFlavour(NULL);
}

void
MainWindow::SetBottomWidget(Widget *_widget)
{
  if (bottom_widget == nullptr && _widget == nullptr)
    return;

  if (map == nullptr) {
    /* this doesn't work without a map */
    delete _widget;
    return;
  }

  if (bottom_widget != nullptr) {
    if (widget == nullptr)
      bottom_widget->Hide();
    bottom_widget->Unprepare();
    delete bottom_widget;
  }

  bottom_widget = _widget;

  const PixelRect main_rect = GetMainRect();
  const PixelRect bottom_rect = GetBottomWidgetRect(main_rect,
                                                    bottom_widget);

  if (bottom_widget != nullptr) {
    bottom_widget->Initialise(*this, bottom_rect);
    bottom_widget->Prepare(*this, bottom_rect);

    if (widget == nullptr)
      bottom_widget->Show(bottom_rect);
  }

  map->Move(GetMapRectAbove(main_rect, bottom_rect));
  map->FullRedraw();
}

void
MainWindow::SetWidget(Widget *_widget)
{
  assert(_widget != NULL);

  activate_map_pending = false;

  /* delete the old widget */
  KillWidget();

  /* hide the map (might be hidden already) */
  if (map != NULL) {
    map->FastHide();

#ifndef ENABLE_OPENGL
    if (!draw_suspended) {
      draw_suspended = true;
      draw_thread->BeginSuspend();
    }
#endif
  }

  if (bottom_widget != nullptr)
    bottom_widget->Hide();

  widget = _widget;

  const PixelRect rc = GetMainRect();
  widget->Initialise(*this, rc);
  widget->Prepare(*this, rc);
  widget->Show(rc);

  if (!widget->SetFocus())
    SetFocus();
}

Widget *
MainWindow::GetFlavourWidget(const TCHAR *flavour)
{
  return InputEvents::IsFlavour(flavour)
    ? widget
    : nullptr;
}

void
MainWindow::UpdateGaugeVisibility()
{
  bool full_screen = GetFullScreen();

  vario.SetVisible(!full_screen &&
                   !CommonInterface::GetUIState().screen_blanked);

  UpdateTrafficGaugeVisibility();
}

void
MainWindow::UpdateTrafficGaugeVisibility()
{
  const FlarmData &flarm = CommonInterface::Basic().flarm;

  bool traffic_visible =
    (force_traffic_gauge ||
     (CommonInterface::GetUISettings().traffic.enable_gauge &&
      !flarm.traffic.IsEmpty())) &&
    !CommonInterface::GetUIState().screen_blanked &&
    /* hide the traffic gauge while the traffic widget is visible, to
       avoid showing the same information twice */
    !InputEvents::IsFlavour(_T("Traffic"));

  if (traffic_visible && suppress_traffic_gauge) {
    if (flarm.status.available &&
        flarm.status.alarm_level != FlarmTraffic::AlarmType::NONE)
      suppress_traffic_gauge = false;
    else
      traffic_visible = false;
  }

  if (traffic_visible) {
    if (HasDialog())
      return;

    if (!traffic_gauge.IsDefined())
      traffic_gauge.Set(new GaugeFLARM(CommonInterface::GetLiveBlackboard(),
                                       GetLook().flarm_gauge));

    if (!traffic_gauge.IsVisible()) {
      traffic_gauge.Show();

      GaugeFLARM *widget = (GaugeFLARM *)traffic_gauge.Get();
      widget->Raise();
    }
  } else
    traffic_gauge.Hide();
}

const MapWindowProjection &
MainWindow::GetProjection() const
{
  AssertThread();
  assert(map != NULL);

  return map->VisibleProjection();
}

void
MainWindow::ToggleSuppressFLARMRadar()
{
  suppress_traffic_gauge = !suppress_traffic_gauge;
}

void
MainWindow::ToggleForceFLARMRadar()
{
  force_traffic_gauge = !force_traffic_gauge;
  CommonInterface::SetUISettings().traffic.enable_gauge = force_traffic_gauge;
}

#ifdef ANDROID

void
MainWindow::OnPause()
{
  if (!IsRunning() && HasDialog())
    /* suspending before initialization has finished doesn't leave
       anything worth resuming, so let's just quit now */
    CancelDialog();

  SingleWindow::OnPause();
}

#endif /* ANDROID */
