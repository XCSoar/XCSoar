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

#include "MapWindow.h"
#include "Interface.hpp"
#include "Protection.hpp"
#include "Screen/Blank.hpp"

bool MapWindowBase::IsDisplayRunning() {
  return (globalRunningEvent.test()
	  && !ScreenBlanked);
}

void MapWindowBase::CreateDrawingThread(void)
{
  bool has_initialised = false;
  do {
    mutexStart.Lock(); // wait for display to start
    has_initialised |= window_initialised;
    mutexStart.Unlock();
    Sleep(100);
  } while (!window_initialised);

  closeTriggerEvent.reset();
  hDrawThread = CreateThread (NULL, 0,
                              (LPTHREAD_START_ROUTINE )
			      MapWindow::DrawThread,
                              (MapWindow *)this, 0, &dwDrawThreadID);
  SetThreadPriority(hDrawThread,THREAD_PRIORITY_NORMAL);
}

void MapWindowBase::SuspendDrawingThread(void)
{
  mutexRun.Lock();
}

void MapWindowBase::ResumeDrawingThread(void)
{
  mutexRun.Unlock();
}

void MapWindowBase::CloseDrawingThread(void)
{
  closeTriggerEvent.trigger();
  drawTriggerEvent.trigger(); // wake self up
  SuspendDrawingThread();
  WaitForSingleObject(hDrawThread, INFINITE);
}
