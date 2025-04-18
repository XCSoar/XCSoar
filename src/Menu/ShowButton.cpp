// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ShowButton.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Pen.hpp"
#include "Screen/Layout.hpp"
#include "Input/InputEvents.hpp"
#include "util/Macros.hpp"
#include "Interface.hpp"
#include "UIState.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

/* "M" menu button */
class ShowMenuButtonRenderer : public ButtonRenderer {
public:
  unsigned GetMinimumButtonWidth() const noexcept override {
    return Layout::GetMinimumControlHeight();
  }

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  ButtonState state) const noexcept override;
};

void
ShowMenuButton::Create(ContainerWindow &parent, const PixelRect &rc,
                       WindowStyle style) noexcept
{
  Button::Create(parent, rc, style,
                 std::make_unique<ShowMenuButtonRenderer>());
}

bool
ShowMenuButton::OnClicked() noexcept
{
  InputEvents::ShowMenu();
  return true;
}

void
ShowMenuButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                                   ButtonState state) const noexcept
{
  const unsigned pen_width = Layout::ScalePenWidth(2);
  const unsigned padding = Layout::GetTextPadding() + pen_width;

  const PagesState &_state = CommonInterface::GetUIState().pages;
  if (_state.special_page.IsDefined())
    return;

  canvas.Select(Pen(pen_width, COLOR_BLACK));
  canvas.DrawRoundRectangle({rc.left, rc.top, rc.right - 1, rc.bottom - 1},
                            PixelSize{Layout::VptScale(8u)});

  const BulkPixelPoint m[] = {
    BulkPixelPoint(rc.left + padding, rc.bottom - padding),
    BulkPixelPoint(rc.left + padding, rc.top + padding),
    BulkPixelPoint((rc.left + rc.right) / 2, rc.bottom - 2 * padding),
    BulkPixelPoint(rc.right - padding, rc.top + padding),
    BulkPixelPoint(rc.right - padding, rc.bottom - padding),
  };

  canvas.DrawPolyline(m, ARRAY_SIZE(m));

  if (state == ButtonState::PRESSED) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    canvas.DrawFilledRectangle(rc, COLOR_YELLOW.WithAlpha(80));
#else
    canvas.InvertRectangle(rc);
#endif
  }
}

/* zoom out button */
class ShowZoomOutButtonRenderer : public ButtonRenderer {
public:
  unsigned GetMinimumButtonWidth() const noexcept override {
    return Layout::GetMinimumControlHeight();
  }

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  ButtonState state) const noexcept override;
};

void
ShowZoomOutButton::Create(ContainerWindow &parent, const PixelRect &rc,
                       WindowStyle style) noexcept
{
  Button::Create(parent, rc, style,
                 std::make_unique<ShowZoomOutButtonRenderer>());
}

bool
ShowZoomOutButton::OnClicked() noexcept
{
  InputEvents::eventZoom(_T("out"));
  return true;
}

void
ShowZoomOutButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                                   ButtonState state) const noexcept
{
  const unsigned padding = Layout::GetTextPadding() + Layout::ScalePenWidth(5);

  const PagesState &_state = CommonInterface::GetUIState().pages;
  if (_state.special_page.IsDefined())
    return;

  canvas.Select(Pen(Layout::ScalePenWidth(1), COLOR_BLACK));
  canvas.DrawRoundRectangle({rc.left, rc.top, rc.right - 1, rc.bottom - 1},
                            PixelSize{Layout::VptScale(8u)});

  canvas.Select(Pen(Layout::ScalePenWidth(2), COLOR_BLACK));
  const BulkPixelPoint minus[] = {
    BulkPixelPoint(rc.left + padding, (rc.top + rc.bottom) / 2),
    BulkPixelPoint(rc.right - padding, (rc.top + rc.bottom) / 2),
  };
  canvas.DrawPolyline(minus, ARRAY_SIZE(minus));

  if (state == ButtonState::PRESSED) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    canvas.DrawFilledRectangle(rc, COLOR_YELLOW.WithAlpha(80));
#else
    canvas.InvertRectangle(rc);
#endif
  }
}

/* zoom in button */
class ShowZoomInButtonRenderer : public ButtonRenderer {
public:
  unsigned GetMinimumButtonWidth() const noexcept override {
    return Layout::GetMinimumControlHeight();
  }

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  ButtonState state) const noexcept override;
};

void
ShowZoomInButton::Create(ContainerWindow &parent, const PixelRect &rc,
                       WindowStyle style) noexcept
{
  Button::Create(parent, rc, style,
                 std::make_unique<ShowZoomInButtonRenderer>());
}

bool
ShowZoomInButton::OnClicked() noexcept
{
  InputEvents::eventZoom(_T("in"));
  return true;
}

void
ShowZoomInButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                                   ButtonState state) const noexcept
{
  const unsigned padding = Layout::GetTextPadding() + Layout::ScalePenWidth(5);

  const PagesState &_state = CommonInterface::GetUIState().pages;
  if (_state.special_page.IsDefined())
    return;

  canvas.Select(Pen(Layout::ScalePenWidth(1), COLOR_BLACK));
  canvas.DrawRoundRectangle({rc.left, rc.top, rc.right - 1, rc.bottom - 1},
                            PixelSize{Layout::VptScale(8u)});

  canvas.Select(Pen(Layout::ScalePenWidth(2), COLOR_BLACK));
  const BulkPixelPoint horizontal[] = {
    BulkPixelPoint(rc.left + padding, (rc.top + rc.bottom) / 2),
    BulkPixelPoint(rc.right - padding, (rc.top + rc.bottom) / 2),
  };
  canvas.DrawPolyline(horizontal, ARRAY_SIZE(horizontal));

  const BulkPixelPoint vertical[] = {
    BulkPixelPoint((rc.left + rc.right) / 2, rc.top + padding),
    BulkPixelPoint((rc.left + rc.right) / 2, rc.bottom - padding),
  };
  canvas.DrawPolyline(vertical, ARRAY_SIZE(vertical));

  if (state == ButtonState::PRESSED) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    canvas.DrawFilledRectangle(rc, COLOR_YELLOW.WithAlpha(80));
#else
    canvas.InvertRectangle(rc);
#endif
  }
}
