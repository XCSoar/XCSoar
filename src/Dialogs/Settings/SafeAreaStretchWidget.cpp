// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SafeAreaStretchWidget.hpp"
#include "DisplaySettings.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/window/PaintWindow.hpp"
#include "Renderer/SymbolRenderer.hpp"
#include "util/Macros.hpp"

#include <algorithm>

namespace {

struct Edge {
  DisplaySettings::SafeAreaStretchEdge bit;
};

constexpr Edge edges_table[] = {
  { DisplaySettings::SAFE_AREA_STRETCH_TOP },
  { DisplaySettings::SAFE_AREA_STRETCH_RIGHT },
  { DisplaySettings::SAFE_AREA_STRETCH_BOTTOM },
  { DisplaySettings::SAFE_AREA_STRETCH_LEFT },
};

enum {
  TOP, RIGHT, BOTTOM, LEFT,
};

/**
 * A schematic screen whose four edges can be picked.  The inner area
 * stands for the safe area: on a stretched edge it reaches the screen
 * border, on the others it keeps a margin for the system UI.
 */
class SafeAreaStretchWindow final : public PaintWindow {
  const DialogLook &look;

  uint8_t &edges;

  /** The edge the keyboard cursor is on. */
  unsigned cursor = TOP;

public:
  SafeAreaStretchWindow(const DialogLook &_look, uint8_t &_edges) noexcept
    :look(_look), edges(_edges) {}

private:
  [[gnu::pure]]
  unsigned GetBandWidth(const PixelRect &screen) const noexcept {
    return std::max(2 * Layout::GetTextPadding(), screen.GetWidth() / 7);
  }

  /**
   * The schematic screen, centred and in a portrait aspect ratio.
   */
  [[gnu::pure]]
  PixelRect GetScreenRect() const noexcept {
    const PixelRect rc = GetClientRect();
    const unsigned padding = 2 * Layout::GetTextPadding();

    unsigned height = rc.GetHeight() > 2 * padding
      ? rc.GetHeight() - 2 * padding
      : rc.GetHeight();
    unsigned width = std::min(height * 2 / 3,
                              rc.GetWidth() > 2 * padding
                              ? rc.GetWidth() - 2 * padding
                              : rc.GetWidth());
    height = std::min(height, width * 3 / 2);

    PixelRect result;
    result.left = rc.left + int(rc.GetWidth() - width) / 2;
    result.top = rc.top + int(rc.GetHeight() - height) / 2;
    result.right = result.left + int(width);
    result.bottom = result.top + int(height);
    return result;
  }

  /**
   * The area that can be tapped to toggle an edge.  This does not
   * move while the edge is toggled, so that repeated taps stay on the
   * same spot.
   */
  [[gnu::pure]]
  PixelRect GetEdgeRect(const PixelRect &screen, unsigned i) const noexcept {
    const int band = int(GetBandWidth(screen));

    switch (i) {
    case TOP:
      return {screen.left + band, screen.top,
              screen.right - band, screen.top + band};

    case BOTTOM:
      return {screen.left + band, screen.bottom - band,
              screen.right - band, screen.bottom};

    case LEFT:
      return {screen.left, screen.top + band,
              screen.left + band, screen.bottom - band};

    case RIGHT:
      return {screen.right - band, screen.top + band,
              screen.right, screen.bottom - band};
    }

    return screen;
  }

  /**
   * The safe area as it results from the current selection.
   */
  [[gnu::pure]]
  PixelRect GetSafeRect(const PixelRect &screen) const noexcept {
    const int band = int(GetBandWidth(screen));
    PixelRect rc = screen;

    if ((edges & DisplaySettings::SAFE_AREA_STRETCH_TOP) == 0)
      rc.top += band;
    if ((edges & DisplaySettings::SAFE_AREA_STRETCH_BOTTOM) == 0)
      rc.bottom -= band;
    if ((edges & DisplaySettings::SAFE_AREA_STRETCH_LEFT) == 0)
      rc.left += band;
    if ((edges & DisplaySettings::SAFE_AREA_STRETCH_RIGHT) == 0)
      rc.right -= band;

    return rc;
  }

  /**
   * The square in the middle of an edge band that holds the arrow.
   */
  [[gnu::pure]]
  PixelRect GetArrowRect(const PixelRect &screen,
                         unsigned i) const noexcept {
    const PixelRect rc = GetEdgeRect(screen, i);
    const int size = int(std::min(rc.GetWidth(), rc.GetHeight()));
    return PixelRect::Centered(rc.GetCenter(), {size, size});
  }

  /**
   * The aircraft symbol, built from rectangles so that it stays crisp
   * at any size.
   */
  void DrawAircraft(Canvas &canvas, PixelPoint centre, int size,
                    Color color) const noexcept;

