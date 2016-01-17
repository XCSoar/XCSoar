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

#include "ShowMenuButton.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Layout.hpp"
#include "Input/InputEvents.hpp"
#include "Util/Macros.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

class ShowMenuButtonRenderer : public ButtonRenderer {
public:
  unsigned GetMinimumButtonWidth() const override {
    return Layout::GetMinimumControlHeight();
  }

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  bool enabled, bool focused, bool pressed) const override;
};

void
ShowMenuButton::Create(ContainerWindow &parent, const PixelRect &rc,
                       WindowStyle style)
{
  Button::Create(parent, rc, style, new ShowMenuButtonRenderer());
}

bool
ShowMenuButton::OnClicked()
{
  InputEvents::ShowMenu();
  return true;
}

void
ShowMenuButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                                   bool enabled, bool focused,
                                   bool pressed) const
{
  const unsigned pen_width = Layout::ScalePenWidth(2);
  const unsigned padding = Layout::GetTextPadding() + pen_width;

  canvas.Select(Pen(pen_width, COLOR_BLACK));
  canvas.DrawRoundRectangle(rc.left, rc.top, rc.right - 1, rc.bottom - 1,
                            Layout::VptScale(8), Layout::VptScale(8));

  const BulkPixelPoint m[] = {
    BulkPixelPoint(rc.left + padding, rc.bottom - padding),
    BulkPixelPoint(rc.left + padding, rc.top + padding),
    BulkPixelPoint((rc.left + rc.right) / 2, rc.bottom - 2 * padding),
    BulkPixelPoint(rc.right - padding, rc.top + padding),
    BulkPixelPoint(rc.right - padding, rc.bottom - padding),
  };

  canvas.DrawPolyline(m, ARRAY_SIZE(m));

  if (pressed) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    canvas.DrawFilledRectangle(rc, COLOR_YELLOW.WithAlpha(80));
#else
    canvas.InvertRectangle(rc);
#endif
  }
}
