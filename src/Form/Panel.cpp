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

#include "Form/Panel.hpp"
#include "Look/DialogLook.hpp"

#ifdef HAVE_CLIPPING
#include "Screen/Canvas.hpp"
#endif

PanelControl::PanelControl(ContainerWindow &parent, const DialogLook &look,
                           const PixelRect &rc,
                           const WindowStyle style)
#ifdef HAVE_CLIPPING
  :background_color(look.background_color)
#endif
{
  Create(parent, rc, style);
}

/* don't need to erase the background when it has been done by the
   parent window already */
#ifdef HAVE_CLIPPING

void
PanelControl::OnPaint(Canvas &canvas)
{
  canvas.Clear(background_color);

  ContainerWindow::OnPaint(canvas);
}

#endif
