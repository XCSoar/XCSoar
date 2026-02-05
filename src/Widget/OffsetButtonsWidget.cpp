// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OffsetButtonsWidget.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>  // for std::any_of
#include <stdio.h>

PixelSize
OffsetButtonsWidget::GetMinimumSize() const noexcept
{
  return { 4u * Layout::GetMinimumControlHeight(),
      Layout::GetMinimumControlHeight() };
}

PixelSize
OffsetButtonsWidget::GetMaximumSize() const noexcept
{
  return { 4u * Layout::GetMaximumControlHeight(),
      Layout::GetMaximumControlHeight() };
}

static constexpr std::array<PixelRect, 4>
LayoutOffsetButtons(const PixelRect &total_rc) noexcept
{
  const unsigned total_width = total_rc.GetWidth();
  PixelRect rc = { 0, total_rc.top, total_rc.left, total_rc.bottom };

  std::array<PixelRect, 4> buttons{};
  for (unsigned i = 0; i < 4; ++i) {
    rc.left = rc.right;
    rc.right = total_rc.left + (i + 1) * total_width / 4;
    buttons[i] = rc;
  }

  return buttons;
}

inline Button
OffsetButtonsWidget::MakeButton(ContainerWindow &parent, const PixelRect &r,
                                unsigned i) noexcept
{
  char caption[16];
  _stprintf(caption, format, offsets[i]);

  WindowStyle style;
  style.TabStop();
  style.Hide();

  return Button(parent, look, caption, r, style, [this, i](){
    OnOffset(offsets[i]);
  });
}

inline std::array<Button, 4>
OffsetButtonsWidget::MakeButtons(ContainerWindow &parent,
                                 const PixelRect &total) noexcept
{
  const auto r = LayoutOffsetButtons(total);

  return {
    MakeButton(parent, r[0], 0),
    MakeButton(parent, r[1], 1),
    MakeButton(parent, r[2], 2),
    MakeButton(parent, r[3], 3),
  };
}

void
OffsetButtonsWidget::Prepare(ContainerWindow &parent,
                             const PixelRect &total_rc) noexcept
{
  buttons.reset(new std::array<Button, 4>{MakeButtons(parent, total_rc)});
}

void
OffsetButtonsWidget::Show(const PixelRect &total_rc) noexcept
{
  const auto rc = LayoutOffsetButtons(total_rc);

  for (unsigned i = 0; i < buttons->size(); ++i)
    (*buttons)[i].MoveAndShow(rc[i]);
}

void
OffsetButtonsWidget::Hide() noexcept
{
  for (auto &i : *buttons)
    i.Hide();
}

void
OffsetButtonsWidget::Move(const PixelRect &total_rc) noexcept
{
  const auto rc = LayoutOffsetButtons(total_rc);

  for (unsigned i = 0; i < buttons->size(); ++i)
    (*buttons)[i].Move(rc[i]);
}

bool
OffsetButtonsWidget::SetFocus() noexcept
{
  (*buttons)[2].SetFocus();
  return true;
}

bool
OffsetButtonsWidget::HasFocus() const noexcept
{
  return std::any_of(buttons->begin(), buttons->end(), [](const Button &b){
    return b.HasFocus();
  });
}
