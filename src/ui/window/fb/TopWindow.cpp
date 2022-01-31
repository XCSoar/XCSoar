/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "../TopWindow.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/Canvas.hpp"

#ifdef KOBO
#include "ui/canvas/Canvas.hpp"
#endif

namespace UI {

#ifdef USE_FB

void
TopWindow::CheckResize() noexcept
{
  assert(screen != nullptr);

  if (screen->CheckResize())
    Resize(screen->GetSize());
}

#endif

#ifdef KOBO
void
TopWindow::OnDestroy()
{
  /* clear the screen before exiting XCSoar */
  Canvas canvas = screen->Lock();
  if (canvas.IsDefined()) {
    canvas.Clear(COLOR_BLACK);
    screen->Flip();
    screen->Wait();

    canvas.ClearWhite();
    screen->Unlock();
    screen->Flip();
  }

  ContainerWindow::OnDestroy();
}
#endif

} // namespace UI
