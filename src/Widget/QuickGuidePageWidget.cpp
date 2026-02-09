// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QuickGuidePageWidget.hpp"
#include "VScrollWidget.hpp"
#include "RichTextWidget.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"

QuickGuidePageWidget::QuickGuidePageWidget(
  const DialogLook &_look,
  const char *_markdown_text) noexcept
  :look(_look),
   markdown_text(_markdown_text ? _markdown_text : "")
{
}

QuickGuidePageWidget::~QuickGuidePageWidget() noexcept = default;

std::unique_ptr<QuickGuidePageWidget>
QuickGuidePageWidget::CreateContentPage(
  const DialogLook &look,
  const char *markdown_text) noexcept
{
  return std::make_unique<QuickGuidePageWidget>(look, markdown_text);
}

std::unique_ptr<QuickGuidePageWidget>
QuickGuidePageWidget::CreateCheckboxPage(
  const DialogLook &look,
  const char *markdown_text,
  const char *_checkbox_label,
  bool initial_state,
  std::function<void(bool)> callback) noexcept
{
  auto widget = std::make_unique<QuickGuidePageWidget>(
    look, markdown_text);
  widget->bar_type = BottomBarType::CHECKBOX;
  widget->checkbox_label = _checkbox_label;
  widget->checkbox_initial_state = initial_state;
  widget->checkbox_callback = std::move(callback);
  return widget;
}

std::unique_ptr<QuickGuidePageWidget>
QuickGuidePageWidget::CreateButtonPage(
  const DialogLook &look,
  const char *markdown_text,
  const char *_button_label,
  std::function<void()> callback) noexcept
{
  auto widget = std::make_unique<QuickGuidePageWidget>(
    look, markdown_text);
  widget->bar_type = BottomBarType::ONE_BUTTON;
  widget->button1_label = _button_label;
  widget->button1_callback = std::move(callback);
  return widget;
}

std::unique_ptr<QuickGuidePageWidget>
QuickGuidePageWidget::CreateTwoButtonPage(
  const DialogLook &look,
  const char *markdown_text,
  const char *_button1_label,
  std::function<void()> button1_cb,
  const char *_button2_label,
  std::function<void()> button2_cb) noexcept
{
  auto widget = std::make_unique<QuickGuidePageWidget>(
    look, markdown_text);
  widget->bar_type = BottomBarType::TWO_BUTTONS;
  widget->button1_label = _button1_label;
  widget->button1_callback = std::move(button1_cb);
  widget->button2_label = _button2_label;
  widget->button2_callback = std::move(button2_cb);
  return widget;
}

bool
QuickGuidePageWidget::GetCheckboxState() const noexcept
{
  if (checkbox)
    return checkbox->GetState();
  return checkbox_initial_state;
}

void
QuickGuidePageWidget::SetCheckboxState(bool value) noexcept
{
  checkbox_initial_state = value;
  if (checkbox)
    checkbox->SetState(value);
}

void
QuickGuidePageWidget::SetText(const char *text) noexcept
{
  markdown_text = text ? text : "";
  // If the scroll widget already has a RichTextWidget inside,
  // we can't easily update it. The caller should recreate the page.
}

QuickGuidePageWidget::PageLayout
QuickGuidePageWidget::CalcLayout(const PixelRect &rc) const noexcept
{
  PageLayout layout;
  layout.has_bottom_bar = (bar_type != BottomBarType::NONE);

  if (layout.has_bottom_bar) {
    const unsigned bar_height =
      Layout::GetMaximumControlHeight();
    layout.content = rc;
    layout.bottom_bar = layout.content.CutBottomSafe(bar_height);
  } else {
    layout.content = rc;
    layout.bottom_bar = {};
  }

  return layout;
}

PixelSize
QuickGuidePageWidget::GetMinimumSize() const noexcept
{
  return {Layout::Scale(200u), Layout::Scale(200u)};
}

PixelSize
QuickGuidePageWidget::GetMaximumSize() const noexcept
{
  /* Use the minimum size as a reasonable default; the actual
     content height is managed by the internal VScrollWidget
     and its VScrollPanel, not by this outer widget. */
  PixelSize size = GetMinimumSize();
  if (bar_type != BottomBarType::NONE)
    size.height += Layout::GetMaximumControlHeight();
  return size;
}

void
QuickGuidePageWidget::Initialise(ContainerWindow &parent,
                                 const PixelRect &rc) noexcept
{
  // Create the RichTextWidget wrapped in VScrollWidget
  auto rich_text = std::make_unique<RichTextWidget>(
    look, markdown_text.c_str(), parse_links);
  auto scroll = std::make_unique<VScrollWidget>(
    std::move(rich_text), look, true);

  if (gesture_callback)
    scroll->SetGestureCallback(gesture_callback);

  scroll_widget = std::move(scroll);

  const auto layout = CalcLayout(rc);
  scroll_widget->Initialise(parent, layout.content);
}

void
QuickGuidePageWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  const auto layout = CalcLayout(rc);
  scroll_widget->Prepare(parent, layout.content);

  if (layout.has_bottom_bar)
    CreateBottomBar(parent, layout.bottom_bar);
}

