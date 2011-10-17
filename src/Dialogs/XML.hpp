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

#ifndef XCSOAR_DIALOGS_XML_HPP
#define XCSOAR_DIALOGS_XML_HPP

#include "Screen/SingleWindow.hpp"

#include <tchar.h>

struct DialogLook;
class Window;
class WndForm;
class SingleWindow;
class ContainerWindow;

/**
 * Class to hold callback entries for dialogs
 */
struct CallBackTableEntry
{
  const TCHAR *name;
  void *callback;
};

/**
 * Dialog display styles
 */
enum DialogStyle
{
  /** cover screen, stretch controls horizontally */
  dsFullWidth = 0,
  /** stretch only frame to maintain aspect ratio */
  dsScaled,
  /** like eDialogScaled but center dialog in screen */
  dsScaledCentered,
  /** don't adjust at all (same as !Layout::ScaleSupported()) */
  dsFixed,
  /** stretch horizontal and place to bottom */
  dsScaledBottom
};

extern DialogStyle dialog_style_setting;

/**
 * Sets the global dialog look for loading XML dialogs.
 */
void
SetXMLDialogLook(const DialogLook &dialog_look);

Window *
LoadWindow(const CallBackTableEntry *LookUpTable, WndForm *form,
           ContainerWindow &parent, const TCHAR *resource);

WndForm *
LoadDialog(const CallBackTableEntry *LookUpTable, SingleWindow &Parent,
               const TCHAR *resource, const PixelRect *targetRect = NULL);

#endif
