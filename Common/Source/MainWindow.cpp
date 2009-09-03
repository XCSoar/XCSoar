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

bool
MainWindow::register_class(HINSTANCE hInstance, const TCHAR* szWindowClass)
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
  wc.lpszClassName              = szWindowClass;

  return (RegisterClass(&wc)!= FALSE);
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
    AfterStartup();
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
  if (CheckShutdown()) {
    if(_timer_id) {
      ::KillTimer(hWnd, _timer_id);
      _timer_id = 0;
    }
    Shutdown();
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
