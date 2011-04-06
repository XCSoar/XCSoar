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
#include "Form/Form.hpp"

#include <stack>
#include <assert.h>

/**
 * The single top-level window of an application.  When it is closed,
 * the process quits.
 */
class SingleWindow : public TopWindow {
protected:
  std::stack<WndForm *> dialogs;

public:
  void add_dialog(WndForm *dialog);
  void remove_dialog(WndForm *dialog);

  /**
   * Forcefully cancel the top-most dialog.
   */
  void CancelDialog();

  bool has_dialog() {
    return !dialogs.empty();
  }

#ifdef ENABLE_SDL
protected:
  gcc_pure
  bool FilterMouseEvent(int x, int y, Window *allowed,
                        Window *second_allowed=NULL) const;
#endif

public:
  /**
   * Check if the specified event should be allowed.  An event may be
   * rejected when a modal dialog is active, and the event should go
   * to a window outside of the dialog.
   */
#ifdef ANDROID
  gcc_pure
  bool FilterEvent(const Event &event, Window *allowed,
                   Window *second_allowed=NULL) const;
#elif defined(ENABLE_SDL)
  gcc_pure
  bool FilterEvent(const SDL_Event &event, Window *allowed,
                   Window *second_allowed=NULL) const;
#else
  gcc_pure
  bool FilterEvent(const MSG &message, Window *allowed,
                   Window *second_allowed=NULL) const;
#endif

protected:
  virtual bool on_close();
  virtual bool on_destroy();
};

#endif
