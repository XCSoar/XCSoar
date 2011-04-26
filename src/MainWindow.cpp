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
#include "resource.h"
#include "Protection.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Interface.hpp"
#include "ButtonLabel.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Components.hpp"
#include "ProcessTimer.hpp"
#include "LogFile.hpp"
#include "Screen/Fonts.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeThermalAssistant.hpp"
#include "Gauge/GlueGaugeVario.hpp"
#include "MenuBar.hpp"
#include "Form/Form.hpp"
#include "Appearance.hpp"
#include "UtilsSystem.hpp"

/**
 * Destructor of the MainWindow-Class
 * @return
 */
MainWindow::~MainWindow()
{
  reset();
}

bool
MainWindow::register_class(HINSTANCE hInstance)
{
#ifdef ENABLE_SDL
  return true;
#else /* !ENABLE_SDL */
  WNDCLASS wc;

  wc.style                      = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = Window::WndProc;
  wc.cbClsExtra                 = 0;
  wc.cbWndExtra = 0;
  wc.hInstance                  = hInstance;
  wc.hIcon                      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOAR));
  wc.hCursor                    = 0;
  wc.hbrBackground              = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName               = 0;
  wc.lpszClassName = _T("XCSoarMain");

  return (RegisterClass(&wc)!= FALSE);
#endif /* !ENABLE_SDL */
}

void
MainWindow::set(const TCHAR* text,
                int left, int top, unsigned width, unsigned height)
{
  SingleWindow::set(_T("XCSoarMain"), text, left, top, width, height);
}

void
MainWindow::Initialise()
{
  PixelRect rc = get_client_rect();

  Layout::Initialize(rc.right - rc.left, rc.bottom - rc.top);

  // color/pattern chart (must have infobox geometry before this)
  Graphics::Initialise();

  LogStartUp(_T("Initialise fonts"));
  Fonts::Initialize();
}

void
MainWindow::InitialiseConfigured()
{
  PixelRect rc = get_client_rect();

  LogStartUp(_T("InfoBox geometry"));
  InfoBoxLayout::Init(rc);
  const InfoBoxLayout::Layout ib_layout =
    InfoBoxLayout::Calculate(rc, InfoBoxLayout::InfoBoxGeometry);

  Fonts::SizeInfoboxFont(ib_layout.control_width);

  if (Appearance.UseCustomFonts) {
    LogStartUp(_T("Load fonts"));
    Fonts::LoadCustom();
  }

  LogStartUp(_T("Create info boxes"));
  InfoBoxManager::Create(rc, ib_layout);
  map_rect = ib_layout.remaining;

  LogStartUp(_T("Create button labels"));
  ButtonLabel::CreateButtonLabels(*this);
  ButtonLabel::SetLabelText(0,_T("MODE"));
  ButtonLabel::SetFont(Fonts::MapBold);

  WindowStyle hidden;
  hidden.hide();

  vario = new GlueGaugeVario(*this,
                             rc.right - ib_layout.control_width, 0,
                             ib_layout.control_width,
                             ib_layout.control_height * 3,
                             hidden);

  WindowStyle hidden_border;
  hidden_border.hide();
  hidden_border.border();

  flarm = new GaugeFLARM(*this,
                         rc.right - ib_layout.control_width * 2 + 1,
                         rc.bottom - ib_layout.control_height * 2 + 1,
                         ib_layout.control_width * 2 - 1,
                         ib_layout.control_height * 2 - 1,
                         hidden_border);
  flarm->bring_to_top();

  unsigned sz = std::min(ib_layout.control_height,
                         ib_layout.control_width) * 2;

  ta = new GaugeThermalAssistant(*this, 0, rc.bottom - sz, sz, sz,
                                 hidden_border);
  ta->bring_to_top();

  map.set(*this, map_rect);
  map.set_font(Fonts::Map);

  LogStartUp(_T("Initialise message system"));
  popup.set(rc);
}

