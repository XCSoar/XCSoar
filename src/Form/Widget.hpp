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

#ifndef XCSOAR_WIDGET_HPP
#define XCSOAR_WIDGET_HPP

#include "Screen/Point.hpp"
#include "Compiler.h"

#include "assert.h"

class ContainerWindow;

/**
 * A Widget is an area on the screen that the user can interact with
 * or that just shows non-interactive data.  It can be implemented by
 * any Window derivative.  The Widget class acts as an adapter with a
 * well-defined interface.  It is used to split large forms into
 * smaller self-contained parts.  For example, a Widget can represent
 * the contents of one tab.
 */
class Widget {
public:
  /**
   * A virtual destructor prototype that allows containers to delete
   * Widget pointers.
   */
  virtual ~Widget();

  /**
   * Called as early as possible after the Widget has been added to
   * the container.  If needed, an implementation might decide to
   * create the Window here, but it is suggested to postpone it to
   * Prepare(), if possible.
   */
  virtual void Initialise(ContainerWindow &parent, const PixelRect &rc) = 0;

  /**
   * Called before the Widget is going to be shown for the first time.
   * The class should create the Window here, unless it has been
   * created already in Initialise().
   */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) = 0;

  /**
   * Save data that was modified by the user.  Will only be called if
   * Prepare() has been invoked previously.  Although this is likely
   * to be called just before destroying the widget, this should not
   * modify the widget.
   *
   * @return true on success, false if saving did not succeed (the
   * method should display a message to the user prior to returning)
   */
  virtual bool Save() = 0;

  /**
   * The user has clicked on the activation area (within the
   * container) of this Widget, which will show the widget.  The
   * Widget may now decide to do something else instead.
   *
   * This may be called before Prepare().
   *
   * @return true to continue, false to cancel the "Show" operation
   */
  virtual bool Click() = 0;

  /**
   * The user has clicked on the activation area while the Widget was
   * already visible.
   */
  virtual void ReClick() = 0;

  /**
   * Make the widget visible.  This will only be called after
   * Prepare(), and will not be called again until Hide() has been
   * called.
   */
  virtual void Show(const PixelRect &rc) = 0;

  /**
   * Make the widget invisible.  This will only be called when it is
   * visible already.
   */
  virtual void Hide() = 0;
};

/**
 * A Widget that implements a few optional methods as no-ops.  Aimed
 * to simplify Widget implementations that don't need all methods.
 */
class NullWidget : public Widget {
public:
  virtual ~NullWidget();

  virtual void Initialise(ContainerWindow &parent, const PixelRect &rc);
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save();
  virtual bool Click();
  virtual void ReClick();
};

#endif
