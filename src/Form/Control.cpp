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

#include "Form/Control.hpp"
#include "Form/Internal.hpp"
#include "Screen/Key.h"
#include "Dialogs/HelpDialog.hpp"

#include <stdlib.h>

WindowControl::WindowControl() :
    help_text(NULL),
    help_callback(NULL)
{
  // Clear the caption
  caption.clear();
}

WindowControl::~WindowControl()
{
  free(help_text);
}

void
WindowControl::SetHelpText(const TCHAR *Value)
{
  free(help_text);
  help_text = Value != NULL ? _tcsdup(Value) : NULL;
}

void
WindowControl::SetCaption(const TCHAR *Value)
{
  if (Value == NULL)
    Value = _T("");

  if (!caption.equals(Value)) {
    caption = Value;
    Invalidate();
  }
}

bool
WindowControl::OnHelp()
{
  if (help_text) {
    dlgHelpShowModal(*(SingleWindow *)GetRootOwner(),
                     caption.c_str(), help_text);
    return true;
  }

  if (help_callback) {
    (help_callback)(this);
    return true;
  }

  return false;
}

bool
WindowControl::OnKeyDown(unsigned key_code)
{
  KeyTimer(true, key_code);

  return ContainerWindow::OnKeyDown(key_code);
}

bool
WindowControl::OnKeyUp(unsigned key_code)
{
  // JMW: detect long enter release
  // VENTA4: PNAs don't have Enter, so it should be better to find an alternate solution
  // activate tool tips if hit return for long time
  if (KeyTimer(false, key_code) && key_code == KEY_RETURN && OnHelp())
    return true;

  return ContainerWindow::OnKeyUp(key_code);
}

void
WindowControl::OnSetFocus()
{
  ContainerWindow::OnSetFocus();
  Invalidate();
}

void
WindowControl::OnKillFocus()
{
  ContainerWindow::OnKillFocus();
  Invalidate();
}
