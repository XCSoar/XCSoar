// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Widget.hpp"
#include "ui/dim/Size.hpp"

PixelSize
NullWidget::GetMinimumSize() const noexcept
{
  return PixelSize{0,0};
}

PixelSize
NullWidget::GetMaximumSize() const noexcept
{
  /* map to GetMinimumSize() by default, so simple derived classes
     need to implement only GetMinimumSize() */
  return GetMinimumSize();
}

void
NullWidget::Initialise([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
}

void
NullWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
}

void
NullWidget::Unprepare() noexcept
{
}

bool
NullWidget::Save([[maybe_unused]] bool &changed) noexcept
{
  return true;
}

bool
NullWidget::Click() noexcept
{
  return true;
}

void
NullWidget::ReClick() noexcept
{
}

bool
NullWidget::Leave() noexcept
{
  return true;
}

void
NullWidget::Move(const PixelRect &rc) noexcept
{
  Hide();
  Show(rc);
}

bool
NullWidget::SetFocus() noexcept
{
  return false;
}

bool
NullWidget::HasFocus() const noexcept
{
  return false;
}

bool
NullWidget::KeyPress([[maybe_unused]] unsigned key_code) noexcept
{
  return false;
}
