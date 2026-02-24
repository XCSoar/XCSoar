// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TabDisplay.hpp"
#include "TabHandler.hpp"
#include "Renderer/TabRenderer.hpp"
#include "Look/DialogLook.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/canvas/Icon.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Features.hpp"
#include "Screen/Layout.hpp"
#include "util/StaticString.hxx"
#include "Asset.hpp"

#include <algorithm>

/**
 * Holds display and callbacks data for a single tab.
 */
class TabDisplay::Button {
  TabRenderer renderer;

public:
  StaticString<32> caption;
  const MaskedIcon *icon;
  PixelRect rc;

public:
  Button(const char *_caption, const MaskedIcon *_icon) noexcept
    :icon(_icon)
  {
    caption = _caption;
  };

  void InvalidateLayout() {
    renderer.InvalidateLayout();
  }

  [[gnu::pure]]
  unsigned GetRecommendedWidth(const DialogLook &look) const noexcept;

  [[gnu::pure]]
  unsigned GetRecommendedHeight(const DialogLook &look) const noexcept;

  /**
   * Paints one button
   */
  void Draw(Canvas &canvas, const DialogLook &look,
            bool focused, bool pressed, bool selected) const noexcept {
    renderer.Draw(canvas, rc, look, caption, icon, focused, pressed, selected);
  }
};

TabDisplay::TabDisplay(TabHandler &_handler, const DialogLook &_look,
                       ContainerWindow &parent, PixelRect rc,
                       bool _vertical,
                       WindowStyle style) noexcept
  :handler(_handler),
   look(_look),
   tab_line_height(Layout::VptScale(5)),
   vertical(_vertical)
{
  style.TabStop();
  Create(parent, rc, style);
}

TabDisplay::~TabDisplay() noexcept
{
  for (const auto i : buttons)
    delete i;
}

inline unsigned
TabDisplay::Button::GetRecommendedWidth(const DialogLook &look) const noexcept
{
  if (icon != nullptr)
    return icon->GetSize().width + 2 * Layout::GetTextPadding();

  return look.button.font->TextSize(caption).width + 2 * Layout::GetTextPadding();
}

inline unsigned
TabDisplay::Button::GetRecommendedHeight(const DialogLook &look) const noexcept
{
  if (icon != nullptr)
    return icon->GetSize().height + 2 * Layout::GetTextPadding();

  return look.button.font->GetHeight() + 2 * Layout::GetTextPadding();
}

unsigned
TabDisplay::GetRecommendedColumnWidth() const noexcept
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
TabDisplay::GetRecommendedRowHeight() const noexcept
{
  unsigned height = Layout::GetMaximumControlHeight();
  for (auto *i : buttons) {
    unsigned h = i->GetRecommendedHeight(GetLook()) + tab_line_height;
    if (h > height)
      height = h;
  }

  const unsigned portraitRows = buttons.size() > 4 ? 2 : 1;
  return height * portraitRows;
}

void
TabDisplay::UpdateLayout(const PixelRect &rc, bool _vertical) noexcept
{
  vertical = _vertical;
  Move(rc);
}

void
TabDisplay::CalculateLayout() noexcept
{
  if (buttons.empty() || !IsDefined())
    return;

  const unsigned margin = 1;

  /*
  const bool partialTab = vertial
    ? tab_display->GetTabHeight() < GetHeight()
    : tab_display->GetTabWidth() < GetWidth();
  */

  const unsigned finalmargin = 1; //partialTab ? tab_line_height - 1 * margin : margin;
  // Todo make the final margin display on either beginning or end of tab bar
  // depending on position of tab bar

  const auto window_size = Window::GetSize();

  if (vertical) {
    const unsigned n = buttons.size();
    const unsigned but_height =
       (window_size.height - finalmargin) / n - margin;

    PixelRect rc = GetClientRect();
    rc.left = 0;
    rc.right -= tab_line_height;

    for (unsigned i = 0; i < n; ++i) {
      rc.top = finalmargin + (margin + but_height) * i;
      rc.bottom = rc.top + but_height;
      buttons[i]->rc = rc;
    }
  } else {
    const unsigned n = buttons.size();
    const unsigned portraitRows = n > 4 ? 2 : 1;
    const unsigned rowheight = (window_size.height - tab_line_height)
      / portraitRows - margin;

    const unsigned portraitColumnsRow0 = portraitRows == 1 ? n : n / 2;
    const unsigned portraitColumnsRow1 = portraitRows == 1 ? 0 : n - n / 2;

    const unsigned but_width1 =
        (window_size.width - finalmargin) / portraitColumnsRow0 - margin;

    for (unsigned i = 0; i < portraitColumnsRow0; ++i) {
      PixelRect &rc = buttons[i]->rc;
      rc.top = 0;
      rc.bottom = rc.top + rowheight;
      rc.left = finalmargin + (margin + but_width1) * i;
      rc.right = rc.left + but_width1;
    }

    if (portraitColumnsRow1 > 0) {
      const unsigned but_width2 =
        (window_size.width - finalmargin) / portraitColumnsRow1 - margin;

      for (unsigned i = portraitColumnsRow0; i < n; ++i) {
        PixelRect &rc = buttons[i]->rc;
        rc.top = rowheight + margin;
        rc.bottom = rc.top + rowheight;
        rc.left = finalmargin + (margin + but_width2) * (i - portraitColumnsRow0);
        rc.right = rc.left + but_width2;
      }
    }
  }

  for (auto *button : buttons)
    button->InvalidateLayout();
}

