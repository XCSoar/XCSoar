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
#include "InputEvents.hpp"
#include "ButtonLabel.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Blank.hpp"
#include "Dialogs/AirspaceWarningDialog.hpp"
#include "Dialogs/Task.hpp"
#include "Audio/Sound.hpp"
#include "Components.hpp"
#include "ProcessTimer.hpp"
#include "LogFile.hpp"
#include "Screen/Fonts.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeThermalAssistant.hpp"
#include "Gauge/GlueGaugeVario.hpp"
#include "MenuBar.hpp"
#include "Form/Form.hpp"
#include "Form/Widget.hpp"
#include "UtilsSystem.hpp"
#include "Look/Look.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "ProgressGlue.hpp"
#include "UIState.hpp"

MainWindow::MainWindow(const StatusMessageList &status_messages)
  :look(NULL),
   map(NULL), widget(NULL), vario(NULL), flarm(NULL), ta(NULL),
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

  WindowStyle hidden_border;
  hidden_border.hide();
  hidden_border.border();

  flarm = new GaugeFLARM(*this,
                         0, // assume top left, moved in layout
                         0,
                         ib_layout.control_width * 2 - 1,
                         ib_layout.control_height * 2 - 1,
                         look->flarm_gauge,
                         hidden_border);
  flarm->bring_to_top();
  ReinitialiseLayout_flarm(rc, ib_layout);

  UPixelScalar sz = std::min(ib_layout.control_height,
                             ib_layout.control_width) * 2;

  ta = new GaugeThermalAssistant(*this, 0, rc.bottom - sz, sz, sz,
                                 hidden_border);
  ta->bring_to_top();

  map = new GlueMapWindow(*look);
  map->SetSettingsComputer(CommonInterface::SettingsComputer());
  map->SetSettingsMap(CommonInterface::SettingsMap());
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

  delete vario;
  vario = NULL;

  delete flarm;
  flarm = NULL;

  delete ta;
  ta = NULL;

  delete look;
  look = NULL;
}

