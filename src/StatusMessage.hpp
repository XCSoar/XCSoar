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

#ifndef XCSOAR_STATUS_MESSAGE_H
#define XCSOAR_STATUS_MESSAGE_H

#include "Util/StaticArray.hpp"

#include <tchar.h>

class TLineReader;

/**
 * Struct used to store status message items
 */
typedef struct {
	const TCHAR *key;		/* English key */
	const TCHAR *sound;		/* What sound entry to play */
	bool doStatus;
	bool doSound;
	int delay_ms;		/* Delay for DoStatusMessage */
} StatusMessageSTRUCT;

/**
 * Class to manage a list of active and recent status messages
 */
class StatusMessageList {
  StaticArray<StatusMessageSTRUCT, 1000> StatusMessageData;
  int olddelay;

public:
  StatusMessageList();

  void LoadFile();
  void LoadFile(TLineReader &reader);

  void Startup(bool first);

  const StatusMessageSTRUCT &First() const {
    return StatusMessageData[0];
  }

  const StatusMessageSTRUCT *Find(const TCHAR *key) const;

private:
  void _init_Status(StatusMessageSTRUCT &m);
};

#endif
