// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/Button.hpp"
#include "ui/event/KeyCode.hpp"
#include "Asset.hpp"
#include "Renderer/TextButtonRenderer.hpp"
#include "Hardware/Vibrator.hpp"

Button::Button(ContainerWindow &parent, const PixelRect &rc,
               WindowStyle style, std::unique_ptr<ButtonRenderer> _renderer,
               Callback _callback) noexcept
{
  Create(parent, rc, style, std::move(_renderer), std::move(_callback));
}

Button::Button(ContainerWindow &parent, const ButtonLook &look,
               const char *caption, const PixelRect &rc,
               WindowStyle style,
               Callback _callback) noexcept
{
  Create(parent, look, caption, rc, style, std::move(_callback));
}

Button::Button() = default;

Button::~Button() noexcept = default;

void
Button::Create(ContainerWindow &parent,
               const PixelRect &rc,
               WindowStyle style,
               std::unique_ptr<ButtonRenderer> _renderer)
{
  dragging = down = selected = false;
  renderer = std::move(_renderer);

  PaintWindow::Create(parent, rc, style);
}

void
Button::Create(ContainerWindow &parent, const ButtonLook &look,
               const char *caption, const PixelRect &rc,
               WindowStyle style)
{
  Create(parent, rc, style, std::make_unique<TextButtonRenderer>(look, caption));
}

void
Button::Create(ContainerWindow &parent, const PixelRect &rc,
               WindowStyle style, std::unique_ptr<ButtonRenderer> _renderer,
               Callback _callback) noexcept
{
  callback = std::move(_callback);

  Create(parent, rc, style, std::move(_renderer));
}

void
Button::Create(ContainerWindow &parent, const ButtonLook &look,
               const char *caption, const PixelRect &rc,
               WindowStyle style,
               Callback _callback) noexcept {
  Create(parent, rc, style,
         std::make_unique<TextButtonRenderer>(look, caption),
         std::move(_callback));
}

void
Button::SetCaption(const char *caption)
{
  assert(caption != nullptr);

  auto &r = (TextButtonRenderer &)*renderer;
  r.SetCaption(caption);

  Invalidate();
}

void
Button::SetSelected(bool _selected)
{
  if (_selected == selected)
    return;

  selected = _selected;
  Invalidate();
}

unsigned
Button::GetMinimumWidth() const
{
  return renderer->GetMinimumButtonWidth();
}

void
Button::SetDown(bool _down)
{
  if (_down == down)
    return;

#ifdef HAVE_VIBRATOR
  VibrateShort();
#endif

  down = _down;
  Invalidate();
}

bool
Button::OnClicked() noexcept
{
  if (callback) {
    callback();
    return true;
  }

  return false;
}

void
Button::Click()
{
  SetDown(false);
  OnClicked();
}

bool
Button::OnKeyCheck(unsigned key_code) const noexcept
{
  switch (key_code) {
  case KEY_RETURN:
    return true;

  default:
    return PaintWindow::OnKeyCheck(key_code);
  }
}

bool
Button::OnKeyDown(unsigned key_code) noexcept
{
  switch (key_code) {
  case KEY_RETURN:
  case KEY_SPACE:
    Click();
    return true;

  default:
    return PaintWindow::OnKeyDown(key_code);
  }
}

bool
Button::OnMouseMove(PixelPoint p, unsigned keys) noexcept
{
  if (dragging) {
    SetDown(IsInside(p));
    return true;
  } else
    return PaintWindow::OnMouseMove(p, keys);
}

bool
Button::OnMouseDown([[maybe_unused]] PixelPoint p) noexcept
{
  if (IsTabStop())
    SetFocus();

  SetDown(true);
  SetCapture();
  dragging = true;
  return true;
}

bool
Button::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
{
  if (!dragging)
    return true;

  dragging = false;
  ReleaseCapture();

  if (!down)
    return true;

  Click();
  return true;
}

void
Button::OnSetFocus() noexcept
{
  PaintWindow::OnSetFocus();
  Invalidate();
}

void
Button::OnKillFocus() noexcept
{
  PaintWindow::OnKillFocus();
  Invalidate();
}

void
Button::OnCancelMode() noexcept
{
  dragging = false;
  SetDown(false);

  PaintWindow::OnCancelMode();
}

void
Button::OnPaint(Canvas &canvas) noexcept
{
  assert(renderer != nullptr);

  renderer->DrawButton(canvas, GetClientRect(), GetState());
}

ButtonState
Button::GetState() const noexcept
{
  if (!IsEnabled())
    return ButtonState::DISABLED;
  else if (down)
    return ButtonState::PRESSED;
  else if (HasCursorKeys() && HasFocus())
    return ButtonState::FOCUSED;
  else if (HasCursorKeys() && selected)
    return ButtonState::SELECTED;
  else
    return ButtonState::ENABLED;
}
