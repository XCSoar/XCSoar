// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "KeyboardWidget.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/CharUtil.hxx"
#include "Screen/Layout.hpp"

#include <cassert>
#include <string.h>

static constexpr char keyboard_letters[] =
  "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void
KeyboardWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  PrepareSize(rc);

  char caption[] = " ";

  for (const char *i = keyboard_letters; !StringIsEmpty(i); ++i) {
    caption[0] = *i;
    AddButton(parent, caption, *i);
  }

  AddButton(parent, "Space", ' ');
  AddButton(parent, ".", '.');
  AddButton(parent, "@", '@');
  AddButton(parent, "-", '-');

  if (show_shift_button) {
    WindowStyle style;
    style.Hide();
    shift_button.Create(parent, { 0, 0, 16, 16 }, style,
                        std::make_unique<SymbolButtonRenderer>(look, "v"),
                        [this](){ OnShiftClicked(); });
  }
  UpdateShiftState();
}

void
KeyboardWidget::Show(const PixelRect &rc) noexcept
{
  OnResize(rc);

  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Show();

  if (show_shift_button)
    shift_button.Show();
}

void
KeyboardWidget::Hide() noexcept
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Hide();

  if (show_shift_button)
    shift_button.Hide();
}

void
KeyboardWidget::Move(const PixelRect &rc) noexcept
{
  OnResize(rc);
}

void
KeyboardWidget::SetAllowedCharacters(const char *allowed)
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].SetEnabled(allowed == nullptr ||
                          StringFind(allowed, buttons[i].GetCharacter()) != nullptr);
}

Button *
KeyboardWidget::FindButton(unsigned ch)
{
  for (unsigned i = 0; i < num_buttons; ++i)
    if (buttons[i].GetUpperCharacter() == ch)
      return &buttons[i];

  return nullptr;

}

/**
 * Move button to the specified left and top coordinates.
 *
 * The coordinates SHOULD BE in pixels of the screen (i.e. after scaling!)
 *
 * @param ch
 * @param left    Number of pixels from the left (in screen pixels)
 * @param top     Number of pixels from the top (in screen pixels)
 */
void
KeyboardWidget::MoveButton(unsigned ch, PixelPoint position) noexcept
{
  auto *kb = FindButton(ch);
  if (kb)
    kb->Move(position);
}

/**
 * Resizes the button to specified width and height values according to display pixels!
 *
 *
 * @param ch
 * @param width   Width measured in display pixels!
 * @param height  Height measured in display pixels!
 */
void
KeyboardWidget::ResizeButton(unsigned ch, PixelSize size) noexcept
{
  auto *kb = FindButton(ch);
  if (kb)
    kb->Resize(size);
}

void
KeyboardWidget::ResizeButtons()
{
  for (unsigned i = 0; i < num_buttons; ++i)
    buttons[i].Resize(button_size);

  if (show_shift_button)
    shift_button.Resize(button_size);
}

void
KeyboardWidget::MoveButtonsToRow(const PixelRect &rc,
                                 const char *buttons, unsigned row,
                                 int offset)
{
  if (StringIsEmpty(buttons))
    return;

  for (unsigned i = 0; buttons[i] != '\0'; i++) {
    MoveButton(buttons[i],
               rc.GetTopLeft() + PixelSize(i * button_size.width + offset,
                                           row * button_size.height));
  }
}

void
KeyboardWidget::MoveButtons(const PixelRect &rc)
{
  MoveButtonsToRow(rc, "1234567890", 0);
  MoveButtonsToRow(rc, "QWERTYUIOP", 1);
  MoveButtonsToRow(rc, "ASDFGHJKL", 2, button_size.width / 3);
  MoveButtonsToRow(rc, "ZXCVBNM@.", 3, button_size.width);

  if (IsLandscape(rc)) {
    MoveButton('-',
               rc.GetTopLeft() + PixelSize{button_size.width * 9, Layout::Scale(160U)});

    MoveButton(' ',
               rc.GetTopLeft() + PixelSize{Layout::Scale(80U), Layout::Scale(160U)});
    ResizeButton(' ', {Layout::Scale(93U), Layout::Scale(40U)});
  } else {
    MoveButton('-',
               rc.GetTopLeft() + PixelSize{button_size.width * 8, button_size.height * 4});

    MoveButton(' ',
               rc.GetTopLeft() + PixelSize{button_size.width * 2, button_size.height * 4});
    ResizeButton(' ', {button_size.width * 11 / 2, button_size.height});
  }

  if (show_shift_button)
    shift_button.Move(rc.GetTopLeft() + PixelSize{0U, 3 * button_size.height});
}

void
KeyboardWidget::PrepareSize(const PixelRect &rc)
{
  const PixelSize new_size = rc.GetSize();
  button_size = {new_size.width / 10, new_size.height / 5};
}

void
KeyboardWidget::OnResize(const PixelRect &rc)
{
  PrepareSize(rc);
  ResizeButtons();
  MoveButtons(rc);
}

void
KeyboardWidget::AddButton(ContainerWindow &parent,
                          const char *caption, unsigned ch)
{
  assert(num_buttons < MAX_BUTTONS);

  WindowStyle style;
  style.Hide();

  CharacterButton &button = buttons[num_buttons++];
  button.Create(parent, look, caption, PixelRect{button_size},
                on_character, ch, style);
}

void
KeyboardWidget::UpdateShiftState()
{
  if (show_shift_button)
    shift_button.SetCaption(shift_state ? "v" : "^");

  for (unsigned i = 0; i < num_buttons; ++i) {
    unsigned uch = buttons[i].GetCharacter();
    if (uch < 0x80) {
      char ch = char(uch);
      if (shift_state) {
        if (IsLowerAlphaASCII(ch))
          buttons[i].SetCharacter(ch - 0x20);
      } else {
        if (IsUpperAlphaASCII(ch))
          buttons[i].SetCharacter(ch + 0x20);
      }
    }
  }
}

void
KeyboardWidget::OnShiftClicked()
{
  assert(show_shift_button);

  shift_state = !shift_state;
  UpdateShiftState();
}
