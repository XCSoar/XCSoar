/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#ifndef MESSAGE_H
#define MESSAGE_H

#include "PopupMessage.hpp"

class StatusMessageList;

class Message : public PopupMessage {
  static StatusMessageList status_messages;
  static PopupMessage *instance;

 public:
  static void Initialize(RECT rc);

  static void Destroy();

  static bool Render() {
    return instance->Render();
  }

  static void AddMessage(DWORD tshow, int type, TCHAR *Text) {
    instance->AddMessage(tshow, type, Text);
  }

  static void AddMessage(const TCHAR* text, const TCHAR *data=NULL) {
    instance->AddMessage(text, data);
  }

  // repeats last non-visible message of specified type (or any message
  // type=0)
  static void Repeat(int type) {
    instance->Repeat(type);
  }

  // clears all visible messages (of specified type or if type=0, all)
  static bool Acknowledge(int type) {
    return instance->Acknowledge(type);
  }

  static void CheckTouch(HWND wmControl) {
    instance->CheckTouch(wmControl);
  }

  static void BlockRender(bool doblock) {
    instance->BlockRender(doblock);
  }

  // used by main
  static void Startup(bool first);
  static void LoadFile(void);
  static void InitFile(void);
};

#endif
