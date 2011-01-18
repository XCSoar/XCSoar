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

#include "ButtonLabel.hpp"
#include "MenuBar.hpp"
#include "Language.hpp"

MenuBar *ButtonLabel::bar;

void
ButtonLabel::CreateButtonLabels(ContainerWindow &parent)
{
  bar = new MenuBar(parent);
}

void
ButtonLabel::SetFont(const Font &Font)
{
  bar->SetFont(Font);
}

void
ButtonLabel::Destroy()
{
  delete bar;
}

void
ButtonLabel::SetLabelText(unsigned index, const TCHAR *text)
{
  if ((text == NULL) || (*text == _T('\0')) || (*text == _T(' '))) {
    bar->HideButton(index);
  } else {
    TCHAR s[100];

    bool greyed = ExpandMacros(text, s, sizeof(s) / sizeof(s[0]));

    if ((s[0] == _T('\0')) || (s[0] == _T(' '))) {
      bar->HideButton(index);
    } else {
      bar->ShowButton(index, !greyed, gettext(s));
    }
  }
}

bool
ButtonLabel::IsEnabled(unsigned i)
{
  return bar->IsButtonEnabled(i);
}
