/* ************************************************************************

	XCSoarLaunch
	main.c

	(C) Copyright 2001 By Tomoaki Nakashima. All right reserved.
		http://www.nakka.com/
		nakka@nakka.com


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

**************************************************************************/

/* ************************************************************************
	Include Files
**************************************************************************/

#include "Compatibility/gdi.h"

#include <stdbool.h>
#include <tchar.h>

#include <windows.h>

#include <todaycmn.h>
#include <aygshell.h>

#include "resource-launch.h"
#include "Compiler.h"

#if _WIN32_WCE >= 0x0420
#define HAVE_GESTURE
#endif

#ifdef HAVE_GESTURE
#define ENABLE_TOOLTIPS
#endif

/* ************************************************************************
	Define
**************************************************************************/

#ifdef HAVE_GESTURE

#ifndef SHRG_RETURNCMD
#define SHRG_RETURNCMD 1
#endif

#ifndef GN_CONTEXTMENU
#define GN_CONTEXTMENU 1000
#endif

#endif /* HAVE_GESTURE */

#define WINDOW_TITLE TEXT("XCSoarLaunch")
#define MAIN_WND_CLASS TEXT("XCSoarLaunchWndClass")
#define REG_PATH TEXT("Software\\OpenSource\\XCSoar")

#define BUF_SIZE 256

#if (WIN32_PLATFORM_PSPC <= 300)
#define USE_OPAQUE_FILL
#endif

/* ************************************************************************
	Global Variables
**************************************************************************/

static HINSTANCE hInst;

#ifdef ENABLE_TOOLTIPS
static HWND hToolTip;
#endif

static const unsigned IconSizeX = 112;
static const unsigned IconSizeY = 30;
static const unsigned WinLeftMargin = 8;
static const unsigned WinTopMargin = 2;
static const unsigned WinRightMargin = 2;
static const unsigned WinBottomMargin = 2;

static BOOL Refresh;

typedef struct _FILELIST {
  const TCHAR *CommandLine;
  const TCHAR *Description;
  HBITMAP bitmap;
  RECT rc;
} FILELIST;

static FILELIST FileList[2] = {
  {
    .CommandLine = _T("-fly"),
    .Description = _T("Start XCSoar in flight mode"),
  },
  {
    .CommandLine = _T("-simulator"),
    .Description = _T("Start XCSoar in simulator mode"),
  },
};

static const int FileListCnt = 2;
static int SelItem = -1;

/**
 * True when the mouse cursor is over the selected button while
 * dragging.
 */
static bool ButtonDown;

static bool
GetRegistryString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize)
{
  HKEY hKey;
  LONG hRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0, KEY_ALL_ACCESS,
                           &hKey);
  if (hRes != ERROR_SUCCESS) {
    RegCloseKey(hKey);
    return false;
  }

  DWORD dwType;
  hRes = RegQueryValueEx(hKey, szRegValue, 0, &dwType, (LPBYTE)pPos, &dwSize);
  RegCloseKey(hKey);
  return hRes == ERROR_SUCCESS && dwType == REG_SZ;
}

static void
CreateFileList(void)
{
  if (FileList[0].bitmap == NULL)
    FileList[0].bitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_XCSOARLSWIFT));

  if (FileList[1].bitmap == NULL)
    FileList[1].bitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_XCSOARLSWIFTSIM));
}

BOOL WINAPI
DllMain(HINSTANCE hModule, gcc_unused DWORD fdwReason,
        gcc_unused PVOID pvReserved)
{
  hInst = hModule;
  return TRUE;
}

#ifdef ENABLE_TOOLTIPS

static BOOL CALLBACK
ToolTipProc(HWND hDlg, UINT uMsg, gcc_unused WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  RECT rect;
  SIZE sz;
  HDC hdc;
  static TCHAR buf[BUF_SIZE];

  switch (uMsg) {
  case WM_INITDIALOG:
    break;

  case WM_LBUTTONDOWN:
    ShowWindow(hDlg, SW_HIDE);
    break;

  case WM_SETTEXT:
    if (lParam == 0)
      break;

    lstrcpy(buf, (TCHAR *)lParam);

    hdc = GetDC(hDlg);
    GetTextExtentPoint32(hdc, buf, lstrlen(buf), &sz);
    ReleaseDC(hDlg, hdc);

    SetWindowPos(hDlg, 0, 0, 0, sz.cx + 6, sz.cy + 6,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOACTIVATE);
    break;

  case WM_PAINT:
    hdc = BeginPaint(hDlg, &ps);

    GetClientRect(hDlg, (LPRECT)&rect);
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
    ExtTextOut(hdc, 2, 2, ETO_OPAQUE, NULL, buf, lstrlen(buf), NULL);

    EndPaint(hDlg, &ps);
    break;

  default:
    return FALSE;
  }

  return TRUE;
}

#endif /* ENABLE_TOOLTIPS */

