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

#include "Screen/TopWindow.hpp"
#include "Event/Globals.hpp"
#include "Event/Windows/Event.hpp"
#include "Event/Windows/Loop.hpp"
#include "Event/Queue.hpp"

void
TopWindow::Create(const TCHAR *cls, const TCHAR *text, PixelSize size,
                  TopWindowStyle style)
{
  hSavedFocus = nullptr;

  const PixelPoint position(CW_USEDEFAULT, CW_USEDEFAULT);
  Window::Create(nullptr, cls, text, PixelRect(position, size), style);
}

void
TopWindow::CancelMode()
{
  HWND focus = ::GetFocus();
  if (focus != nullptr)
    ::SendMessage(focus, WM_CANCELMODE, 0, 0);
}


void
TopWindow::Refresh()
{
  EventQueue::HandlePaintMessages();
}

bool
TopWindow::OnActivate()
{
  if (hSavedFocus != nullptr && ::IsWindow(hSavedFocus) &&
      ::IsWindowVisible(hSavedFocus) && ::IsWindowEnabled(hSavedFocus)) {
    /* restore the keyboard focus to the control which was previously
       focused */
    ::SetFocus(hSavedFocus);
    return true;
  }

  return false;
}

bool
TopWindow::OnDeactivate()
{
  /* remember the currently focused control */
  hSavedFocus = ::GetFocus();
  if (hSavedFocus != nullptr && !IdentifyDescendant(hSavedFocus))
    hSavedFocus = nullptr;

  return false;
}

bool
TopWindow::OnClose()
{
  return false;
}

LRESULT
TopWindow::OnMessage(HWND _hWnd, UINT message,
                      WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_CLOSE:
    if (OnClose())
      /* true returned: message was handled */
      return 0;
    break;

  case WM_ACTIVATE:
    if (wParam == WA_INACTIVE ? OnDeactivate() : OnActivate())
      return true;
    break;
  }
  return ContainerWindow::OnMessage(_hWnd, message, wParam, lParam);
}

int
TopWindow::RunEventLoop()
{
  EventLoop loop(*event_queue);
  Event event;
  while (loop.Get(event))
    loop.Dispatch(event);

  return event.msg.wParam;
}

void
TopWindow::PostQuit()
{
  ::PostQuitMessage(0);
}
