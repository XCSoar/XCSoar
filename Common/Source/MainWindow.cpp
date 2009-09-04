#include "MainWindow.hpp"
#include "resource.h"
#include "Protection.hpp"
#include "InfoBoxManager.h"
#include "Message.h"
#include "Interface.hpp"
#include "ButtonLabel.h"
#include "Screen/Graphics.hpp"
#include "Components.hpp"
#include "ProcessTimer.hpp"
#include "LogFile.hpp"
#include "InfoBoxLayout.h"
#include "Screen/Fonts.hpp"

#if WINDOWSPC > 0
#include "Asset.hpp" /* for SCREENWIDTH and SCREENHEIGHT */
#endif

bool
MainWindow::register_class(HINSTANCE hInstance)
{
  WNDCLASS wc;

  wc.style                      = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = Window::WndProc;
  wc.cbClsExtra                 = 0;
#if (WINDOWSPC>0)
  wc.cbWndExtra = 0;
#else
  WNDCLASS dc;
  GetClassInfo(hInstance,TEXT("DIALOG"),&dc);
  wc.cbWndExtra                 = dc.cbWndExtra ;
#endif
  wc.hInstance                  = hInstance;
#if defined(GNAV) && !defined(PCGNAV)
  wc.hIcon = NULL;
#else
  wc.hIcon                      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOARSWIFT));
#endif
  wc.hCursor                    = 0;
  wc.hbrBackground              = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName               = 0;
  wc.lpszClassName = _T("XCSoarMain");

  return (RegisterClass(&wc)!= FALSE);
}

void
MainWindow::set(LPCTSTR text,
                int left, int top, unsigned width, unsigned height)
{
  TopWindow::set(_T("XCSoarMain"), text, left, top, width, height);

  RECT rc;
#if WINDOWSPC > 0
  rc.left = 0;
  rc.right = SCREENWIDTH;
  rc.top = 0;
  rc.bottom = SCREENHEIGHT;
#else
  rc = get_client_rect();
#endif

  StartupStore(TEXT("InfoBox geometry\n"));
  InfoBoxLayout::ScreenGeometry(rc);

  // color/pattern chart (must have infobox geometry before this)
  MapGfx.Initialise(XCSoarInterface::hInst);

  StartupStore(TEXT("Create info boxes\n"));
  RECT rcsmall = InfoBoxManager::Create(rc);

  StartupStore(TEXT("Create button labels\n"));
  ButtonLabel::CreateButtonLabels(rc);
  ButtonLabel::SetLabelText(0,TEXT("MODE"));

  StartupStore(TEXT("Initialise fonts\n"));
  InitialiseFonts(*this, rc);

  map.set(XCSoarInterface::main_window, rcsmall, rc);
  map.set_font(MapWindowFont);
  map.SetMapRect(rcsmall);
}

///////////////////////////////////////////////////////////////////////////
// Windows event handlers

bool
MainWindow::on_command(HWND wmControl, unsigned id, unsigned code)
{
  if (wmControl && globalRunningEvent.test()) {

    full_screen();

    if (InfoBoxManager::Click(wmControl)) {
      return true; // don't continue processing..
    }

    Message::CheckTouch(wmControl);
    
    if (ButtonLabel::CheckButtonPress(wmControl)) {
      return true; // don't continue processing..
    }
  }

  return TopWindow::on_command(wmControl, id, code);
}

LRESULT MainWindow::on_colour(HDC hdc, int wdata)
{
  switch(wdata) {
  case 0:
    SetBkColor(hdc, MapGfx.ColorUnselected);
    SetTextColor(hdc, MapGfx.ColorBlack);
    return (LRESULT)MapGfx.infoUnselectedBrush.native();
  case 1:
    SetBkColor(hdc, MapGfx.ColorSelected);
    SetTextColor(hdc, MapGfx.ColorBlack);
    return (LRESULT)MapGfx.infoSelectedBrush.native();
  case 2:
    SetBkColor(hdc, MapGfx.ColorUnselected);
    SetTextColor(hdc, MapGfx.ColorWarning);
    return (LRESULT)MapGfx.infoUnselectedBrush.native();
  case 3:
    SetBkColor(hdc, MapGfx.ColorUnselected);
    SetTextColor(hdc, MapGfx.ColorOK);
    return (LRESULT)MapGfx.infoUnselectedBrush.native();
  case 4:
    // black on light green
    SetBkColor(hdc, MapGfx.ColorButton);
    SetTextColor(hdc, MapGfx.ColorBlack);
    return (LRESULT)MapGfx.buttonBrush.native();
  case 5:
    // grey on light green
    SetBkColor(hdc, MapGfx.ColorButton);
    SetTextColor(hdc, MapGfx.ColorMidGrey);
    return (LRESULT)MapGfx.buttonBrush.native();
  }
}

bool MainWindow::on_timer(void)
{
  if (globalRunningEvent.test()) {
    XCSoarInterface::AfterStartup();
#ifdef _SIM_
    SIMProcessTimer();
#else
    ProcessTimer();
#endif
  }
  return true;
}

bool MainWindow::on_create(void)
{
  TopWindow::on_create();

  if (_timer_id == 0) {
    _timer_id = SetTimer(hWnd,1000,500,NULL); // 2 times per second
  }
  return true;
}

void MainWindow::install_timer(void) {
}

bool MainWindow::on_destroy(void) {
  TopWindow::on_destroy();

  PostQuitMessage(0);
  return true;
}

bool MainWindow::on_close() {
  if (XCSoarInterface::CheckShutdown()) {
    if(_timer_id) {
      ::KillTimer(hWnd, _timer_id);
      _timer_id = 0;
    }
    XCSoarInterface::Shutdown();
  }
  return true;
}


LRESULT MainWindow::on_message(HWND _hWnd, UINT message,
			       WPARAM wParam, LPARAM lParam) {
  switch (message) {
    /*
  case WM_CTLCOLORSTATIC:
    return on_colour((HDC)wParam, get_userdata((HWND)lParam));
    break;
    */
  case WM_SETFOCUS:
    InfoBoxManager::Focus();
    break;
  case WM_ACTIVATE:
    if(LOWORD(wParam) != WA_INACTIVE) {
      full_screen();
    }
    break;
  case WM_TIMER:
    if (on_timer()) return 0;
    break;
  };

  return TopWindow::on_message(_hWnd, message, wParam, lParam);
}
