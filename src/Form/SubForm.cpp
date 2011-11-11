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

#include "Form/SubForm.hpp"
#include "Screen/Window.hpp"

SubForm::~SubForm()
{
  for (window_list_t::iterator i = destruct_windows.begin();
       i != destruct_windows.end(); ++i)
    delete *i;
}

Window *
SubForm::FindByName(const TCHAR *name)
{
  name_to_window_t::iterator i = name_to_window.find(name);
  if (i == name_to_window.end())
    return NULL;

  return i->second;
}

void
SubForm::FilterAdvanced(bool advanced)
{
  for (window_list_t::const_iterator i = expert_windows.begin();
       i != expert_windows.end(); ++i)
    (*i)->set_visible(advanced);
}
