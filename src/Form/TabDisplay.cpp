/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Form/TabDisplay.hpp"
#include "Widget/TabWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Key.h"
#include "Screen/Bitmap.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Scope.hpp"

#ifdef USE_GLSL
#include "Screen/OpenGL/Shaders.hpp"
#include "Screen/OpenGL/Program.hpp"
#else
#include "Screen/OpenGL/Compatibility.hpp"
#endif
#endif

#include <assert.h>
#include <winuser.h>

static constexpr unsigned TabLineHeightInitUnscaled = 5;

TabDisplay::TabDisplay(TabWidget &_pager, const DialogLook &_look,
                       ContainerWindow &parent, PixelRect rc,
                       bool _vertical,
                       WindowStyle style)
  :pager(_pager),
   look(_look),
   vertical(_vertical),
   dragging(false),
   tab_line_height(vertical
                   ? (Layout::Scale(TabLineHeightInitUnscaled) * 0.75)
                   : Layout::Scale(TabLineHeightInitUnscaled))
{
  style.TabStop();
  Create(parent, rc, style);
}

TabDisplay::~TabDisplay()
{
  for (const auto i : buttons)
    delete i;
}

inline unsigned
TabButton::GetRecommendedWidth(const DialogLook &look) const
{
  if (bitmap != nullptr) {
    unsigned w = bitmap->GetWidth();
#ifndef ENABLE_OPENGL
    /* second half is the mask */
    w /= 2;
#endif
    return w;
  }

  return look.button.font->TextSize(caption).cx + 2 * Layout::GetTextPadding();
}

inline unsigned
TabButton::GetRecommendedHeight() const
{
  if (bitmap != nullptr)
    return bitmap->GetHeight() + 2 * Layout::GetTextPadding();

  return 0;
}

unsigned
TabDisplay::GetRecommendedColumnWidth() const
{
  unsigned width = Layout::GetMaximumControlHeight();
  for (auto *i : buttons) {
    unsigned w = i->GetRecommendedWidth(GetLook()) + tab_line_height;
    if (w > width)
      width = w;
  }

  return width;
}

unsigned
TabDisplay::GetRecommendedRowHeight() const
{
  unsigned height = Layout::GetMaximumControlHeight();
  for (auto *i : buttons) {
    unsigned h = i->GetRecommendedHeight() + tab_line_height;
    if (h > height)
      height = h;
  }

  return height;
}

void
TabDisplay::UpdateLayout(const PixelRect &rc, bool _vertical)
{
  vertical = _vertical;
  Move(rc);
}

const PixelRect &
TabDisplay::GetButtonSize(unsigned i) const
{
  assert(i < GetSize());

  if (buttons[i]->rc.left < buttons[i]->rc.right)
    return buttons[i]->rc;

  const unsigned margin = 1;

  /*
  const bool partialTab = vertial
    ? tab_display->GetTabHeight() < GetHeight()
    : tab_display->GetTabWidth() < GetWidth();
  */

  const unsigned finalmargin = 1; //partialTab ? tab_line_height - 1 * margin : margin;
  // Todo make the final margin display on either beginning or end of tab bar
  // depending on position of tab bar

  PixelRect rc;

  if (vertical) {
    const unsigned but_height =
       (GetHeight() - finalmargin) / GetSize() - margin;

    rc.left = 0;
    rc.right = GetWidth() - tab_line_height;

    rc.top = finalmargin + (margin + but_height) * i;
    rc.bottom = rc.top + but_height;

  } else {
    const unsigned portraitRows = (GetSize() > 4) ? 2 : 1;

    const unsigned portraitColumnsRow0 = ((portraitRows == 1)
       ? GetSize() : GetSize() / 2);
    const unsigned portraitColumnsRow1 = ((portraitRows == 1)
       ? 0 : GetSize() - GetSize() / 2);

    const unsigned row = (i > (portraitColumnsRow0 - 1)) ? 1 : 0;

    const unsigned rowheight = (GetHeight() - tab_line_height)
        / portraitRows - margin;

    const unsigned but_width =
          (GetWidth() - finalmargin) /
          ((row == 0) ? portraitColumnsRow0 : portraitColumnsRow1) - margin;

    rc.top = row * (rowheight + margin);
    rc.bottom = rc.top + rowheight;

    rc.left = finalmargin + (margin + but_width) * (row ? (i - portraitColumnsRow0) : i);
    rc.right = rc.left + but_width;
  }

  buttons[i]->rc = rc;
  return buttons[i]->rc;
}

