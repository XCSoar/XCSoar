/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_DIALOGS_XML_HPP
#define XCSOAR_DIALOGS_XML_HPP

#include "Screen/Window.hpp"

#include <tchar.h>

struct DialogLook;
struct CallBackTableEntry;
class Window;
class SubForm;
class WndForm;
class SingleWindow;
class ContainerWindow;

/**
 * Sets the global dialog look for loading XML dialogs.
 */
void
SetXMLDialogLook(const DialogLook &dialog_look);

Window *
LoadWindow(const CallBackTableEntry *LookUpTable, SubForm *form,
           ContainerWindow &parent, const TCHAR *resource,
           WindowStyle style=WindowStyle());

WndForm *
LoadDialog(const CallBackTableEntry *LookUpTable, SingleWindow &Parent,
               const TCHAR *resource, const PixelRect *targetRect = NULL);

#endif
