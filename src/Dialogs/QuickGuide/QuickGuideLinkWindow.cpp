// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QuickGuideLinkWindow.hpp"

#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>
#include <winuser.h>

QuickGuideLinkWindow::QuickGuideLinkWindow() noexcept = default;

unsigned
QuickGuideLinkWindow::DrawLink(Canvas &canvas, std::size_t index,
                                PixelRect rc, const TCHAR *text) noexcept
{
  const bool is_focused = focused_link.has_value() && focused_link.value() == index;

  if (is_focused) {
    // Draw focus highlight background - estimate height based on font
    const unsigned line_height = canvas.GetFontHeight() + Layout::GetTextPadding();
    PixelRect highlight_rc = rc;
    highlight_rc.bottom = rc.top + int(line_height);
    canvas.DrawFilledRectangle(highlight_rc, COLOR_LIGHT_GRAY);
  }

  canvas.SetTextColor(COLOR_BLUE);
  const unsigned height = canvas.DrawFormattedText(rc, text, 
                                                    DT_LEFT | DT_UNDERLINE);
  canvas.SetTextColor(COLOR_BLACK);

  if (index >= link_rects.size()) {
    link_rects.resize(index + 1);
  }

  link_rects[index] = {rc.left, rc.top, rc.right, rc.top + int(height)};

  return height;
}

bool
QuickGuideLinkWindow::OnKeyCheck(unsigned key_code) const noexcept
{
  if (link_rects.empty())
    return PaintWindow::OnKeyCheck(key_code);

  switch (key_code) {
  case KEY_UP:
  case KEY_DOWN:
  case KEY_RETURN:
    return true;
  }

  return PaintWindow::OnKeyCheck(key_code);
}

bool
QuickGuideLinkWindow::OnKeyDown(unsigned key_code) noexcept
{
  if (link_rects.empty())
    return PaintWindow::OnKeyDown(key_code);

  switch (key_code) {
  case KEY_DOWN:
    // If we didn't handle it, forward to parent for smooth scrolling
    if (FocusNextLink())
      return true;
    // Forward to parent VScrollPanel
    if (ContainerWindow *parent = GetParent())
      return parent->OnKeyDown(key_code);
    break;

  case KEY_UP:
    // If we didn't handle it, forward to parent for smooth scrolling
    if (FocusPreviousLink())
      return true;
    // Forward to parent VScrollPanel
    if (ContainerWindow *parent = GetParent())
      return parent->OnKeyDown(key_code);
    break;

  case KEY_RETURN:
    ActivateFocusedLink();
    return true;
  }

  return PaintWindow::OnKeyDown(key_code);
}

bool
QuickGuideLinkWindow::OnMouseUp(PixelPoint p) noexcept
{
  for (std::size_t i = 0; i < link_rects.size(); ++i) {
    if (link_rects[i].Contains(p) && OnLinkActivated(i)) {
      Invalidate();
      return true;
    }
  }

  return PaintWindow::OnMouseUp(p);
}

bool
QuickGuideLinkWindow::FocusNextLink() noexcept
{
  if (link_rects.empty())
    return false;  // Let parent handle scrolling

  if (!focused_link.has_value()) {
    // No focus - check if there's a visible link to select
    if (FindFirstVisibleLink()) {
      // There's a visible link - select it (first time or scrolled back)
      ScrollToFocusedLink();
      Invalidate();
      return true;
    } else {
      // No visible link - let parent handle smooth scrolling
      return false;
    }
  } else if (focused_link.value() + 1 < link_rects.size()) {
    focused_link = focused_link.value() + 1;
    ScrollToFocusedLink();
    Invalidate();
    return true;
  } else {
    // At last link - let parent handle smooth scrolling for more content
    return false;
  }
}

