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

#include "Gauge/GaugeCDI.hpp"
#include "XCSoar.h"
#include "Interface.hpp"
#include "LogFile.hpp"
#include "Math/FastMath.h"
#include "InfoBoxLayout.h"
#include "Screen/Fonts.hpp"
#include "Screen/TextWindow.hpp"
#include "MainWindow.hpp"

#include <tchar.h>

TextWindow GaugeCDI::window;

void GaugeCDI::Create() {
  // start of new code for displaying CDI window
  StartupStore(TEXT("Create CDI\n"));

  window.set(main_window,
             (int)(InfoBoxLayout::ControlWidth*0.6),
             (int)(InfoBoxLayout::ControlHeight+1),
             (int)(InfoBoxLayout::ControlWidth*2.8),
             (int)(InfoBoxLayout::TitleHeight*1.4));
  window.insert_after(HWND_TOP);
  window.set_font(CDIWindowFont);

  // end of new code for drawing CDI window (see below for destruction of objects)

  window.hide();
}



void GaugeCDI::Destroy() {
  window.reset();
}

void GaugeCDI::Show()
{
  window.show();
}

void GaugeCDI::Hide()
{
  window.hide();
}

void GaugeCDI::Update(double TrackBearing, double WaypointBearing)
{
  // JMW changed layout here to fit reorganised display
  // insert waypoint bearing ".<|>." into CDIScale string"

  TCHAR CDIScale[] = TEXT("330..340..350..000..010..020..030..040..050..060..070..080..090..100..110..120..130..140..150..160..170..180..190..200..210..220..230..240..250..260..270..280..290..300..310..320..330..340..350..000..010..020..030..040.");
  TCHAR CDIDisplay[25] = TEXT("");
  int j;
  int CDI_WP_Bearing = (int)WaypointBearing/2;
  CDIScale[CDI_WP_Bearing + 9] = 46;
  CDIScale[CDI_WP_Bearing + 10] = 60;
  CDIScale[CDI_WP_Bearing + 11] = 124; // "|" character
  CDIScale[CDI_WP_Bearing + 12] = 62;
  CDIScale[CDI_WP_Bearing + 13] = 46;
  for (j=0;j<24;j++) CDIDisplay[j] = CDIScale[(j + (int)(TrackBearing)/2)];
  CDIDisplay[24] = _T('\0');
  // JMW fix bug! This indicator doesn't always display correctly!

  // JMW added arrows at end of CDI to point to track if way off..
  int deltacdi = iround(WaypointBearing - TrackBearing);

  while (deltacdi>180) {
    deltacdi-= 360;
  }
  while (deltacdi<-180) {
    deltacdi+= 360;
  }
  if (deltacdi>20) {
    CDIDisplay[21]='>';
    CDIDisplay[22]='>';
    CDIDisplay[23]='>';
  }
  if (deltacdi<-20) {
    CDIDisplay[0]='<';
    CDIDisplay[1]='<';
    CDIDisplay[2]='<';
  }

  window.set_text(CDIDisplay);
  // end of new code to display CDI scale
}
