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

#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "resource.h"
#include "Protection.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Interface.hpp"
#include "Input/InputEvents.hpp"
#include "Menu/ButtonLabel.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Blank.hpp"
#include "Dialogs/AirspaceWarningDialog.hpp"
#include "Audio/Sound.hpp"
#include "Components.hpp"
#include "ProcessTimer.hpp"
#include "LogFile.hpp"
#include "Screen/Fonts.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeThermalAssistant.hpp"
#include "Gauge/GlueGaugeVario.hpp"
#include "Menu/MenuBar.hpp"
#include "Form/Form.hpp"
#include "Form/Widget.hpp"
#include "UtilsSystem.hpp"
#include "Look/Look.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "ProgressGlue.hpp"
#include "UIState.hpp"

#if !defined(WIN32) && !defined(ANDROID)
#include <unistd.h>
#endif

#if !defined(WIN32) && !defined(ANDROID)
#include <unistd.h> /* for execl() */
#endif

MainWindow::MainWindow(const StatusMessageList &status_messages)
  :look(NULL),
   map(NULL), widget(NULL), vario(*this),
   traffic_gauge(*this),
   suppress_traffic_gauge(false), force_traffic_gauge(false),
   thermal_assistant(*this),
   popup(status_messages, *this, CommonInterface::GetUISettings()),
   timer(*this),
   FullScreen(false),
   airspace_warning_pending(false)
{
}

/**
 * Destructor of the MainWindow-Class
 * @return
 */
MainWindow::~MainWindow()
{
  reset();
}

#ifdef USE_GDI

bool
MainWindow::register_class(HINSTANCE hInstance)
{
  WNDCLASS wc;

  wc.style                      = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = Window::WndProc;
  wc.cbClsExtra                 = 0;
  wc.cbWndExtra = 0;
  wc.hInstance                  = hInstance;
  wc.hIcon                      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOAR));
  wc.hCursor                    = 0;
  wc.hbrBackground = NULL;
  wc.lpszMenuName               = 0;
  wc.lpszClassName = _T("XCSoarMain");

  return (RegisterClass(&wc)!= FALSE);
}

#endif /* USE_GDI */

void
MainWindow::set(const TCHAR* text,
                PixelScalar left, PixelScalar top,
                UPixelScalar width, UPixelScalar height)
{
  SingleWindow::set(_T("XCSoarMain"), text, left, top, width, height);
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
  PixelRect rc = get_client_rect();

  Layout::Initialize(rc.right - rc.left, rc.bottom - rc.top);

  // color/pattern chart (must have infobox geometry before this)
  Graphics::Initialise();

  LogStartUp(_T("Initialise fonts"));
  if (!Fonts::Initialize()) {
    reset();
    NoFontsAvailable();
  }

  if (look == NULL)
    look = new Look();

  look->Initialise();
}

void
MainWindow::InitialiseConfigured()
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  PixelRect rc = get_client_rect();

  LogStartUp(_T("InfoBox geometry"));
  InfoBoxLayout::Init(rc);
  const InfoBoxLayout::Layout ib_layout =
    InfoBoxLayout::Calculate(rc, InfoBoxLayout::InfoBoxGeometry);

  Fonts::SizeInfoboxFont(ib_layout.control_width);

  if (ui_settings.custom_fonts) {
    LogStartUp(_T("Load custom fonts"));
    if (!Fonts::LoadCustom()) {
      LogStartUp(_T("Failed to load custom fonts"));
      if (!Fonts::Initialize()) {
        reset();
        NoFontsAvailable();
      }
    }
  }

  assert(look != NULL);
  look->InitialiseConfigured(CommonInterface::GetUISettings());

  LogStartUp(_T("Create info boxes"));
  InfoBoxManager::Create(rc, ib_layout, look->info_box);
  map_rect = ib_layout.remaining;

  LogStartUp(_T("Create button labels"));
  ButtonLabel::CreateButtonLabels(*this);
  ButtonLabel::SetFont(Fonts::map_bold);

  ReinitialiseLayout_vario(ib_layout);

  ReinitialiseLayoutTA(rc, ib_layout);

  WindowStyle hidden_border;
  hidden_border.hide();
  hidden_border.border();

  ReinitialiseLayout_flarm(rc, ib_layout);

  map = new GlueMapWindow(*look);
  map->SetComputerSettings(CommonInterface::GetComputerSettings());
  map->SetMapSettings(CommonInterface::GetMapSettings());
  map->set(*this, map_rect);
  map->set_font(Fonts::map);

  LogStartUp(_T("Initialise message system"));
  popup.set(rc);
}

