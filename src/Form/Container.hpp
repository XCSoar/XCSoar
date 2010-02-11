/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_FORM_CONTAINER_HPP
#define XCSOAR_FORM_CONTAINER_HPP

#include "Form/Control.hpp"

/**
 * A ContainerControl is a special type of WindowControl,
 * that can contain other WindowControls itself.
 * It is for example the base class for WndForm.
 */
class ContainerControl : public WindowControl {
protected:
  /** Array of ClientControls */
  WindowControl *mClients[50];
  /** Number of ClientControls */
  unsigned mClientCount;

  /** the bottom of the most recently added child window */
  int bottom_most;

public:
  /**
   * Constructor for the ContainerControl class
   * @param owner
   * @param parent
   * @param x x-Coordinate of the ContainerControl
   * @param y y-Coordinate of the ContainerControl
   * @param width Width of the ContainerControl
   * @param height Height of the ContainerControl
   * @param visible True if the ContainerControl should be visible, False if not
   */
  ContainerControl(ContainerControl *owner, ContainerWindow *parent,
                   int x, int y, int width, int height,
                   const WindowStyle style)
    :WindowControl(owner, parent, x, y, width, height, style),
     mClientCount(0),
     bottom_most(0) {}
  /** Destructor */
  virtual ~ContainerControl();

public:
  /**
   * Add a ClientControl to the ContainerControl
   * @param Client A WindowControl to add as a ClientControl
   */
  virtual void AddClient(WindowControl *Client);

  /**
   * Shows/Hides the ClientControls depending on the given value of advanced and
   * whether their caption includes an asterisk.
   * @param advanced True if advanced mode activated
   */
  virtual void FilterAdvanced(bool advanced);
};

#endif