void
MainWindow::ReinitialiseLayout()
{
  if (!map.defined()) {
#ifdef ANDROID
    if (has_dialog())
      dialogs.top()->ReinitialiseLayout();  // adapt simulator prompt
#endif
    /* without the MapWindow, it is safe to assume that the MainWindow
       is just being initialized, and the InfoBoxes aren't initialized
       yet either, so there is nothing to do here */
    return;
  }

#ifndef ENABLE_SDL
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

  InfoBoxManager::Create(rc, ib_layout);
  map_rect = ib_layout.remaining;

  if (vario != NULL)
    vario->move(rc.right - ib_layout.control_width, 0,
                ib_layout.control_width,
                ib_layout.control_height * 3);

  if (flarm != NULL)
    flarm->move(rc.right - ib_layout.control_width * 2 + 1,
                rc.bottom - ib_layout.control_height * 2 + 1,
                ib_layout.control_width * 2 - 1,
                ib_layout.control_height * 2 - 1);

  if (ta != NULL) {
    unsigned sz = std::min(ib_layout.control_height,
                           ib_layout.control_width) * 2;
    ta->move(0, rc.bottom - sz, sz, sz);
  }

  if (!FullScreen) {
    map.move(map_rect.left, map_rect.top,
             map_rect.right - map_rect.left,
             map_rect.bottom - map_rect.top);
    map.FullRedraw();
  }

#ifdef ANDROID
  // move topmost dialog to fit into the current layout, or close it
  if (has_dialog())
    dialogs.top()->ReinitialiseLayout();
#endif

  map.BringToBottom();
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
  map.reset();

  delete vario;
  vario = NULL;

  delete flarm;
  flarm = NULL;

  delete ta;
  ta = NULL;

  TopWindow::reset();
}

void
MainWindow::full_redraw()
{
  map.FullRedraw();
}

// Windows event handlers

bool
MainWindow::on_resize(unsigned width, unsigned height)
{
  SingleWindow::on_resize(width, height);

  Layout::Initialize(width, height);

  ReinitialiseLayout();

  if (map.defined()) {
    /* the map being created already is an indicator that XCSoar is
       running already, and so we assume the menu buttons have been
       created, too */

    ButtonLabel::Destroy();
    ButtonLabel::CreateButtonLabels(*this);
    ButtonLabel::SetFont(Fonts::MapBold);

    map.BringToBottom();
  }

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
    if (map.defined())
      map.set_focus();
    return true;
  }

  return SingleWindow::on_setfocus();
}

bool
MainWindow::on_timer(timer_t id)
{
  if (id != timer_id)
    return SingleWindow::on_timer(id);

  if (globalRunningEvent.test()) {
    ProcessTimer::Process();

    if (flarm != NULL)
      flarm->Update(CommonInterface::SettingsMap().EnableFLARMGauge,
                    CommonInterface::Basic(),
                    CommonInterface::SettingsComputer());

    if (ta != NULL)
      ta->Update(CommonInterface::SettingsMap().EnableTAGauge,
                 CommonInterface::Calculated().Heading,
                 CommonInterface::Calculated());
  }
  return true;
}

bool MainWindow::on_create(void)
{
  SingleWindow::on_create();

  timer_id = set_timer(1000, 500); // 2 times per second

  return true;
}

bool MainWindow::on_destroy(void) {
  kill_timer(timer_id);

  SingleWindow::on_destroy();

  return true;
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

  if (CustomView)
    /* target dialog is active */
    return;

  if (FullScreen)
    InfoBoxManager::Hide();
  else
    InfoBoxManager::Show();

  const PixelRect rc = FullScreen ? get_client_rect() : map_rect;
  map.fast_move(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
  // the repaint will be triggered by the DrawThread
}

void
MainWindow::SetCustomView(PixelRect rc)
{
  CustomView = true;

  InfoBoxManager::Hide();
  map.fast_move(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
}

void
MainWindow::LeaveCustomView()
{
  CustomView = false;

  /* quick hack to force SetFullScreen() to update the MapWindow */
  FullScreen = !FullScreen;
  SetFullScreen(!FullScreen);
}

#ifdef ANDROID

void
MainWindow::on_pause()
{
  if (!map.defined() && has_dialog())
    /* suspending before initialization has finished doesn't leave
       anything worth resuming, so let's just quit now */
    CancelDialog();

  SingleWindow::on_pause();
}

#endif /* ANDROID */