void
MainWindow::Deinitialise()
{
  InfoBoxManager::Destroy();
  ButtonLabel::Destroy();

  popup.reset();

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
                                 look->vario));

  vario.Move(layout.vario);
  vario.Show();

  // XXX vario->bring_to_top();
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
    if (has_dialog())
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

  PixelRect rc = get_client_rect();
  InfoBoxLayout::Init(rc);
  const InfoBoxLayout::Layout ib_layout =
    InfoBoxLayout::Calculate(rc, InfoBoxLayout::InfoBoxGeometry);

  Fonts::SizeInfoboxFont(ib_layout.control_width);

  InfoBoxManager::Create(rc, ib_layout, look->info_box);
  InfoBoxManager::ProcessTimer();
  map_rect = ib_layout.remaining;

  popup.reset();
  popup.set(rc);

  ReinitialiseLayout_vario(ib_layout);

  ReinitialiseLayout_flarm(rc, ib_layout);

  ReinitialiseLayoutTA(rc, ib_layout);

  if (map != NULL) {
    if (FullScreen)
      InfoBoxManager::Hide();
    else
      InfoBoxManager::Show();

    const PixelRect &current_map = FullScreen ? rc : map_rect;
    map->move(current_map.left, current_map.top,
              current_map.right - current_map.left,
              current_map.bottom - current_map.top);
    map->FullRedraw();
  }

  if (widget != NULL) {
    const PixelRect &current_map = FullScreen ? rc : map_rect;
    widget->Move(current_map);
  }

#ifdef ANDROID
  // move topmost dialog to fit into the current layout, or close it
  if (has_dialog())
    dialogs.top()->ReinitialiseLayout();
#endif

  if (map != NULL)
    map->BringToBottom();
}

