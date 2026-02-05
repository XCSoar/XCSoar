// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RichTextWindow.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"
#include "Screen/Layout.hpp"
#include "Look/Colors.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/OpenLink.hpp"
#include "Dialogs/InternalLink.hpp"

#include <algorithm>

/**
 * Check if character can be part of a URL.
 */
static constexpr bool
IsUrlChar(TCHAR c) noexcept
{
  return (c >= _T('a') && c <= _T('z')) ||
         (c >= _T('A') && c <= _T('Z')) ||
         (c >= _T('0') && c <= _T('9')) ||
         c == _T('-') || c == _T('_') || c == _T('.') ||
         c == _T('/') || c == _T(':') || c == _T('?') ||
         c == _T('=') || c == _T('&') || c == _T('#') ||
         c == _T('%') || c == _T('+') || c == _T('~') ||
         c == _T('@');
}

void
RichTextWindow::Create(ContainerWindow &parent, PixelRect rc,
                       const WindowStyle style)
{
  origin = 0;
  focused_link = -1;

  PaintWindow::Create(parent, rc, style);
}

void
RichTextWindow::SetText(const TCHAR *_text) noexcept
{
  if (_text != nullptr)
    text = _text;
  else
    text.clear();

  ParseLinks();
  origin = 0;
  focused_link = -1;
  Invalidate();
}

void
RichTextWindow::ParseLinks() noexcept
{
  links.clear();

  if (text.empty())
    return;

  const TCHAR *str = text.c_str();
  const TCHAR *start = str;

  while (*str != _T('\0')) {
    // Look for URL prefixes
    const TCHAR *url_start = nullptr;

    if (StringStartsWith(str, _T("https://"))) {
      url_start = str;
    } else if (StringStartsWith(str, _T("http://"))) {
      url_start = str;
    } else if (StringStartsWith(str, _T("xcsoar://"))) {
      url_start = str;
    }

    if (url_start != nullptr) {
      // Find end of URL
      const TCHAR *url_end = url_start;
      while (*url_end != _T('\0') && IsUrlChar(*url_end))
        ++url_end;

      // Remove trailing punctuation that's unlikely part of URL
      while (url_end > url_start) {
        TCHAR last = *(url_end - 1);
        if (last == _T('.') || last == _T(',') ||
            last == _T(')') || last == _T(']'))
          --url_end;
        else
          break;
      }

      if (url_end > url_start) {
        RichTextLink link;
        link.start = url_start - start;
        link.end = url_end - start;
        link.url.assign(url_start, url_end);
        links.push_back(std::move(link));
      }

      str = url_end;
    } else {
      ++str;
    }
  }
}

unsigned
RichTextWindow::GetVisibleRows() const noexcept
{
  if (font == nullptr)
    return 1;
  return GetSize().height / font->GetLineSpacing();
}

unsigned
RichTextWindow::GetRowCount() const noexcept
{
  const TCHAR *str = text.c_str();
  unsigned row_count = 1;
  while ((str = StringFind(str, _T('\n'))) != nullptr) {
    str++;
    row_count++;
  }
  return row_count;
}

void
RichTextWindow::ScrollVertically(int delta_lines) noexcept
{
  const unsigned visible_rows = GetVisibleRows();
  const unsigned row_count = GetRowCount();

  if (visible_rows >= row_count)
    return;

  int new_origin = static_cast<int>(origin) + delta_lines;
  if (new_origin < 0)
    new_origin = 0;
  else if (static_cast<unsigned>(new_origin) > row_count - visible_rows)
    new_origin = row_count - visible_rows;

  ScrollTo(static_cast<unsigned>(new_origin));
}

void
RichTextWindow::ScrollTo(unsigned new_origin) noexcept
{
  if (new_origin != origin) {
    origin = new_origin;
    Invalidate();
  }
}

void
RichTextWindow::OnResize(PixelSize new_size) noexcept
{
  PaintWindow::OnResize(new_size);

  if (!text.empty()) {
    const unsigned visible_rows = GetVisibleRows();
    const unsigned row_count = GetRowCount();
    if (visible_rows >= row_count)
      origin = 0;
    else if (origin > row_count - visible_rows)
      origin = row_count - visible_rows;
    Invalidate();
  }
}

void
RichTextWindow::OnSetFocus() noexcept
{
  PaintWindow::OnSetFocus();
  Invalidate();
}

