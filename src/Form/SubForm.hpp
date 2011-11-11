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

#ifndef XCSOAR_SUB_FORM_HPP
#define XCSOAR_SUB_FORM_HPP

#include "Util/tstring.hpp"

#include <map>
#include <forward_list>

class Window;

/**
 * A SubForm is an object that manages the controls in a form.  It is
 * code that was originally in WndForm, but was moved out, to make
 * panels self-contained.
 */
class SubForm {
  struct tstring_less_than {
    bool operator()(const tstring &a, const tstring &b) const
    {
      return a.compare(b) < 0;
    }
  };

  typedef std::map<tstring, Window *, tstring_less_than> name_to_window_t;

  /**
   * List of windows which will be deleted by the destructor of this
   * class.
   */
  std::forward_list<Window *> destruct_windows;

  /**
   * Mapping of control names to #Window objects.
   */
  std::map<tstring, Window *, tstring_less_than> name_to_window;

  /**
   * List of windows which should only be visible in "advanced" mode.
   */
  std::forward_list<Window *> expert_windows;

public:
  ~SubForm();

public:
  /**
   * Add a #Window to the "destruct" list: the object will be deleted
   * by the destructor of this class.  This means that the caller
   * doesn't have to keep track of the specified Window, because this
   * WndForm is now responsible for freeing memory.
   */
  void AddDestruct(Window *window) {
    destruct_windows.push_front(window);
  }

  /**
   * Adds a #Window to the name-to-window map.
   */
  void AddNamed(const TCHAR *name, Window *window) {
    name_to_window[name] = window;
  }

  /**
   * Finds the ancestor window with the specified name.
   *
   * @param name the name of the #Window that is searched
   * @return the Window, or NULL if not found
   */
  Window *FindByName(const TCHAR *name);

  /**
   * Finds the ancestor window with the specified name.
   *
   * @param name the name of the #Window that is searched
   * @return the Window, or NULL if not found
   */
  const Window *FindByName(const TCHAR *name) const {
    return const_cast<SubForm *>(this)->FindByName(name);
  }

  /**
   * Adds a #Window to the "advanced window list" (#advanced_windows).
   */
  void AddExpert(Window *window) {
    expert_windows.push_front(window);
  }

  /**
   * Removes a #Window from the "advanced window list" (#advanced_windows).
   */
  void RemoveExpert(Window *window) {
    expert_windows.remove(window);
  }

  /**
   * Shows/Hides the ClientControls depending on the given value of advanced and
   * whether their caption includes an asterisk.
   * @param advanced True if advanced mode activated
   */
  void FilterAdvanced(bool advanced);
};

#endif
