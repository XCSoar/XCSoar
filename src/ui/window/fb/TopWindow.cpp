// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
TopWindow::OnDestroy() noexcept
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