bool
QuickGuideLinkWindow::FocusPreviousLink() noexcept
{
  if (link_rects.empty())
    return false;  // Let parent handle scrolling

  if (!focused_link.has_value()) {
    // No focus - check if there's a visible link to select
    if (FindLastVisibleLink()) {
      // There's a visible link - select it
      ScrollToFocusedLink();
      Invalidate();
      return true;
    } else {
      // No visible link - let parent handle smooth scrolling
      return false;
    }
  } else if (focused_link.value() > 0) {
    focused_link = focused_link.value() - 1;
    ScrollToFocusedLink();
    Invalidate();
    return true;
  } else {
    // At first link - let parent handle smooth scrolling for more content
    return false;
  }
}

bool
QuickGuideLinkWindow::FindFirstVisibleLink() noexcept
{
  ContainerWindow *parent = GetParent();
  if (parent == nullptr)
    return false;

  const int parent_height = parent->GetSize().height;
  const PixelRect window_rect = GetPosition();

  for (std::size_t i = 0; i < link_rects.size(); ++i) {
    const int link_top = link_rects[i].top + window_rect.top;
    const int link_bottom = link_rects[i].bottom + window_rect.top;

    if (link_bottom > 0 && link_top < parent_height) {
      focused_link = i;
      return true;
    }
  }
  return false;
}

bool
QuickGuideLinkWindow::FindLastVisibleLink() noexcept
{
  ContainerWindow *parent = GetParent();
  if (parent == nullptr)
    return false;

  const int parent_height = parent->GetSize().height;
  const PixelRect window_rect = GetPosition();

  for (std::size_t i = link_rects.size(); i > 0; --i) {
    const std::size_t idx = i - 1;
    const int link_top = link_rects[idx].top + window_rect.top;
    const int link_bottom = link_rects[idx].bottom + window_rect.top;

    if (link_bottom > 0 && link_top < parent_height) {
      focused_link = idx;
      return true;
    }
  }
  return false;
}

void
QuickGuideLinkWindow::ActivateFocusedLink() noexcept
{
  if (focused_link.has_value() && focused_link.value() < link_rects.size()) {
    OnLinkActivated(focused_link.value());
    Invalidate();
  }
}

void
QuickGuideLinkWindow::ScrollToFocusedLink() noexcept
{
  if (!focused_link.has_value() || focused_link.value() >= link_rects.size())
    return;

  ContainerWindow *parent = GetParent();
  if (parent == nullptr)
    return;

  const PixelRect &link_rc = link_rects[focused_link.value()];
  const PixelRect window_rect = GetPosition();
  const int parent_height = parent->GetSize().height;
  
  // Calculate link position in parent coordinates
  const int link_top = link_rc.top + window_rect.top;
  const int link_bottom = link_rc.bottom + window_rect.top;
  
  // Minimum scroll step (similar to text scrolling - ~6 lines)
  const int min_step = Layout::Scale(96);
  
  // Padding from edge
  const int padding = Layout::Scale(20);
  
  // Check if link is already fully visible with padding
  if (link_top >= padding && link_bottom <= parent_height - padding)
    return;  // Already visible, no scroll needed
  
  // Calculate scroll target - position link with padding from edge
  PixelRect scroll_rc;
  scroll_rc.left = 0;
  scroll_rc.right = 1;
  
  if (link_top < padding) {
    // Link is above viewport - scroll up
    // Position link at top with padding, but scroll at least min_step
    const int needed = padding - link_top;
    const int scroll_amount = std::max(needed, min_step);
    scroll_rc.top = -scroll_amount;
    scroll_rc.bottom = scroll_rc.top + 1;
  } else {
    // Link is below viewport - scroll down
    // Position link at bottom with padding, but scroll at least min_step
    const int needed = link_bottom - (parent_height - padding);
    const int scroll_amount = std::max(needed, min_step);
    scroll_rc.top = parent_height + scroll_amount;
    scroll_rc.bottom = scroll_rc.top + 1;
  }
  
  parent->ScrollTo(scroll_rc);
}