void
RichTextWindow::OnKillFocus() noexcept
{
  PaintWindow::OnKillFocus();
  focused_link = -1;
  Invalidate();
}

void
RichTextWindow::OnPaint(Canvas &canvas) noexcept
{
  canvas.ClearWhite();

  auto rc = canvas.GetRect();
  canvas.DrawOutlineRectangle(rc, COLOR_BLACK);

  if (HasFocus())
    canvas.DrawFocusRectangle(rc.WithPadding(1));

  if (text.empty() || font == nullptr)
    return;

  const int padding = Layout::GetTextPadding() * 2;
  rc.Grow(-padding);

  canvas.SetBackgroundTransparent();
  canvas.Select(*font);

  const int line_height = font->GetLineSpacing();

  // Calculate scroll offset
  const int scroll_offset = origin * line_height;

  // Draw text line by line, handling links
  int y = rc.top - scroll_offset;
  const TCHAR *line_start = text.c_str();
  std::size_t char_pos = 0;

  while (*line_start != _T('\0') && y < rc.bottom) {
    // Find end of line
    const TCHAR *line_end = line_start;
    while (*line_end != _T('\0') && *line_end != _T('\n'))
      ++line_end;

    std::size_t line_len = line_end - line_start;
    std::size_t line_start_pos = char_pos;
    std::size_t line_end_pos = char_pos + line_len;

    if (y + line_height > rc.top) {
      // Draw this line
      int x = rc.left;
      std::size_t pos = line_start_pos;

      while (pos < line_end_pos) {
        // Check if we're at a link
        int link_idx = -1;
        for (std::size_t i = 0; i < links.size(); ++i) {
          if (pos >= links[i].start && pos < links[i].end) {
            link_idx = static_cast<int>(i);
            break;
          }
        }

        if (link_idx >= 0) {
          // Draw link
          RichTextLink &link = links[link_idx];
          std::size_t link_start_in_line =
            std::max(link.start, line_start_pos) - line_start_pos;
          std::size_t link_end_in_line =
            std::min(link.end, line_end_pos) - line_start_pos;

          tstring link_text(line_start + link_start_in_line,
                            link_end_in_line - link_start_in_line);

          // Set link color
          bool is_focused = (link_idx == focused_link);
          canvas.SetTextColor(is_focused ? COLOR_XCSOAR_LIGHT : COLOR_XCSOAR);

          // Draw link text
          PixelSize text_size = canvas.CalcTextSize(link_text.c_str());
          canvas.DrawText({x, y}, link_text.c_str());

          // Draw underline
          int underline_y = y + line_height - 2;
          canvas.DrawLine({x, underline_y},
                          {x + static_cast<int>(text_size.width), underline_y});

          // Update link rectangle (for click detection)
          link.rect = PixelRect{x, y,
                                x + static_cast<int>(text_size.width),
                                y + line_height};

          // Draw focus indicator
          if (is_focused && HasFocus()) {
            PixelRect focus_rc = link.rect;
            focus_rc.Grow(2);
            canvas.DrawFocusRectangle(focus_rc);
          }

          x += text_size.width;
          pos = line_start_pos + link_end_in_line;
        } else {
          // Draw regular text until next link or end of line
          std::size_t next_link_pos = line_end_pos;
          for (const auto &link : links) {
            if (link.start > pos && link.start < next_link_pos)
              next_link_pos = link.start;
          }

          std::size_t text_start_in_line = pos - line_start_pos;
          std::size_t text_end_in_line = next_link_pos - line_start_pos;

          tstring regular_text(line_start + text_start_in_line,
                               text_end_in_line - text_start_in_line);

          canvas.SetTextColor(COLOR_BLACK);
          PixelSize text_size = canvas.CalcTextSize(regular_text.c_str());
          canvas.DrawText({x, y}, regular_text.c_str());

          x += text_size.width;
          pos = next_link_pos;
        }
      }
    }

    // Move to next line
    y += line_height;
    char_pos = line_end_pos;
    if (*line_end == _T('\n')) {
      ++char_pos;
      line_start = line_end + 1;
    } else {
      line_start = line_end;
    }
  }
}

