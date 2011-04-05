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
#include "Gauge/GaugeVario.hpp"
#include "MenuBar.hpp"
#include "Appearance.hpp"

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
  RECT rc = get_client_rect();

  Layout::Initialize(rc.right - rc.left, rc.bottom - rc.top);

  // color/pattern chart (must have infobox geometry before this)
  Graphics::Initialise();

  LogStartUp(_T("Initialise fonts"));
  Fonts::Initialize();
}

void
MainWindow::InitialiseConfigured()
{
  RECT rc = get_client_rect();

  LogStartUp(_T("InfoBox geometry"));
  InfoBoxLayout::Init(rc);
  map_rect = InfoBoxLayout::GetRemainingRect(rc);

  Fonts::SizeInfoboxFont();

  if (Appearance.UseCustomFonts) {
    LogStartUp(_T("Load fonts"));
    Fonts::LoadCustom();
  }

  LogStartUp(_T("Create info boxes"));
  InfoBoxManager::Create(rc);

  LogStartUp(_T("Create button labels"));
  ButtonLabel::CreateButtonLabels(*this);
  ButtonLabel::SetLabelText(0,_T("MODE"));
  ButtonLabel::SetFont(Fonts::Map);

  WindowStyle hidden;
  hidden.hide();

  vario = new GaugeVario(*this,
                         rc.right - InfoBoxLayout::ControlWidth, 0,
                         InfoBoxLayout::ControlWidth,
                         InfoBoxLayout::ControlHeight * 3,
                         hidden);

  WindowStyle hidden_border;
  hidden_border.hide();
  hidden_border.border();

  flarm = new GaugeFLARM(*this,
                         rc.right - InfoBoxLayout::ControlWidth * 2 + 1,
                         rc.bottom - InfoBoxLayout::ControlHeight * 2 + 1,
                         InfoBoxLayout::ControlWidth * 2 - 1,
                         InfoBoxLayout::ControlHeight * 2 - 1,
                         hidden_border);
  flarm->bring_to_top();

  unsigned sz = std::min(InfoBoxLayout::ControlHeight,
                         InfoBoxLayout::ControlWidth) * 2;

  ta = new GaugeThermalAssistant(*this, 0, rc.bottom - sz, sz, sz,
                                 hidden_border);
  ta->bring_to_top();

  map.set(*this, map_rect);
  map.set_font(Fonts::Map);

  LogStartUp(_T("Initialise message system"));
  popup.set(rc);
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

// Windows event handlers

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
                 CommonInterface::Basic().Heading,
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

  const RECT rc = FullScreen ? get_client_rect() : map_rect;
  map.fast_move(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
  // the repaint will be triggered by the DrawThread
}

void
MainWindow::SetCustomView(RECT rc)
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
