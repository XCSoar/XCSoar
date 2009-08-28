#include "Screen/Window.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Interface.hpp"

#ifdef PNA
#include "Appearance.hpp" // for GlobalModelType
#include "Asset.hpp" // for MODELTYPE_*
#endif

#include <assert.h>

void
Window::set(ContainerWindow *parent, LPCTSTR cls, LPCTSTR text,
            unsigned left, unsigned top,
            unsigned width, unsigned height,
            DWORD style, DWORD ex_style)
{
  hWnd = ::CreateWindowEx(ex_style, cls, text, style,
                          left, top, width, height,
                          parent != NULL ? parent->hWnd : NULL,
                          NULL, hInst, NULL);
}

void
Window::set(ContainerWindow *parent, LPCTSTR cls, LPCTSTR text,
            unsigned left, unsigned top,
            unsigned width, unsigned height,
            bool center, bool notify, bool show,
            bool tabstop, bool border)
{
  DWORD ex_style = 0;
  DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

  if (parent == NULL)
    style |= WS_POPUP;
  else
    style |= WS_CHILD;

  if (show)
    style |= WS_VISIBLE;

  if (center)
    style |= SS_CENTER;

  if (notify)
    style |= SS_NOTIFY;

  if (tabstop)
    style |= WS_TABSTOP;

  if (border) {
    style |= WS_BORDER;

#ifdef PNA // VENTA3 FIX  better borders
    if (GlobalModelType == MODELTYPE_PNA_HP31X) {
      ex_style |= WS_EX_CLIENTEDGE;
      style |= WS_THICKFRAME;
    }
#endif
  }

  set(parent, cls, text, left, top, width, height, style, ex_style);
}

void
Window::created(HWND _hWnd)
{
  assert(hWnd == NULL);
  hWnd = _hWnd;
}

void
Window::reset()
{
  DestroyWindow(hWnd);
}