void
TabDisplay::Add(const char *caption, const MaskedIcon *icon) noexcept
{
  buttons.append(new Button(caption, icon));
  CalculateLayout();
}

const char *
TabDisplay::GetCaption(unsigned i) const noexcept
{
  return buttons[i]->caption.c_str();
}

inline int
TabDisplay::GetButtonIndexAt(PixelPoint p) const noexcept
{
  for (std::size_t i = 0; i < buttons.size(); i++) {
    if (buttons[i]->rc.Contains(p))
      return i;
  }

  return -1;
}

void
TabDisplay::OnResize(PixelSize new_size) noexcept
{
  PaintWindow::OnResize(new_size);

  CalculateLayout();
}

void
TabDisplay::OnPaint(Canvas &canvas) noexcept
{
  if (HaveClipping())
    canvas.Clear(look.dark_mode
                 ? DarkColor(look.background_color)
                 : COLOR_BLACK);

  const bool is_focused = !HasCursorKeys() || HasFocus();
  for (unsigned i = 0; i < buttons.size(); i++) {
    const auto &button = *buttons[i];

    const bool is_down = dragging && i == down_index && !drag_off_button;
    const bool is_selected = i == current_index;

    button.Draw(canvas, look, is_focused, is_down, is_selected);
  }

  if (!buttons.empty()) {
    const PixelRect &selected_rc = buttons[current_index]->rc;
    const Color indicator_color = look.dark_mode
      ? look.focused.background_color
      : look.list.focused.background_color;

    PixelRect indicator_rc = selected_rc;
    if (vertical) {
      indicator_rc.left = selected_rc.right;
      indicator_rc.right = selected_rc.right + (int)tab_line_height;
    } else {
      indicator_rc.top = selected_rc.bottom;
      indicator_rc.bottom = selected_rc.bottom + (int)tab_line_height;
    }

    // TODO: add PixelRect::ClippedTo() in ui/dim/Rect.hpp and use it here.
    const PixelRect client_rc = GetClientRect();
    indicator_rc.left = std::max(indicator_rc.left, client_rc.left);
    indicator_rc.top = std::max(indicator_rc.top, client_rc.top);
    indicator_rc.right = std::min(indicator_rc.right, client_rc.right);
    indicator_rc.bottom = std::min(indicator_rc.bottom, client_rc.bottom);
    if (!indicator_rc.IsEmpty())
      canvas.DrawFilledRectangle(indicator_rc, indicator_color);
  }
}

void
TabDisplay::OnKillFocus() noexcept
{
  Invalidate();
  PaintWindow::OnKillFocus();
}

void
TabDisplay::OnSetFocus() noexcept
{
  Invalidate();
  PaintWindow::OnSetFocus();
}

void
TabDisplay::OnCancelMode() noexcept
{
  PaintWindow::OnCancelMode();
  EndDrag();
}

bool
TabDisplay::OnKeyCheck(unsigned key_code) const noexcept
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
    return current_index > 0;

  case KEY_RIGHT:
    return current_index < buttons.size() - 1;

  case KEY_DOWN:
    return false;

  case KEY_UP:
    return false;

  default:
    return false;
  }
}

bool
TabDisplay::OnKeyDown(unsigned key_code) noexcept
{
  switch (key_code) {

  case KEY_APP1:
    if (buttons.size() > 0)
      handler.ClickPage(0);
    return true;

  case KEY_APP2:
    if (buttons.size() > 1)
      handler.ClickPage(1);
    return true;

  case KEY_APP3:
    if (buttons.size() > 2)
      handler.ClickPage(2);
    return true;

  case KEY_APP4:
    if (buttons.size() > 3)
      handler.ClickPage(3);
    return true;

  case KEY_RETURN:
    handler.ClickPage(current_index);
    return true;

  case KEY_DOWN:
    break;

  case KEY_RIGHT:
    handler.NextPage();
    return true;

  case KEY_UP:
    break;

  case KEY_LEFT:
    handler.PreviousPage();
    return true;
  }
  return PaintWindow::OnKeyDown(key_code);
}

bool
TabDisplay::OnMouseDown(PixelPoint p) noexcept
{
  EndDrag();

  // If possible -> Give focus to the Control
  SetFocus();

  int i = GetButtonIndexAt(p);
  if (i >= 0) {
    dragging = true;
    drag_off_button = false;
    down_index = i;
    SetCapture();
    Invalidate();
    return true;
  }

  return PaintWindow::OnMouseDown(p);
}

bool
TabDisplay::OnMouseUp(PixelPoint p) noexcept
{
  if (dragging) {
    EndDrag();

    if (!drag_off_button)
      handler.ClickPage(down_index);

    return true;
  } else {
    return PaintWindow::OnMouseUp(p);
  }
}

bool
TabDisplay::OnMouseMove(PixelPoint p, [[maybe_unused]] unsigned keys) noexcept
{
  if (!dragging)
    return false;

  const PixelRect &rc = buttons[down_index]->rc;

  bool not_on_button = !rc.Contains(p);
  if (drag_off_button != not_on_button) {
    drag_off_button = not_on_button;
    Invalidate(rc);
  }
  return true;
}

void
TabDisplay::EndDrag() noexcept
{
  if (dragging) {
    dragging = false;
    ReleaseCapture();
    Invalidate();
  }
}
