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

#include "Form/Container.hpp"

ContainerControl::~ContainerControl()
{
  for (int i = mClientCount - 1; i >= 0; i--)
    delete mClients[i];
}

void
ContainerControl::AddClient(WindowControl *Client)
{
  mClients[mClientCount] = Client;
  mClientCount++;

  Client->SetFont(GetFont());

  if (Client->get_position().top == -1 && mClientCount > 1)
    Client->move(Client->get_position().left,
                 mClients[mClientCount - 2]->get_position().bottom);

  /*
  // TODO code: also allow autosizing of height/width to maximum of parent

  if (Client->mHeight == -1){
    // maximum height
    Client->mHeight = mHeight - Client->mY;
    SetWindowPos(Client->GetHandle(), 0,
                 Client->mX, Client->mY,
                 Client->mWidth, Client->mHeight,
                 SWP_NOSIZE | SWP_NOZORDER
                 | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
  }
  */
}

WindowControl *
ContainerControl::FindByName(const TCHAR *Name)
{
  WindowControl *w = WindowControl::FindByName(Name);
  if (w != NULL)
    return w;

  for (unsigned i = 0; i < mClientCount; i++) {
    w = mClients[i]->FindByName(Name);
    if (w != NULL)
      return w;
  }

  return NULL;
}

Window *
ContainerControl::GetCanFocus(bool forward)
{
  if (!is_visible())
    return NULL;

  Window *w = WindowControl::GetCanFocus(forward);
  if (w != NULL)
    return w;

  if (forward) {
    for (unsigned idx = 0; idx < mClientCount; ++idx) {
      Window *w = mClients[idx]->GetCanFocus(forward);
      if (w != NULL)
        return w;
    }
  } else {
    for (int idx = mClientCount - 1; idx >= 0; --idx) {
      Window *w = mClients[idx]->GetCanFocus(forward);
      if (w != NULL)
        return w;
    }
  }

  return NULL;
}

void
ContainerControl::FilterAdvanced(bool advanced)
{
  WindowControl::FilterAdvanced(advanced);

  for (unsigned i = 0; i < mClientCount; i++)
    mClients[i]->FilterAdvanced(advanced);
}

Window *
ContainerControl::FocusNext(WindowControl *Sender)
{
  unsigned idx;
  Window *W;

  if (Sender != NULL) {
    for (idx = 0; idx < mClientCount; idx++)
      if (mClients[idx] == Sender)
        break;

    idx++;
  } else
    idx = 0;

  for (; idx < mClientCount; idx++) {
    if ((W = mClients[idx]->GetCanFocus(true)) != NULL) {
      W->set_focus();
      return W;
    }
  }

  if (GetOwner() != NULL)
    return GetOwner()->FocusNext(this);

  return NULL;
}

Window *
ContainerControl::FocusPrev(WindowControl *Sender)
{
  int idx;
  Window *W;

  if (Sender != NULL) {
    for (idx = 0; (unsigned)idx < mClientCount; idx++) {
      if (mClients[idx] == Sender)
        break;
    }
    idx--;
  } else
    idx = mClientCount - 1;

  for (; idx >= 0; idx--) {
    if ((W = mClients[idx]->GetCanFocus(false)) != NULL) {
      W->set_focus();
      return W;
    }
  }

  if (GetOwner() != NULL)
    return GetOwner()->FocusPrev(this);

  return NULL;
}
