// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "LogFile.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "Asset.hpp"
#include "Renderer/TextButtonRenderer.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "util/StringAPI.hxx"
#include "Hardware/Vibrator.hpp"

#ifdef HAVE_VIBRATOR
#include "Interface.hpp"
#include "UISettings.hpp"
#include "GlobalSettings.hpp"
#endif

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
PlayHapticFeedback() noexcept
{
#ifdef HAVE_VIBRATOR
  const UISettings &ui_settings = CommonInterface::GetUISettings();
  if (ui_settings.haptic_feedback == UISettings::HapticFeedback::ON ||
      (ui_settings.haptic_feedback == UISettings::HapticFeedback::DEFAULT &&
       GlobalSettings::haptic_feedback))
    Vibrate(HapticFeedbackType::PRESS);
#endif
}

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

[[gnu::pure]]
static const char *
MenuSymbolCaption(const char *caption) noexcept
{
  if (SymbolButtonRenderer::IsSymbolCaption(caption))
    return caption;

  const char *nl = StringFind(caption, '\n');
  if (nl == nullptr || nl[1] == '\0' || nl[2] != '\0')
    return nullptr;

  const char *symbol = nl + 1;
  return SymbolButtonRenderer::IsSymbolCaption(symbol) ? symbol : nullptr;
}

void
Button::SetMenuCaption(const ButtonLook &look, const char *caption) noexcept
{
  assert(caption != nullptr);

  const char *symbol = MenuSymbolCaption(caption);
  if (symbol != nullptr)
    renderer = std::make_unique<SymbolButtonRenderer>(
      look, symbol, SymbolButtonRenderer::Style::MENU);
  else
    renderer = std::make_unique<TextButtonRenderer>(look, caption);

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

  PlayHapticFeedback();

  down = _down;
  Invalidate();
}

bool
Button::OnClicked() noexcept
{
  if (callback) {
    try {
      callback();
    } catch (...) {
      LogError(std::current_exception(), "Button callback failed");
    }

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
  case KEY_UP:
  case KEY_DOWN:
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

  case KEY_UP:
    /* WndForm remaps unhandled Up/Down to tab order, but a Button
       outside a modal form (map overlay, and Up from the first
       chrome button) never got that path back to the widget */
    if (auto *parent = GetParent())
      return parent->FocusPreviousControl();
    break;

  case KEY_DOWN:
    if (auto *parent = GetParent())
      return parent->FocusNextControl();
    break;
  }

  return PaintWindow::OnKeyDown(key_code);
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
  if (cursor_key_group != nullptr)
    cursor_key_group->OnButtonGainedFocus(*this);
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
  /* Real keyboard focus uses `look.focused`.  Armed cursor-selection
     (list still focused, Left/Right chose an action) uses
     `look.selected` so the list cursor and action stay visible
     together — same model as Alternates. */
  else if (HasCursorKeys() && HasFocus())
    return ButtonState::FOCUSED;
  else if (HasCursorKeys() && selected)
    return ButtonState::SELECTED;
  else
    return ButtonState::ENABLED;
}
