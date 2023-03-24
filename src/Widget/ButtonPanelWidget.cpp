// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ButtonPanelWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Form/ButtonPanel.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "util/Compiler.h"

#include <cassert>

ButtonPanelWidget::~ButtonPanelWidget() noexcept = default;

PixelRect
ButtonPanelWidget::UpdateLayout(const PixelRect &rc) noexcept
{
  assert(buttons != nullptr);

  switch (alignment) {
  case Alignment::AUTO:
    return buttons->UpdateLayout(rc);

  case Alignment::LEFT:
    return buttons->LeftLayout(rc);

  case Alignment::BOTTOM:
    return buttons->BottomLayout(rc);
  }

  gcc_unreachable();
}

PixelSize
ButtonPanelWidget::GetMinimumSize() const noexcept
{
  PixelSize size = widget->GetMinimumSize();
  if (size.height > 0)
    size.height += Layout::GetMinimumControlHeight();
  return size;
}

PixelSize
ButtonPanelWidget::GetMaximumSize() const noexcept
{
  PixelSize size = widget->GetMaximumSize();
  if (size.height > 0)
    size.height += Layout::GetMaximumControlHeight();
  return size;
}

void
ButtonPanelWidget::Initialise(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  assert(buttons == nullptr);

  buttons = std::make_unique<ButtonPanel>(parent, UIGlobals::GetDialogLook().button);
  buttons->SetDefaultHidden();

  /* initialise with full dimensions for now, buttons will be added
     later */
  widget->Initialise(parent, rc);
}

void
ButtonPanelWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  assert(buttons != nullptr);

  /* initialise with full dimensions for now, buttons may be added
     during this call, and the final layout will be set by Show() */
  widget->Prepare(parent, rc);
}

void
ButtonPanelWidget::Unprepare() noexcept
{
  assert(buttons != nullptr);

  widget->Unprepare();
}

bool
ButtonPanelWidget::Save(bool &changed) noexcept
{
  return widget->Save(changed);
}

bool
ButtonPanelWidget::Click() noexcept
{
  return widget->Click();
}

void
ButtonPanelWidget::ReClick() noexcept
{
  assert(buttons != nullptr);

  widget->ReClick();
}

void
ButtonPanelWidget::Show(const PixelRect &rc) noexcept
{
  assert(buttons != nullptr);

  buttons->ShowAll();
  widget->Show(UpdateLayout(rc));
}

bool
ButtonPanelWidget::Leave() noexcept
{
  return widget->Leave();
}

void
ButtonPanelWidget::Hide() noexcept
{
  buttons->HideAll();
  widget->Hide();
}

void
ButtonPanelWidget::Move(const PixelRect &rc) noexcept
{
  widget->Move(UpdateLayout(rc));
}

bool
ButtonPanelWidget::SetFocus() noexcept
{
  return widget->SetFocus();
}

bool
ButtonPanelWidget::HasFocus() const noexcept
{
  return widget->HasFocus() || buttons->HasFocus();
}

bool
ButtonPanelWidget::KeyPress(unsigned key_code) noexcept
{
  /* apply ButtonPanel hot keys and cursor navigation only any part of
     this widget is focused */
  return (HasFocus() && buttons->KeyPress(key_code)) ||
    widget->KeyPress(key_code);
}