void
TabDisplay::PaintButton(Canvas &canvas, unsigned CaptionStyle,
                        const TCHAR *caption, const PixelRect &rc,
                        const Bitmap *bmp, const bool isDown, bool inverse)
{
  PixelRect rcTextFinal = rc;
  const unsigned buttonheight = rc.bottom - rc.top;
  const PixelSize text_size = canvas.CalcTextSize(caption);
  const int textwidth = text_size.cx;
  const int textheight = text_size.cy;
  unsigned textheightoffset = 0;

  if (textwidth > (rc.right - rc.left)) // assume 2 lines
    textheightoffset = std::max(0, (int)(buttonheight - textheight * 2) / 2);
  else
    textheightoffset = std::max(0, (int)(buttonheight - textheight) / 2);

  rcTextFinal.top += textheightoffset;

  canvas.DrawFilledRectangle(rc, canvas.GetBackgroundColor());

  if (bmp != nullptr) {
    const PixelSize bitmap_size = bmp->GetSize();
    const int offsetx = (rc.right - rc.left - bitmap_size.cx / 2) / 2;
    const int offsety = (rc.bottom - rc.top - bitmap_size.cy) / 2;

#ifdef ENABLE_OPENGL

#ifdef USE_GLSL
    if (inverse)
      OpenGL::invert_shader->Use();
    else
      OpenGL::texture_shader->Use();
#else
    const GLEnable<GL_TEXTURE_2D> scope;

    if (inverse) {
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

      /* invert the texture color */
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_ONE_MINUS_SRC_COLOR);

      /* copy the texture alpha */
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
    } else
      /* simple copy */
      OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif

    const GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLTexture &texture = *bmp->GetNative();
    texture.Bind();
    texture.Draw(rc.left + offsetx, rc.top + offsety);

#else
    if (inverse) // black background
      canvas.CopyNotOr(rc.left + offsetx,
                       rc.top + offsety,
                       bitmap_size.cx / 2,
                       bitmap_size.cy,
                       *bmp,
                       bitmap_size.cx / 2, 0);

    else
      canvas.CopyAnd(rc.left + offsetx,
                      rc.top + offsety,
                      bitmap_size.cx / 2,
                      bitmap_size.cy,
                      *bmp,
                      bitmap_size.cx / 2, 0);
#endif

  } else {
#ifndef USE_GDI
    if (IsDithered())
      CaptionStyle |= DT_UNDERLINE;
#endif

    canvas.DrawFormattedText(&rcTextFinal, caption, CaptionStyle);
  }
}

void
TabDisplay::Add(const TCHAR *caption, const Bitmap *bmp)
{
  TabButton *b = new TabButton(caption, bmp);
  buttons.append(b);
}

int
TabDisplay::GetButtonIndexAt(RasterPoint p) const
{
  for (unsigned i = 0; i < GetSize(); i++) {
    const PixelRect &rc = GetButtonSize(i);
    if (rc.IsInside(p))
      return i;
  }

  return -1;
}

void
TabDisplay::OnResize(PixelSize new_size)
{
  PaintWindow::OnResize(new_size);

  for (auto button : buttons)
    button->InvalidateLayout();
}