static void
OnPaint(HWND hWnd, HDC hdc, PAINTSTRUCT *ps)
{
  RECT rect;
  GetClientRect(hWnd, (LPRECT)&rect);

  HDC drawdc = CreateCompatibleDC(hdc);
  HDC tempdc = CreateCompatibleDC(hdc);
  HBITMAP hDrawBitMap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
  HBITMAP hRetDrawBmp = SelectObject(drawdc, hDrawBitMap);

#ifdef USE_OPAQUE_FILL
  FillRect(drawdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
#endif

  TODAYDRAWWATERMARKINFO dwi;
  dwi.hdc = drawdc;
  GetClientRect(hWnd, &dwi.rc);
  dwi.hwnd = hWnd;
  SendMessage(GetParent(hWnd), TODAYM_DRAWWATERMARK, 0, (LPARAM)&dwi);

  for (int i = 0; i < FileListCnt; i++) {
    SelectObject(tempdc, FileList[i].bitmap);

    TransparentImage(drawdc,
                     FileList[i].rc.left, FileList[i].rc.top,
                     FileList[i].rc.right - FileList[i].rc.left,
                     FileList[i].rc.bottom - FileList[i].rc.top,
                     tempdc, 0, 0, IconSizeX, IconSizeY, RGB(0, 0, 255));
  }

  if (SelItem >= 0 && ButtonDown) {
    const int i = SelItem;

    HBITMAP neg_bmp = CreateCompatibleBitmap(hdc, IconSizeX, IconSizeY);
    HDC neg_dc = CreateCompatibleDC(hdc);
    SelectObject(neg_dc, neg_bmp);

    /* create an inverted bitmap */
    SelectObject(tempdc, FileList[i].bitmap);
    BitBlt(neg_dc, 0, 0, IconSizeX, IconSizeY,
           tempdc, 0, 0, NOTSRCCOPY);

    /* draw it (with inverted transparent color) */
    TransparentImage(drawdc,
                     FileList[i].rc.left, FileList[i].rc.top,
                     FileList[i].rc.right - FileList[i].rc.left,
                     FileList[i].rc.bottom - FileList[i].rc.top,
                     neg_dc, 0, 0, IconSizeX, IconSizeY, RGB(255, 255, 0));

    DeleteDC(neg_dc);
    DeleteObject(neg_bmp);
  }

  BitBlt(hdc, ps->rcPaint.left, ps->rcPaint.top, ps->rcPaint.right,
         ps->rcPaint.bottom, drawdc, ps->rcPaint.left, ps->rcPaint.top, SRCCOPY);

  SelectObject(drawdc, hRetDrawBmp);
  DeleteObject(hDrawBitMap);
  DeleteDC(drawdc);
  DeleteDC(tempdc);
}

static int
Point2Item(int px, int py)
{
  POINT pt;
  pt.x = px;
  pt.y = py;

  for (int i = 0; i < FileListCnt; i++)
    if (PtInRect(&FileList[i].rc, pt))
      return i;

  return -1;
}

static BOOL
LaunchXCSoar(HWND hWnd, const TCHAR *CommandLine)
{
  TCHAR FileName[BUF_SIZE];
  if (!GetRegistryString(TEXT("InstallDir"), FileName, BUF_SIZE - 16)) {
    MessageBox(hWnd, _T("XCSoar installation was not found"),
               _T("Error"), MB_OK);
    return false;
  }

  _tcscat(FileName, _T("\\XCSoar.exe"));

  TCHAR buffer[256];
  _sntprintf(buffer, 256, _T("\"%s\" %s"), FileName, CommandLine);

  PROCESS_INFORMATION pi;
  if (!CreateProcess(FileName, buffer, NULL, NULL, false,
                     0, NULL, NULL, NULL, &pi))
    return false;

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return true;
}

static LRESULT CALLBACK
WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static int is_running = 0;

  HDC hdc;
  PAINTSTRUCT ps;
#ifdef ENABLE_TOOLTIPS
  SHRGINFO rg;
#endif
  POINT pt;

  switch (msg) {
#ifdef ENABLE_TOOLTIPS
  case WM_CREATE:
    if (hToolTip == NULL)
      hToolTip = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG_TOOLTIP), NULL,
                              ToolTipProc);
    break;

  case WM_DESTROY:
    DestroyWindow(hToolTip);
    hToolTip = NULL;

    // FileListCnt = 0;
    return 0;
