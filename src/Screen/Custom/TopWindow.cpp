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

#include "Screen/TopWindow.hpp"
#include "Screen/Custom/TopCanvas.hpp"

TopWindow::~TopWindow()
{
  delete screen;
}

bool
TopWindow::find(const TCHAR *cls, const TCHAR *text)
{
  return false; // XXX
}

void
TopWindow::Create(const TCHAR *cls, const TCHAR *text, PixelRect rc,
                  TopWindowStyle style)
{
  invalidated.store(true, std::memory_order_relaxed);

  rc.right -= rc.left;
  rc.bottom -= rc.top;
  rc.left = rc.top = 0;

  delete screen;
  screen = new TopCanvas();
  screen->Create(rc.right, rc.bottom,
                 style.GetFullScreen(), style.GetResizable());

  ContainerWindow::Create(NULL, rc, style);

  SetCaption(text);
}

void
TopWindow::CancelMode()
{
  OnCancelMode();
}

void
TopWindow::Fullscreen()
{
  screen->Fullscreen();
}

void
TopWindow::Expose()
{
  OnPaint(*screen);
  screen->Flip();
}

void
TopWindow::Refresh()
{
  if (!CheckResumeSurface())
    /* the application is paused/suspended, and we don't have an
       OpenGL surface - ignore all drawing requests */
    return;

  if (!invalidated.exchange(false, std::memory_order_relaxed))
    return;

  Expose();
}

bool
TopWindow::OnActivate()
{
  return false;
}

bool
TopWindow::OnDeactivate()
{
  return false;
}

bool
TopWindow::OnClose()
{
  Destroy();
  return true;
}
