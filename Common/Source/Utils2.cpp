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

#include "Utils2.h"
#include "UtilsSystem.hpp"
#include "XCSoar.h"
#include <stdio.h>
#ifndef __MINGW32__
#if defined(CECORE)
#include "winbase.h"
#endif
#if (WINDOWSPC<1)
#include "projects.h"
#endif
#endif
#include "Settings.hpp"
#include "SettingsUser.hpp"
#include "InfoBoxLayout.h"
#include "Device/device.h"
#include "Logger.h"
#include "Dialogs.h"
#include "Utils.h"
#include "MapWindow.h"

/*
	Virtual Key Manager by Paolo Ventafridda

	Returns 0 if invalid virtual scan code, otherwise a valid transcoded keycode.

 */
int ProcessVirtualKey(int X, int Y, long keytime, short vkmode) {

// 0 is always thermal mode, and does not account
#define MAXBOTTOMMODES 5
#define VKTIMELONG 1500

	#ifdef DEBUG_PROCVK
	TCHAR buf[100];
	wsprintf(buf,_T("R=%d,%d,%d,%d, X=%d Y=%d kt=%ld"),MapWindow::MapRect.left, MapWindow::MapRect.top,
	MapWindow::MapRect.right, MapWindow::MapRect.bottom,X,Y,keytime);
	DoStatusMessage(buf);
	#endif

	short sizeup=MapWindow::MapRect.bottom-MapWindow::MapRect.top;
	short sizeright=MapWindow::MapRect.right-MapWindow::MapRect.left;
	short yup=(sizeup/3)+MapWindow::MapRect.top;
	short ydown=MapWindow::MapRect.bottom-(sizeup/3);
	short xleft=sizeright/3; // TODO FIX
	short xright=sizeright-xleft;

	if (Y<yup) {
		#ifndef DISABLEAUDIO
	        if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
		#endif
		if (keytime>=VKTIMELONG)
			return 0xc1;
		else
			return 38;
	}
	if (Y>ydown) {
		#ifndef DISABLEAUDIO
	        if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
		#endif
		if (keytime>=VKTIMELONG)
			return 0xc2;
		else

			return 40;
	}

		/*
		 * FIX ready: do not pass virtual ENTER while in Panmode.
		 * Currently it is allowed, should be better tested.  VNT 090702
		if ( !MapWindow::EnablePan ) {
	             DoStatusMessage(_T("Virtual ENTER"));
		     return 13;
		}
		return 0; // ignore it
		*/
	        return 13;
//	}
	DoStatusMessage(_T("VirtualKey Error"));
	return 0;
}