#endif

  case WM_TODAYCUSTOM_CLEARCACHE:
    break;

  case WM_TODAYCUSTOM_QUERYREFRESHCACHE:
    if (Refresh) {
      Refresh = FALSE;

      const unsigned screen_width = GetSystemMetrics(SM_CXSCREEN) -
        (WinLeftMargin + WinRightMargin);

      /* scale the buttons on large screens */
      unsigned scale = screen_width / (IconSizeX * 2);
      if (scale < 1)
        scale = 1;

      const unsigned top = WinTopMargin;
      const unsigned bottom = top + IconSizeY * scale;
      for (int x = WinLeftMargin, i = 0; i < FileListCnt; i++) {
        SetRect(&FileList[i].rc, x, top, x + IconSizeX * scale, bottom);
        x += IconSizeX * scale;
      }

      ((TODAYLISTITEM *)(wParam))->cyp = bottom + WinBottomMargin;
      return TRUE;
    }
    return FALSE;

  case WM_LBUTTONDOWN:
    SelItem = Point2Item(LOWORD(lParam), HIWORD(lParam));
    ButtonDown = true;
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
#ifdef ENABLE_TOOLTIPS
    rg.cbSize = sizeof(SHRGINFO);
    rg.hwndClient = hWnd;
    rg.ptDown.x = LOWORD(lParam);
    rg.ptDown.y = HIWORD(lParam);
    rg.dwFlags = SHRG_RETURNCMD;
    if (SelItem != -1 && SHRecognizeGesture(&rg) == GN_CONTEXTMENU) {
      RECT rect;
      RECT tip_rect;

      SendMessage(hToolTip, WM_SETTEXT, 0,
                  (LPARAM)(FileList + SelItem)->Description);
      GetWindowRect(hWnd, &rect);
      GetWindowRect(hToolTip, &tip_rect);

      tip_rect.left = rect.left + LOWORD(lParam) -
        (tip_rect.right - tip_rect.left) - 10;
      if (tip_rect.left < 0)
        tip_rect.left = 0;

      tip_rect.top = rect.top + HIWORD(lParam) -
        (tip_rect.bottom - tip_rect.top) - 10;
      if (tip_rect.top < 0)
        tip_rect.top = 0;

      SetWindowPos(hToolTip, HWND_TOPMOST, tip_rect.left, tip_rect.top,
                   0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }
#endif
    SetCapture(hWnd);
    break;

  case WM_LBUTTONUP:
#ifdef ENABLE_TOOLTIPS
    ShowWindow(hToolTip, SW_HIDE);
#endif

    ReleaseCapture();
    if (SelItem >= 0 && ButtonDown && !is_running) {
      is_running = 1;
      LaunchXCSoar(hWnd, FileList[SelItem].CommandLine);
      Sleep(1000);
      is_running = 0;
    }

    SelItem = -1;
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    break;

  case WM_MOUSEMOVE:
    if (SelItem >= 0) {
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      bool down = PtInRect(&FileList[SelItem].rc, pt);
      if (down != ButtonDown) {
        ButtonDown = down;
        InvalidateRect(hWnd, &FileList[SelItem].rc, false);
      }
    }

    break;

  case WM_PAINT:
    hdc = BeginPaint(hWnd, &ps);
    OnPaint(hWnd, hdc, &ps);
    EndPaint(hWnd, &ps);
    break;

  case WM_ERASEBKGND:
    return 1;

  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }

  return 0;
}

static HWND InitInstance(HWND pWnd, TODAYLISTITEM *ptli)
{
  hInst = ptli->hinstDLL;

  CreateFileList();

  WNDCLASS wc;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.hCursor = 0;
  wc.lpszMenuName = 0;
  wc.lpfnWndProc = (WNDPROC)WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = ptli->hinstDLL;
  wc.hIcon = NULL;
  wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wc.lpszClassName = MAIN_WND_CLASS;
  UnregisterClass(MAIN_WND_CLASS, ptli->hinstDLL);
  RegisterClass(&wc);

  Refresh = TRUE;

  return CreateWindow(MAIN_WND_CLASS, WINDOW_TITLE, WS_CHILD | WS_VISIBLE,
                      CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, pWnd, NULL,
                      ptli->hinstDLL, NULL);
}

gcc_unused
HWND APIENTRY InitializeCustomItem(TODAYLISTITEM *ptli, HWND pWnd)
{
  if (ptli->fEnabled == 0)
    return NULL;

  return InitInstance(pWnd, ptli);
}

gcc_unused
BOOL APIENTRY
CustomItemOptionsDlgProc(HWND hDlg, UINT uMsg, UINT wParam,
                         gcc_unused LONG lParam)
{
  SHINITDLGINFO shidi;
  // LVCOLUMN lvc;
  // LV_ITEM lvi;
  // int ItemIndex;
  // int i;

  switch (uMsg) {
  case WM_INITDIALOG:
    shidi.dwMask = SHIDIM_FLAGS;
    shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN |
      SHIDIF_SIZEDLGFULLSCREEN;
    shidi.hDlg = hDlg;
    SHInitDialog(&shidi);

    SetWindowText(hDlg, TEXT("XCSoarLaunch"));
    break;

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDC_BUTTON_UNINSTALL:
      if (MessageBox(hDlg, TEXT("Delete today information?"),
                     TEXT("Uninstall"), MB_ICONSTOP | MB_YESNO) == IDYES) {
        RegDeleteKey(HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\Today\\Items\\XCSoarLaunch"));
        MessageBox(hDlg, TEXT("Information was deleted. Please uninstall."),
                   TEXT("Info"), MB_OK | MB_ICONINFORMATION);
      }

      EndDialog(hDlg, IDOK);
      break;

    case IDOK:
      EndDialog(hDlg, IDOK);
      break;
    }
    break;

  default:
    return FALSE;
  }

  return TRUE;
}