void
MainWindow::ReinitialiseLayout_vario(const InfoBoxLayout::Layout &layout)
{
  if (!layout.HasVario()) {
    delete vario;
    vario = NULL;
    return;
  }

  if (vario == NULL) {
    WindowStyle hidden;
    hidden.hide();

    vario = new GlueGaugeVario(CommonInterface::Full(),
                               *this, look->vario,
                               layout.vario.left,
                               layout.vario.top,
                               layout.vario.right - layout.vario.left,
                               layout.vario.bottom - layout.vario.top,
                               hidden);
  } else
    vario->move(layout.vario);

  vario->bring_to_top();
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

  if (ta != NULL) {
    UPixelScalar sz = std::min(ib_layout.control_height,
                               ib_layout.control_width) * 2;
    ta->move(0, rc.bottom - sz, sz, sz);
  }

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
  if (flarm != NULL) {
    unsigned val = 0;
    if (!Profile::Get(szProfileFlarmLocation, val)) val = flAuto;

    // Automatic mode - follow info boxes
    if (val == flAuto) {
      switch (InfoBoxLayout::InfoBoxGeometry) {
        case InfoBoxLayout::ibTop8:
          val = flTopRight;
          break;
        case InfoBoxLayout::ibLeft8:
          val = flBottomLeft;
          break;
        case InfoBoxLayout::ibTop12:
          val = flTopLeft;
          break;
        default:
          val = flBottomRight;    // Assume bottom right unles...
          break;
      }
    }

    switch (val) {
      case flTopLeft:
        flarm->move(
          rc.left + 1, 
          rc.top + 1,
          ib_layout.control_width * 2 - 1,
          ib_layout.control_height * 2 - 1
        );
        break;
      case flTopRight:
        flarm->move(
          rc.right - ib_layout.control_width * 2 + 1, 
          rc.top + 1,
          ib_layout.control_width * 2 - 1,
          ib_layout.control_height * 2 - 1
        );
        break;
      case flBottomLeft:
        flarm->move(
          rc.left + 1, 
          rc.bottom - ib_layout.control_height * 2 + 1,
          ib_layout.control_width * 2 - 1,
          ib_layout.control_height * 2 - 1
        );
        break;
      case flCentreTop:
        flarm->move(
          rc.left + ((rc.right - rc.left) / 2) - ib_layout.control_width, 
          1,
          ib_layout.control_width * 2 - 1,
          ib_layout.control_height * 2 - 1
        );
        break;
      case flCentreBottom:
        flarm->move(
          rc.left + ((rc.right - rc.left) / 2) - ib_layout.control_width, 
          rc.bottom - ib_layout.control_height * 2 + 1,
          ib_layout.control_width * 2 - 1,
          ib_layout.control_height * 2 - 1
        );
        break;
      default:    // aka flBottomRight
        flarm->move(
          rc.right - ib_layout.control_width * 2 + 1, 
          rc.bottom - ib_layout.control_height * 2 + 1,
          ib_layout.control_width * 2 - 1,
          ib_layout.control_height * 2 - 1
        );
        break;
    }
  }
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

bool
MainWindow::on_resize(UPixelScalar width, UPixelScalar height)
{
  SingleWindow::on_resize(width, height);

  Layout::Initialize(width, height);

  ReinitialiseLayout();

  if (map != NULL) {
    /* the map being created already is an indicator that XCSoar is
       running already, and so we assume the menu buttons have been
       created, too */

    ButtonLabel::Destroy();
    ButtonLabel::CreateButtonLabels(*this);
    ButtonLabel::SetFont(Fonts::map_bold);

    map->BringToBottom();
  }

  ProgressGlue::Resize(width, height);

  return true;
}

bool
MainWindow::on_activate()
{
  SingleWindow::on_activate();

  full_screen();

  return true;
}

bool
MainWindow::on_setfocus()
{
  if (!has_dialog()) {
    /* the main window should never have the keyboard focus; if we
       happen to get the focus despite of that, forward it to the map
       window to make keyboard shortcuts work */
    if (map != NULL && widget == NULL)
      map->set_focus();
    else if (widget != NULL)
      widget->SetFocus();
    return true;
  }

  return SingleWindow::on_setfocus();
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

    if (flarm != NULL)
      flarm->Update(CommonInterface::GetUISettings().enable_flarm_gauge,
                    CommonInterface::Basic(),
                    CommonInterface::SettingsComputer());

    if (ta != NULL)
      ta->Update(CommonInterface::GetUISettings().enable_thermal_assistant_gauge,
                 CommonInterface::Calculated().heading,
                 CommonInterface::Calculated());
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

  case CMD_CALCULATED_UPDATE:
    if (map != NULL)
      map->FullRedraw();

    InfoBoxManager::SetDirty();
    InfoBoxManager::ProcessTimer();

    TargetDialogUpdate();
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
MainWindow::SetSettingsComputer(const SETTINGS_COMPUTER &settings_computer)
{
  if (map != NULL)
    map->SetSettingsComputer(settings_computer);
}

void
MainWindow::SetSettingsMap(const SETTINGS_MAP &settings_map)
{
  if (map != NULL)
    map->SetSettingsMap(settings_map);
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

  if (vario != NULL)
    vario->set_visible(!full_screen &&
                       !CommonInterface::GetUIState().screen_blanked);

  if (flarm != NULL && CommonInterface::Basic().flarm.new_traffic)
    flarm->Suppress = false;
}

void
MainWindow::TriggerVarioUpdate()
{
  if (vario != NULL)
    vario->invalidate_blackboard();
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
  if (flarm == NULL)
    return;

  flarm->Suppress = !flarm->Suppress;
}

void
MainWindow::ToggleForceFLARMRadar()
{
  if (flarm == NULL)
    return;

  flarm->ForceVisible = !flarm->ForceVisible;
  CommonInterface::SetUISettings().enable_flarm_gauge = flarm->ForceVisible;
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
