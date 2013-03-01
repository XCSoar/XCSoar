/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_OFFSET_BUTTONS_WIDGET_HPP
#define XCSOAR_OFFSET_BUTTONS_WIDGET_HPP

#include "Widget.hpp"
#include "Form/ActionListener.hpp"
#include "Math/fixed.hpp"

#include <tchar.h>

struct DialogLook;
class ActionListener;
class WndButton;

/**
 * Show four buttons to increment/decrement a value.
 */
class OffsetButtonsWidget : public NullWidget, private ActionListener {
  const DialogLook &look;
  const TCHAR *const format;
  const fixed offsets[4];
  WndButton *buttons[4];

public:
  OffsetButtonsWidget(const DialogLook &_look, const TCHAR *_format,
                      fixed small_offset, fixed large_offset)
    :look(_look), format(_format),
     offsets{-large_offset, -small_offset, small_offset, large_offset} {}

public:
  /* virtual methods from Widget */
  virtual PixelSize GetMinimumSize() const override;
  virtual PixelSize GetMaximumSize() const override;
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;
  virtual void Unprepare() override;
  virtual void Show(const PixelRect &rc) override;
  virtual void Hide() override;
  virtual void Move(const PixelRect &rc) override;
  virtual bool SetFocus() override;

protected:
  virtual void OnOffset(fixed offset) = 0;

private:
  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;
};

#endif