bool
RichTextWindow::OnKeyCheck(unsigned key_code) const noexcept
{
  switch (key_code) {
  case KEY_UP:
    return origin > 0 || focused_link > 0;

  case KEY_DOWN:
    return (GetRowCount() > GetVisibleRows() &&
            origin < GetRowCount() - GetVisibleRows()) ||
           (focused_link < 0 && !links.empty()) ||
           (focused_link >= 0 &&
            static_cast<std::size_t>(focused_link) < links.size() - 1);

  case KEY_RETURN:
    return focused_link >= 0;

  case KEY_TAB:
    return !links.empty();
  }

  return false;
}

bool
RichTextWindow::OnKeyDown(unsigned key_code) noexcept
{
  switch (key_code) {
  case KEY_UP:
    if (focused_link > 0) {
      FocusPreviousLink();
      return true;
    }
    ScrollVertically(-1);
    return true;

  case KEY_DOWN:
    if (focused_link < 0 && !links.empty()) {
      focused_link = 0;
      ScrollToFocusedLink();
      Invalidate();
      return true;
    }
    if (FocusNextLink())
      return true;
    ScrollVertically(1);
    return true;

  case KEY_TAB:
    if (FocusNextLink())
      return true;
    // If no more links, let tab go to next control
    return false;

  case KEY_RETURN:
    if (focused_link >= 0) {
      ActivateFocusedLink();
      return true;
    }
    return false;

  case KEY_HOME:
    ScrollTo(0);
    return true;

  case KEY_END:
    if (unsigned visible_rows = GetVisibleRows(), row_count = GetRowCount();
        visible_rows < row_count)
      ScrollTo(row_count - visible_rows);
    return true;

  case KEY_PRIOR:
    ScrollVertically(-static_cast<int>(GetVisibleRows()));
    return true;

  case KEY_NEXT:
    ScrollVertically(static_cast<int>(GetVisibleRows()));
    return true;
  }

  return PaintWindow::OnKeyDown(key_code);
}

bool
RichTextWindow::OnMouseDown([[maybe_unused]] PixelPoint p) noexcept
{
  if (IsTabStop())
    SetFocus();

  return true;
}

bool
RichTextWindow::OnMouseUp(PixelPoint p) noexcept
{
  int link_idx = FindLinkAt(p);
  if (link_idx >= 0) {
    OnLinkActivated(links[link_idx].url.c_str());
    return true;
  }

  return PaintWindow::OnMouseUp(p);
}

bool
RichTextWindow::OnLinkActivated(const TCHAR *url) noexcept
{
#ifdef _UNICODE
  // Convert to UTF-8
  char utf8_url[512];
  WideCharToMultiByte(CP_UTF8, 0, url, -1, utf8_url, sizeof(utf8_url),
                      nullptr, nullptr);
  const char *url_utf8 = utf8_url;
#else
  const char *url_utf8 = url;
#endif

  // Try internal xcsoar:// links first
  if (HandleInternalLink(url_utf8))
    return true;

  // Then try external URLs
  if (StringStartsWith(url, _T("https://")) ||
      StringStartsWith(url, _T("http://"))) {
    return OpenLink(url_utf8);
  }

  return false;
}

bool
RichTextWindow::FocusNextLink() noexcept
{
  if (links.empty())
    return false;

  if (focused_link < 0) {
    focused_link = 0;
  } else if (static_cast<std::size_t>(focused_link) < links.size() - 1) {
    ++focused_link;
  } else {
    return false;
  }

  ScrollToFocusedLink();
  Invalidate();
  return true;
}

bool
RichTextWindow::FocusPreviousLink() noexcept
{
  if (links.empty() || focused_link <= 0)
    return false;

  --focused_link;
  ScrollToFocusedLink();
  Invalidate();
  return true;
}

void
RichTextWindow::ActivateFocusedLink() noexcept
{
  if (focused_link >= 0 &&
      static_cast<std::size_t>(focused_link) < links.size()) {
    OnLinkActivated(links[focused_link].url.c_str());
  }
}

int
RichTextWindow::FindLinkAt(PixelPoint p) const noexcept
{
  for (std::size_t i = 0; i < links.size(); ++i) {
    if (links[i].rect.Contains(p))
      return static_cast<int>(i);
  }
  return -1;
}

void
RichTextWindow::ScrollToFocusedLink() noexcept
{
  if (focused_link < 0 ||
      static_cast<std::size_t>(focused_link) >= links.size())
    return;

  // For now, simple approach - we'd need to calculate which line
  // the link is on and scroll to make it visible
  // This requires knowing the link's position in terms of lines,
  // which we can estimate from the link's character position

  // TODO: Implement proper scroll-to-link
}
