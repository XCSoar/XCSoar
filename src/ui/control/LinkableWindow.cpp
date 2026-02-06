// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LinkableWindow.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "ui/event/KeyCode.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>

void
LinkableWindow::RegisterLinkRect(std::size_t index, PixelRect rect) noexcept
{
  link_segments.push_back({index, rect, rect.top});
  if (index >= link_count)
    link_count = index + 1;
}

bool
LinkableWindow::OnKeyCheck(unsigned key_code) const noexcept
{
  if (link_segments.empty())
    return PaintWindow::OnKeyCheck(key_code);

  switch (key_code) {
  case KEY_UP:
  case KEY_DOWN:
    return true;

  case KEY_RETURN:
    // Only claim KEY_RETURN if we have a focused link
    return focused_link.has_value();
  }

  return PaintWindow::OnKeyCheck(key_code);
}

bool
LinkableWindow::OnKeyDown(unsigned key_code) noexcept
{
  if (link_segments.empty())
    return PaintWindow::OnKeyDown(key_code);

  switch (key_code) {
  case KEY_DOWN:
    // If we didn't handle it, forward to parent for scrolling
    if (FocusNextLink())
      return true;
    // Forward to parent VScrollPanel
    if (ContainerWindow *parent = GetParent())
      return parent->InjectKeyPress(key_code);
    break;

  case KEY_UP:
    // If we didn't handle it, forward to parent for scrolling
    if (FocusPreviousLink())
      return true;
    // Forward to parent VScrollPanel
    if (ContainerWindow *parent = GetParent())
      return parent->InjectKeyPress(key_code);
    break;

  case KEY_RETURN:
    // Only handle if we have a focused link
    if (focused_link.has_value()) {
      ActivateFocusedLink();
      return true;
    }
    break;
  }

  return PaintWindow::OnKeyDown(key_code);
}

bool
LinkableWindow::OnMouseDown([[maybe_unused]] PixelPoint p) noexcept
{
  // Set focus if tab stop (required for VScrollPanel tap detection)
  if (IsTabStop())
    SetFocus();

  // Return true to indicate we want mouse events
  return true;
}

bool
LinkableWindow::OnMouseUp(PixelPoint p) noexcept
{
  for (const auto &seg : link_segments) {
    if (seg.rect.Contains(p) && OnLinkActivated(seg.link_index)) {
      Invalidate();
      return true;
    }
  }

  return PaintWindow::OnMouseUp(p);
}

bool
LinkableWindow::FocusNextLink() noexcept
{
  if (link_count == 0)
    return false;  // Let parent handle scrolling

  if (!focused_link.has_value()) {
    // No focus - check if there's a visible link to select
    if (FindFirstVisibleLink()) {
      // There's a visible link - select it
      ScrollToFocusedLink();
      Invalidate();
      return true;
    } else {
      // No visible link - let parent handle smooth scrolling
      return false;
    }
  } else if (focused_link.value() + 1 < link_count) {
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
LinkableWindow::FocusPreviousLink() noexcept
{
  if (link_count == 0)
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
LinkableWindow::FindFirstVisibleLink() noexcept
{
  ContainerWindow *parent = GetParent();
  if (parent == nullptr)
    return false;

  const int parent_height = parent->GetSize().height;
  const PixelRect window_rect = GetPosition();

  for (const auto &seg : link_segments) {
    const int link_top = seg.rect.top + window_rect.top;
    const int link_bottom = seg.rect.bottom + window_rect.top;

    if (link_bottom > 0 && link_top < parent_height) {
      focused_link = seg.link_index;
      return true;
    }
  }
  return false;
}

bool
LinkableWindow::FindLastVisibleLink() noexcept
{
  ContainerWindow *parent = GetParent();
  if (parent == nullptr)
    return false;

  const int parent_height = parent->GetSize().height;
  const PixelRect window_rect = GetPosition();

  // Iterate backwards through segments
  for (auto it = link_segments.rbegin(); it != link_segments.rend(); ++it) {
    const int link_top = it->rect.top + window_rect.top;
    const int link_bottom = it->rect.bottom + window_rect.top;

    if (link_bottom > 0 && link_top < parent_height) {
      focused_link = it->link_index;
      return true;
    }
  }
  return false;
}

void
LinkableWindow::ActivateFocusedLink() noexcept
{
  if (focused_link.has_value() && focused_link.value() < link_count) {
    OnLinkActivated(focused_link.value());
    Invalidate();
  }
}

void
LinkableWindow::ScrollToFocusedLink() noexcept
{
  if (!focused_link.has_value() || focused_link.value() >= link_count)
    return;

  // Find the first segment for the focused link
  const LinkSegment *focused_segment = nullptr;
  for (const auto &seg : link_segments) {
    if (seg.link_index == focused_link.value()) {
      focused_segment = &seg;
      break;
    }
  }

  if (focused_segment == nullptr)
    return;

  ContainerWindow *parent = GetParent();
  if (parent == nullptr)
    return;

  const PixelRect &link_rc = focused_segment->rect;
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
    const int needed = padding - link_top;
    const int scroll_amount = std::max(needed, min_step);
    scroll_rc.top = -scroll_amount;
    scroll_rc.bottom = scroll_rc.top + 1;
  } else {
    // Link is below viewport - scroll down
    const int needed = link_bottom - (parent_height - padding);
    const int scroll_amount = std::max(needed, min_step);
    scroll_rc.top = parent_height + scroll_amount;
    scroll_rc.bottom = scroll_rc.top + 1;
  }

  parent->ScrollTo(scroll_rc);
}
