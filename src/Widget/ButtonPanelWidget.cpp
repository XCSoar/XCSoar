/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "ButtonPanelWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Form/ButtonPanel.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"

#include <assert.h>

ButtonPanelWidget::~ButtonPanelWidget()
{
  delete buttons;
  delete widget;
}

PixelRect
ButtonPanelWidget::UpdateLayout(const PixelRect &rc)
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
ButtonPanelWidget::GetMinimumSize() const
{
  PixelSize size = widget->GetMinimumSize();
  if (size.cy > 0)
    size.cy += Layout::GetMinimumControlHeight();
  return size;
}

PixelSize
ButtonPanelWidget::GetMaximumSize() const
{
  PixelSize size = widget->GetMaximumSize();
  if (size.cy > 0)
    size.cy += Layout::GetMaximumControlHeight();
  return size;
}

void
ButtonPanelWidget::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  assert(buttons == nullptr);

  buttons = new ButtonPanel(parent, UIGlobals::GetDialogLook().button);
  buttons->SetDefaultHidden();

  /* initialise with full dimensions for now, buttons will be added
     later */
  widget->Initialise(parent, rc);
}

void
ButtonPanelWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  assert(buttons != nullptr);

  /* initialise with full dimensions for now, buttons may be added
     during this call, and the final layout will be set by Show() */
  widget->Prepare(parent, rc);
}

void
ButtonPanelWidget::Unprepare()
{
  assert(buttons != nullptr);

  widget->Unprepare();
}

bool
ButtonPanelWidget::Save(bool &changed)
{
  return widget->Save(changed);
}

bool
ButtonPanelWidget::Click()
{
  return widget->Click();
}

void
ButtonPanelWidget::ReClick()
{
  assert(buttons != nullptr);

  widget->ReClick();
}

void
ButtonPanelWidget::Show(const PixelRect &rc)
{
  assert(buttons != nullptr);

  widget->Show(UpdateLayout(rc));
  buttons->ShowAll();
}

bool
ButtonPanelWidget::Leave()
{
  return widget->Leave();
}

void
ButtonPanelWidget::Hide()
{
  buttons->HideAll();
  widget->Hide();
}

void
ButtonPanelWidget::Move(const PixelRect &rc)
{
  widget->Move(UpdateLayout(rc));
}

bool
ButtonPanelWidget::SetFocus()
{
  return widget->SetFocus();
}

bool
ButtonPanelWidget::KeyPress(unsigned key_code)
{
  return widget->KeyPress(key_code) || buttons->KeyPress(key_code);
}
