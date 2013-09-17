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

#ifndef XCSOAR_DIALOGS_XML_HPP
#define XCSOAR_DIALOGS_XML_HPP

#include "Screen/Window.hpp"

#include <tchar.h>

struct CallBackTableEntry;
class SubForm;
class WndForm;
class SingleWindow;
class ContainerWindow;

/**
 * Loads a stand-alone XML file as a single top-level XML node
 * into an existing SubForm object and sets its parent to the parent parameter
 * Ignores additional top-level XML nodes.
 * Scales based on the DialogStyle of the last XML form loaded by XCSoar.
 * The Window is destroyed by its Form's destructor
 *
 * @param LookUpTable The CallBackTable
 * @param form The WndForm into which the Window is added
 * @param parent The parent window of the control being created
 *    set parent to "form-GetClientRect()" to make top level control
 *    or to a PanelControl to add it to a tab window
 * @param rc the rectangle within the parent for relative coordinates
 * @param FileName The XML filename
 * @return the pointer to the Window added to the form
 */
Window *
LoadWindow(const CallBackTableEntry *LookUpTable, SubForm *form,
           ContainerWindow &parent, const PixelRect &rc,
           const TCHAR *resource, WindowStyle style=WindowStyle());

/**
 * This function returns a WndForm created either from the ressources or
 * from the XML file in XCSoarData(if found)
 * @param LookUpTable The CallBackTable
 * @param FileName The XML filename to search for in XCSoarData
 * @param Parent The parent window (e.g. XCSoarInterface::main_window)
 * @param resource The resource to look for
 * @param targetRect The area where to move the dialog if not parent
 * @return The WndForm object
 */
WndForm *
LoadDialog(const CallBackTableEntry *LookUpTable, SingleWindow &Parent,
               const TCHAR *resource, const PixelRect *targetRect = NULL);

#endif
