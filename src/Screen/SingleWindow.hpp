/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_SINGLE_WINDOW_HXX
#define XCSOAR_SCREEN_SINGLE_WINDOW_HXX

#include "Screen/TopWindow.hpp"

#include <stack>
#include <assert.h>

/**
 * The single top-level window of an application.  When it is closed,
 * the process quits.
 */
class SingleWindow : public TopWindow {
protected:
  std::stack<Window *> dialogs;

public:
  void add_dialog(Window *dialog);
  void remove_dialog(Window *dialog);

  bool has_dialog() {
    return !dialogs.empty();
  }

protected:
  virtual bool on_close();
  virtual bool on_destroy();
};

#endif