void
QuickGuidePageWidget::CreateBottomBar(ContainerWindow &parent,
                                      const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.TabStop();

  switch (bar_type) {
  case BottomBarType::NONE:
    break;

  case BottomBarType::CHECKBOX:
    checkbox = std::make_unique<CheckBoxControl>();
    checkbox->Create(parent, look, checkbox_label.c_str(), rc,
                     style, checkbox_callback);
    checkbox->SetState(checkbox_initial_state);
    break;

  case BottomBarType::ONE_BUTTON:
    button1 = std::make_unique<Button>();
    button1->Create(parent, look.button, button1_label.c_str(),
                    rc, style, button1_callback);
    break;

  case BottomBarType::TWO_BUTTONS:
    {
      const auto [left, right] = rc.VerticalSplit();
      button1 = std::make_unique<Button>();
      button1->Create(parent, look.button, button1_label.c_str(),
                      left, style, button1_callback);
      button2 = std::make_unique<Button>();
      button2->Create(parent, look.button, button2_label.c_str(),
                      right, style, button2_callback);
    }
    break;
  }
}

void
QuickGuidePageWidget::Show(const PixelRect &rc) noexcept
{
  const auto layout = CalcLayout(rc);
  scroll_widget->Show(layout.content);

  if (layout.has_bottom_bar)
    PositionBottomBar(layout.bottom_bar, true);

  visible = true;
}

void
QuickGuidePageWidget::Hide() noexcept
{
  visible = false;
  scroll_widget->Hide();

  if (checkbox) checkbox->Hide();
  if (button1) button1->Hide();
  if (button2) button2->Hide();
}

void
QuickGuidePageWidget::Move(const PixelRect &rc) noexcept
{
  const auto layout = CalcLayout(rc);
  scroll_widget->Move(layout.content);

  if (layout.has_bottom_bar)
    PositionBottomBar(layout.bottom_bar, false);
}

void
QuickGuidePageWidget::PositionBottomBar(
  const PixelRect &bar_rect, bool show) noexcept
{
  switch (bar_type) {
  case BottomBarType::NONE:
    break;

  case BottomBarType::CHECKBOX:
    if (checkbox) {
      if (show)
        checkbox->MoveAndShow(bar_rect);
      else
        checkbox->Move(bar_rect);
    }
    break;

  case BottomBarType::ONE_BUTTON:
    if (button1) {
      if (show)
        button1->MoveAndShow(bar_rect);
      else
        button1->Move(bar_rect);
    }
    break;

  case BottomBarType::TWO_BUTTONS:
    {
      const auto [left, right] = bar_rect.VerticalSplit();
      if (button1) {
        if (show)
          button1->MoveAndShow(left);
        else
          button1->Move(left);
      }
      if (button2) {
        if (show)
          button2->MoveAndShow(right);
        else
          button2->Move(right);
      }
    }
    break;
  }
}

bool
QuickGuidePageWidget::SetFocus() noexcept
{
  return scroll_widget->SetFocus();
}

bool
QuickGuidePageWidget::HasFocus() const noexcept
{
  if (scroll_widget->HasFocus())
    return true;
  if (checkbox && checkbox->HasFocus())
    return true;
  if (button1 && button1->HasFocus())
    return true;
  if (button2 && button2->HasFocus())
    return true;
  return false;
}

bool
QuickGuidePageWidget::KeyPress(unsigned key_code) noexcept
{
  // Let the scroll content handle the key first
  if (scroll_widget->KeyPress(key_code))
    return true;

  // Handle DOWN to move focus from content to bottom bar
  if (key_code == KEY_DOWN && bar_type != BottomBarType::NONE) {
    switch (bar_type) {
    case BottomBarType::NONE:
      break;
    case BottomBarType::CHECKBOX:
      if (checkbox) {
        checkbox->SetFocus();
        return true;
      }
      break;
    case BottomBarType::ONE_BUTTON:
      if (button1) {
        button1->SetFocus();
        return true;
      }
      break;
    case BottomBarType::TWO_BUTTONS:
      if (button1) {
        button1->SetFocus();
        return true;
      }
      break;
    }
  }

  // Handle UP to move focus from bottom bar back to content
  if (key_code == KEY_UP) {
    bool bottom_has_focus =
      (checkbox && checkbox->HasFocus()) ||
      (button1 && button1->HasFocus()) ||
      (button2 && button2->HasFocus());
    if (bottom_has_focus) {
      scroll_widget->SetFocus();
      return true;
    }
  }

  // Handle LEFT/RIGHT between buttons in two-button mode
  if (bar_type == BottomBarType::TWO_BUTTONS) {
    if (key_code == KEY_RIGHT && button1 &&
        button1->HasFocus() && button2) {
      button2->SetFocus();
      return true;
    }
    if (key_code == KEY_LEFT && button2 &&
        button2->HasFocus() && button1) {
      button1->SetFocus();
      return true;
    }
  }

  return false;
}
