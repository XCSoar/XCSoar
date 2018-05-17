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

#include "ButtonWidget.hpp"
#include "Form/Button.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "Renderer/TextButtonRenderer.hpp"
#include "Screen/Layout.hpp"

ButtonWidget::ButtonWidget(const ButtonLook &look, const TCHAR *caption,
                           ActionListener &_listener, int _id)
  :renderer(new TextButtonRenderer(look, caption)),
   listener(_listener), id(_id) {}

ButtonWidget::~ButtonWidget()
{
  if (IsDefined())
    DeleteWindow();
  else
    delete renderer;
}

void
ButtonWidget::Invalidate()
{
  assert(IsDefined());

  ((Button &)GetWindow()).Invalidate();
}

PixelSize
ButtonWidget::GetMinimumSize() const
{
  return PixelSize(renderer->GetMinimumButtonWidth(),
                   Layout::GetMinimumControlHeight());
}

PixelSize
ButtonWidget::GetMaximumSize() const
{
  return PixelSize(renderer->GetMinimumButtonWidth() + Layout::GetMaximumControlHeight(),
                   Layout::GetMaximumControlHeight());
}

void
ButtonWidget::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  WindowStyle style;
  style.Hide();
  style.TabStop();

  SetWindow(new Button(parent, rc, style, renderer, listener, id));
}

bool
ButtonWidget::SetFocus()
{
  GetWindow().SetFocus();
  return true;
}
