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

class ContainerControl : public WindowControl {
protected:
  WindowControl *mClients[50];
  int mClientCount;

public:
  ContainerControl(ContainerControl *owner, ContainerWindow *parent,
                   const TCHAR *name, int x, int y, int width, int height,
                   bool visible = true)
    :WindowControl(owner, parent, name, x, y, width, height, visible),
     mClientCount(0) {}
  virtual ~ContainerControl();

public:
  virtual void AddClient(WindowControl *Client);

  virtual WindowControl *FindByName(const TCHAR *Name);

  const WindowControl *FindByName(const TCHAR *Name) const {
    return const_cast<ContainerControl *>(this)->FindByName(Name);
  }

  virtual Window *GetCanFocus(bool forward);

  virtual void FilterAdvanced(bool advanced);

public:
  Window *FocusNext(WindowControl *Sender);
  Window *FocusPrev(WindowControl *Sender);
};

#endif