void 
MainWindow::ReinitialiseLayout_flarm(PixelRect rc, const InfoBoxLayout::Layout ib_layout)
{
  UISettings::FlarmLocation val = CommonInterface::GetUISettings().flarm_location;

  // Automatic mode - follow info boxes
  if (val == UISettings::flAuto) {
    switch (InfoBoxLayout::InfoBoxGeometry) {
    case InfoBoxLayout::ibTop8:
      val = UISettings::flTopRight;
      break;
    case InfoBoxLayout::ibLeft8:
      val = UISettings::flBottomLeft;
      break;
    case InfoBoxLayout::ibTop12:
      val = UISettings::flTopLeft;
      break;
    default:
      val = UISettings::flBottomRight;    // Assume bottom right unles...
      break;
    }
  }

  switch (val) {
  case UISettings::flTopLeft:
    rc.right = rc.left + ib_layout.control_width * 2;
    ++rc.left;
    rc.bottom = rc.top + ib_layout.control_height * 2;
    ++rc.top;
    break;

  case UISettings::flTopRight:
    rc.left = rc.right - ib_layout.control_width * 2 + 1;
    rc.bottom = rc.top + ib_layout.control_height * 2;
    ++rc.top;
    break;

  case UISettings::flBottomLeft:
    rc.right = rc.left + ib_layout.control_width * 2;
    ++rc.left;
    rc.top = rc.bottom - ib_layout.control_height * 2 + 1;
    break;

  case UISettings::flCentreTop:
    rc.left = (rc.left + rc.right) / 2 - ib_layout.control_width;
    rc.right = rc.left + ib_layout.control_width * 2 - 1;
    rc.bottom = rc.top + ib_layout.control_height * 2;
    ++rc.top;
    break;

  case UISettings::flCentreBottom:
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
MainWindow::ReinitialisePosition()
{
  PixelRect rc = SystemWindowSize();
  fast_move(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
}

void
MainWindow::reset()
{
  Deinitialise();

  TopWindow::reset();
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
    map->set_focus();
  else if (widget == NULL || !widget->SetFocus())
    set_focus();
}

void
MainWindow::full_redraw()
{
  if (map != NULL)
    map->FullRedraw();
}

// Windows event handlers

void
MainWindow::on_resize(UPixelScalar width, UPixelScalar height)
{
  SingleWindow::on_resize(width, height);

  Layout::Initialize(width, height);

  ReinitialiseLayout();

  if (map != NULL) {
    /* the map being created already is an indicator that XCSoar is
       running already, and so we assume the menu buttons have been
       created, too */
    map->BringToBottom();
  }

  ButtonLabel::OnResize(get_client_rect());

  ProgressGlue::Resize(width, height);
}

bool
MainWindow::on_activate()
{
  SingleWindow::on_activate();

  full_screen();

  return true;
}

void
MainWindow::on_setfocus()
{
  SingleWindow::on_setfocus();

  if (!has_dialog()) {
    /* the main window should never have the keyboard focus; if we
       happen to get the focus despite of that, forward it to the map
       window to make keyboard shortcuts work */
    if (map != NULL && widget == NULL)
      map->set_focus();
    else if (widget != NULL)
      widget->SetFocus();
  }
}

bool
MainWindow::on_key_down(unsigned key_code)
{
  return InputEvents::processKey(key_code) ||
    SingleWindow::on_key_down(key_code);
}

bool
MainWindow::on_timer(WindowTimer &_timer)
{
  if (_timer != timer)
    return SingleWindow::on_timer(_timer);

  if (globalRunningEvent.Test()) {
    battery_timer.Process();

    ProcessTimer::Process();

    if (!CommonInterface::GetUISettings().enable_thermal_assistant_gauge) {
      thermal_assistant.Clear();
    } else if (!CommonInterface::Calculated().circling ||
               InputEvents::IsFlavour(_T("TA"))) {
      thermal_assistant.Hide();
    } else if (!has_dialog()) {
      if (!thermal_assistant.IsDefined())
        thermal_assistant.Set(new GaugeThermalAssistant(CommonInterface::GetLiveBlackboard()));

      if (!thermal_assistant.IsVisible()) {
        thermal_assistant.Show();

        GaugeThermalAssistant *widget =
          (GaugeThermalAssistant *)thermal_assistant.Get();
        widget->Raise();
      }
    }
  }

  return true;
}

bool
MainWindow::on_user(unsigned id)
{
  ProtectedAirspaceWarningManager *airspace_warnings;

  switch ((enum cmd)id) {
  case CMD_AIRSPACE_WARNING:
    airspace_warnings = GetAirspaceWarnings();
    if (!airspace_warning_pending || airspace_warnings == NULL)
      return true;

    airspace_warning_pending = false;
    if (dlgAirspaceWarningVisible())
      /* already visible */
      return true;

    /* un-blank the display, play a sound and show the dialog */
    ResetDisplayTimeOut();
#ifndef GNAV
    PlayResource(_T("IDR_WAV_BEEPBWEEP"));
#endif
    dlgAirspaceWarningsShowModal(*this, *airspace_warnings, true);
    return true;

  case CMD_GPS_UPDATE:
    XCSoarInterface::ReceiveGPS();
    return true;

  case CMD_CALCULATED_UPDATE:
    XCSoarInterface::ReceiveCalculated();

    if (map != NULL)
      map->FullRedraw();

    InfoBoxManager::SetDirty();
    InfoBoxManager::ProcessTimer();

    return true;
  }

  return false;
}

void
MainWindow::on_create()
{
  SingleWindow::on_create();

  timer.Schedule(500); // 2 times per second
}

void
MainWindow::on_destroy()
{
  timer.Cancel();

  KillWidget();

  SingleWindow::on_destroy();
}

bool MainWindow::on_close() {
  if (!IsRunning())
    /* no shutdown dialog if XCSoar hasn't completed initialization
       yet (e.g. if we are in the simulator prompt) */
    return SingleWindow::on_close();

  if (XCSoarInterface::CheckShutdown()) {
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
    widget->Move(FullScreen ? get_client_rect() : map_rect);

  if (map != NULL) {
    const PixelRect rc = FullScreen ? get_client_rect() : map_rect;
    map->fast_move(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
  }

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

DisplayMode
MainWindow::GetDisplayMode() const
{
  return map != NULL
    ? map->GetDisplayMode()
    : DM_NONE;
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
  if (map == NULL)
    return NULL;

  if (widget != NULL) {
    KillWidget();
    map->show();
    map->set_focus();
  }

  return map;
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
MainWindow::SetWidget(Widget *_widget)
{
  assert(_widget != NULL);

  /* delete the old widget */
  KillWidget();

  /* hide the map (might be hidden already) */
  if (map != NULL)
    map->fast_hide();

  widget = _widget;

  const PixelRect rc = FullScreen ? get_client_rect() : map_rect;
  widget->Initialise(*this, rc);
  widget->Prepare(*this, rc);
  widget->Show(rc);

  if (!widget->SetFocus())
    set_focus();
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
  const FlarmState &flarm = CommonInterface::Basic().flarm;
  bool traffic_visible =
    (force_traffic_gauge ||
     (CommonInterface::GetUISettings().enable_flarm_gauge &&
      flarm.available && !flarm.traffic.empty())) &&
    !CommonInterface::GetUIState().screen_blanked &&
    /* hide the traffic gauge while the traffic widget is visible, to
       avoid showing the same information twice */
    !InputEvents::IsFlavour(_T("Traffic"));

  if (traffic_visible && suppress_traffic_gauge) {
    if (flarm.available &&
        flarm.alarm_level != FlarmTraffic::AlarmType::NONE)
      suppress_traffic_gauge = false;
    else
      traffic_visible = false;
  }

  if (traffic_visible) {
    if (has_dialog())
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
  assert_thread();
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
  CommonInterface::SetUISettings().enable_flarm_gauge = force_traffic_gauge;
}

#ifdef ANDROID

void
MainWindow::on_pause()
{
  if (!IsRunning() && has_dialog())
    /* suspending before initialization has finished doesn't leave
       anything worth resuming, so let's just quit now */
    CancelDialog();

  SingleWindow::on_pause();
}

#endif /* ANDROID */