  /**
   * A schematic map: a few terrain bands and the aircraft in the
   * middle.  The map always uses the whole screen, so this is the
   * background of the whole picture.
   */
  void DrawMap(Canvas &canvas, const PixelRect &screen,
               const PixelRect &safe) const noexcept;

  /**
   * An arrow pointing out of the safe area on a stretched edge, and
   * into it on the others.  The focused arrow is highlighted.
   */
  void DrawArrow(Canvas &canvas, const PixelRect &screen,
                 unsigned i) const noexcept;

  void MoveCursor(unsigned i) noexcept {
    cursor = i;
    Invalidate();
  }

protected:
  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;

  /* virtual methods from class Window */
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;

  void OnSetFocus() noexcept override {
    PaintWindow::OnSetFocus();
    Invalidate();
  }

  void OnKillFocus() noexcept override {
    PaintWindow::OnKillFocus();
    Invalidate();
  }
};

void
SafeAreaStretchWindow::DrawMap(Canvas &canvas, const PixelRect &screen,
                               const PixelRect &safe) const noexcept
{
  /* terrain does not follow the dialog theme in XCSoar either, so
     these are fixed, muted versions of the terrain ramp */
  static constexpr Color terrain[] = {
    Color(0xa7, 0xbc, 0x94),
    Color(0xc4, 0xc7, 0x97),
    Color(0xd6, 0xbb, 0x8f),
    Color(0xc2, 0x9b, 0x7e),
  };

  const int height = int(screen.GetHeight());

  for (unsigned i = 0; i < ARRAY_SIZE(terrain); ++i) {
    PixelRect rc = screen;
    rc.top = screen.top + height * int(i) / int(ARRAY_SIZE(terrain));
    rc.bottom = screen.top + height * int(i + 1) / int(ARRAY_SIZE(terrain));
    canvas.DrawFilledRectangle(rc, terrain[i]);
  }

  /* the edges that are not stretched are covered by system UI: hide
     the map there */
  const Color cover = look.background_color;

  if (safe.top > screen.top)
    canvas.DrawFilledRectangle({screen.left, screen.top,
                                screen.right, safe.top}, cover);
  if (safe.bottom < screen.bottom)
    canvas.DrawFilledRectangle({screen.left, safe.bottom,
                                screen.right, screen.bottom}, cover);
  if (safe.left > screen.left)
    canvas.DrawFilledRectangle({screen.left, safe.top,
                                safe.left, safe.bottom}, cover);
  if (safe.right < screen.right)
    canvas.DrawFilledRectangle({safe.right, safe.top,
                                screen.right, safe.bottom}, cover);

  /* the aircraft marks the centre of the visible map */
  const PixelPoint centre = safe.GetCenter();
  const int size = int(std::min(safe.GetWidth(), safe.GetHeight())) / 8;
  if (size < 4)
    return;

  /* a white halo, like the aircraft has on the real map */
  const int halo = std::max(1, size / 10);
  DrawAircraft(canvas, {centre.x - halo, centre.y}, size, COLOR_WHITE);
  DrawAircraft(canvas, {centre.x + halo, centre.y}, size, COLOR_WHITE);
  DrawAircraft(canvas, {centre.x, centre.y - halo}, size, COLOR_WHITE);
  DrawAircraft(canvas, {centre.x, centre.y + halo}, size, COLOR_WHITE);

  /* like the terrain colours, the aircraft does not follow the dialog
     theme: it is drawn on the map, where a light symbol would
     disappear */
  DrawAircraft(canvas, centre, size, COLOR_BLACK);
}

void
SafeAreaStretchWindow::DrawAircraft(Canvas &canvas, PixelPoint centre,
                                    int size, Color color) const noexcept
{
  const int thin = std::max(1, size / 5);

  canvas.DrawFilledRectangle({centre.x - thin / 2, centre.y - size / 2,
                              centre.x - thin / 2 + thin,
                              centre.y + size / 2}, color);
  canvas.DrawFilledRectangle({centre.x - size, centre.y - thin / 2,
                              centre.x + size,
                              centre.y - thin / 2 + thin}, color);
  canvas.DrawFilledRectangle({centre.x - size / 3,
                              centre.y + size / 2 - thin,
                              centre.x + size / 3,
                              centre.y + size / 2}, color);
}

void
SafeAreaStretchWindow::DrawArrow(Canvas &canvas, const PixelRect &screen,
                                 unsigned i) const noexcept
{
  const bool out = (edges & edges_table[i].bit) != 0;
  const bool focused = HasFocus() && i == cursor;
  const PixelRect rc = GetArrowRect(screen, i);

  /* the tip points away from the centre of the screen when the edge
     is stretched, and towards it when it is not */
  SymbolRenderer::Direction direction;
  switch (i) {
  case TOP:
    direction = out ? SymbolRenderer::UP : SymbolRenderer::DOWN;
    break;

  case BOTTOM:
    direction = out ? SymbolRenderer::DOWN : SymbolRenderer::UP;
    break;

  case LEFT:
    direction = out ? SymbolRenderer::LEFT : SymbolRenderer::RIGHT;
    break;

  default:
    direction = out ? SymbolRenderer::RIGHT : SymbolRenderer::LEFT;
    break;
  }

  /* the focused arrow sits on a filled badge, the way XCSoar marks a
     focused control elsewhere */
  if (focused)
    canvas.DrawFilledRectangle(rc, look.focused.background_color);

  const Brush brush(focused
                    ? look.focused.text_color
                    : (out
                       ? look.focused.background_color
                       : look.text_color));

  canvas.SelectNullPen();
  canvas.Select(brush);

  SymbolRenderer::DrawArrow(canvas, rc, direction);

  /* with GDI, Select() keeps the handle, and the brush is about to go
     out of scope */
  canvas.SelectHollowBrush();
}

void
SafeAreaStretchWindow::OnPaint(Canvas &canvas) noexcept
{
  /* like a list, the picker is the content area of the dialog; only
     the help text below it keeps the dialog background */
  canvas.Clear(look.list.background_color);

  const PixelRect screen = GetScreenRect();
  const PixelRect safe = GetSafeRect(screen);

  /* the map always uses the whole screen, so it is the background of
     the whole picture */
  DrawMap(canvas, screen, safe);

  for (unsigned i = 0; i < ARRAY_SIZE(edges_table); ++i)
    DrawArrow(canvas, screen, i);

  /* the safe area is outlined on the map rather than covering it, so
     that the map stays visible underneath - it does use the whole
     screen, after all */
  const Pen safe_pen(std::max(2u, Layout::VptScale(2u)), look.text_color);

  canvas.SelectHollowBrush();
  canvas.Select(safe_pen);
  canvas.DrawRectangle(safe);

  canvas.Select(look.focused.border_pen);
  canvas.DrawRectangle(screen);
}

bool
SafeAreaStretchWindow::OnMouseDown(PixelPoint p) noexcept
{
  const PixelRect screen = GetScreenRect();

  for (unsigned i = 0; i < ARRAY_SIZE(edges_table); ++i) {
    if (!GetEdgeRect(screen, i).Contains(p))
      continue;

    SetFocus();
    cursor = i;
    edges ^= edges_table[i].bit;
    Invalidate();
    return true;
  }

  return PaintWindow::OnMouseDown(p);
}

bool
SafeAreaStretchWindow::OnKeyCheck(unsigned key_code) const noexcept
{
  switch (key_code) {
  case KEY_UP:
    return cursor != TOP;

  case KEY_DOWN:
    return cursor != BOTTOM;

  case KEY_LEFT:
    return cursor != LEFT;

  case KEY_RIGHT:
    return cursor != RIGHT;

  case KEY_RETURN:
  case KEY_SPACE:
    return true;

  default:
    return PaintWindow::OnKeyCheck(key_code);
  }
}

bool
SafeAreaStretchWindow::OnKeyDown(unsigned key_code) noexcept
{
  /* a cursor key picks the edge it points at; pressing it again when
     that edge is already picked is not consumed, so that the focus
     can leave this control */
  switch (key_code) {
  case KEY_UP:
    if (cursor == TOP)
      break;
    MoveCursor(TOP);
    return true;

  case KEY_DOWN:
    if (cursor == BOTTOM)
      break;
    MoveCursor(BOTTOM);
    return true;

  case KEY_LEFT:
    if (cursor == LEFT)
      break;
    MoveCursor(LEFT);
    return true;

  case KEY_RIGHT:
    if (cursor == RIGHT)
      break;
    MoveCursor(RIGHT);
    return true;

  case KEY_RETURN:
  case KEY_SPACE:
    edges ^= edges_table[cursor].bit;
    Invalidate();
    return true;
  }

  return PaintWindow::OnKeyDown(key_code);
}

} // anonymous namespace

PixelSize
SafeAreaStretchWidget::GetMinimumSize() const noexcept
{
  const unsigned height = Layout::GetMaximumControlHeight() * 5;
  return {height * 2 / 3, height};
}

PixelSize
SafeAreaStretchWidget::GetMaximumSize() const noexcept
{
  const unsigned height = Layout::GetMaximumControlHeight() * 10;
  return {height * 2 / 3, height};
}

void
SafeAreaStretchWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.TabStop();

  auto window = std::make_unique<SafeAreaStretchWindow>(look, edges);
  window->Create(parent, rc, style);
  SetWindow(std::move(window));
}
