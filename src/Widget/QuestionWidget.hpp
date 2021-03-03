/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_QUESTION_WIDGET_HPP
#define XCSOAR_QUESTION_WIDGET_HPP

#include "SolidWidget.hpp"
#include "util/StaticArray.hxx"

#include <functional>

#include <tchar.h>

/**
 * A #Widget that displays a message and a number of buttons.  It is
 * used by XCSoar to display context-sensitive dialogs in the "bottom
 * area".
 */
class QuestionWidget : public SolidWidget {
  struct Button {
    const TCHAR *caption;
    std::function<void()> callback;
  };

  const TCHAR *const message;

  StaticArray<Button, 8> buttons;

public:
  explicit QuestionWidget(const TCHAR *_message) noexcept;

  void SetMessage(const TCHAR *_message) noexcept;

  void AddButton(const TCHAR *caption,
                 std::function<void()> callback) noexcept {
    buttons.append({caption, std::move(callback)});
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};

#endif
