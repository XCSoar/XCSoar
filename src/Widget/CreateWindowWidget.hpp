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

#ifndef XCSOAR_CREATE_WINDOW_WIDGET_HPP
#define XCSOAR_CREATE_WINDOW_WIDGET_HPP

#include "WindowWidget.hpp"

#include <functional>
#include <memory>

class WindowStyle;

/**
 * A class that calls a function object to create the Window in
 * Prepare() and deletes the Window in Unprepare().
 */
class CreateWindowWidget final : public WindowWidget {
  typedef std::function<std::unique_ptr<Window>(ContainerWindow &parent,
                                                const PixelRect &rc,
                                                WindowStyle style)> CreateFunction;

  CreateFunction create;

public:
  CreateWindowWidget(CreateFunction &&_create)
    :create(std::move(_create)) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
};

#endif