void
TabDisplay::OnPaint(Canvas &canvas)
{
  canvas.Clear(COLOR_BLACK);
  canvas.Select(*look.button.font);

  const unsigned CaptionStyle = DT_CENTER | DT_NOCLIP
      | DT_WORDBREAK;

  const bool is_focused = !HasCursorKeys() || HasFocus();
  for (unsigned i = 0; i < buttons.size(); i++) {
    const TabButton &button = *buttons[i];

    const bool is_down = dragging && i == down_index && !drag_off_button;
    const bool is_selected = i == pager.GetCurrentIndex();

    canvas.SetTextColor(look.list.GetTextColor(is_selected, is_focused,
                                               is_down));
    canvas.SetBackgroundColor(look.list.GetBackgroundColor(is_selected,
                                                           is_focused,
                                                           is_down));

    const PixelRect &rc = GetButtonSize(i);
    PaintButton(canvas, CaptionStyle, button.caption, rc, button.bitmap,
                is_down, is_selected);
  }
}

void
TabDisplay::OnKillFocus()
{
  Invalidate();
  PaintWindow::OnKillFocus();
}

void
TabDisplay::OnSetFocus()
{
  Invalidate();
  PaintWindow::OnSetFocus();
}

void
TabDisplay::OnCancelMode()
{
  PaintWindow::OnCancelMode();
  EndDrag();
}

bool
TabDisplay::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {

  case KEY_APP1:
  case KEY_APP2:
  case KEY_APP3:
  case KEY_APP4:
    return true;

  case KEY_RETURN:
    return true;

  case KEY_LEFT:
    return pager.GetCurrentIndex() > 0;

  case KEY_RIGHT:
    return pager.GetCurrentIndex() < GetSize() - 1;

  case KEY_DOWN:
    return false;

  case KEY_UP:
    return false;

  default:
    return false;
  }
}


bool
TabDisplay::OnKeyDown(unsigned key_code)
{
  switch (key_code) {

  case KEY_APP1:
    if (GetSize() > 0)
      pager.ClickPage(0);
    return true;

  case KEY_APP2:
    if (GetSize() > 1)
      pager.ClickPage(1);
    return true;

  case KEY_APP3:
    if (GetSize() > 2)
      pager.ClickPage(2);
    return true;

  case KEY_APP4:
    if (GetSize() > 3)
      pager.ClickPage(3);
    return true;

  case KEY_RETURN:
    pager.ClickPage(pager.GetCurrentIndex());
    return true;

  case KEY_DOWN:
    break;

  case KEY_RIGHT:
    pager.NextPage();
    return true;

  case KEY_UP:
    break;

  case KEY_LEFT:
    pager.PreviousPage();
    return true;
  }
  return PaintWindow::OnKeyDown(key_code);
}

bool
TabDisplay::OnMouseDown(PixelScalar x, PixelScalar y)
{
  EndDrag();

  // If possible -> Give focus to the Control
  SetFocus();

  int i = GetButtonIndexAt({ x, y });
  if (i >= 0) {
    dragging = true;
    drag_off_button = false;
    down_index = i;
    SetCapture();
    Invalidate();
    return true;
  }

  return PaintWindow::OnMouseDown(x, y);
}

bool
TabDisplay::OnMouseUp(PixelScalar x, PixelScalar y)
{
  if (dragging) {
    EndDrag();

    if (!drag_off_button)
      pager.ClickPage(down_index);

    return true;
  } else {
    return PaintWindow::OnMouseUp(x, y);
  }
}

bool
TabDisplay::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  if (!dragging)
    return false;

  const PixelRect rc = GetButtonSize(down_index);

  bool not_on_button = !rc.IsInside({ x, y });
  if (drag_off_button != not_on_button) {
    drag_off_button = not_on_button;
    Invalidate(rc);
  }
  return true;
}

void
TabDisplay::EndDrag()
{
  if (dragging) {
    dragging = false;
    ReleaseCapture();
    Invalidate();
  }
}
