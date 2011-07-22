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

#include "CommandLine.hpp"
#include "Profile/Profile.hpp"
#include "Simulator.hpp"

#include <windef.h> /* for MAX_PATH */

#if !defined(_WIN32_WCE)
int SCREENWIDTH = 640;
int SCREENHEIGHT = 480;
#endif

void
ParseCommandLine(const TCHAR *CommandLine)
{
  TCHAR extrnProfileFile[MAX_PATH];
  extrnProfileFile[0] = 0;

#ifdef SIMULATOR_AVAILABLE
  bool bSimTemp=false;
  bSimTemp = _tcsstr(CommandLine, _T("-simulator")) != NULL;
  if (bSimTemp) {
    global_simulator_flag=true;
    sim_set_in_cmd_line_flag=true;
  }
  bSimTemp = _tcsstr(CommandLine, _T("-fly")) != NULL;
  if (bSimTemp) {
    global_simulator_flag=false;
    sim_set_in_cmd_line_flag=true;
  }
#endif

  const TCHAR *pC, *pCe;

  pC = _tcsstr(CommandLine, _T("-profile="));
  if (pC != NULL) {
    pC += strlen("-profile=");
    if (*pC == '"') {
      pC++;
      pCe = pC;
      while (*pCe != '"' && *pCe != '\0')
        pCe++;
    } else {
      pCe = pC;
      while (*pCe != ' ' && *pCe != '\0')
        pCe++;
    }
    if (pCe != NULL && pCe - 1 > pC) {
      TCHAR *end = std::copy(pC, pCe, extrnProfileFile);
      *end = _T('\0');
    }
  }

  Profile::SetFiles(extrnProfileFile);

#if !defined(_WIN32_WCE)
  SCREENWIDTH = 640;
  SCREENHEIGHT = 480;

  #if defined(SCREENWIDTH_)
  SCREENWIDTH = SCREENWIDTH_;
  #endif
  #if defined(SCREENHEIGHT_)
  SCREENHEIGHT = SCREENHEIGHT_;
  #endif

  pC = _tcsstr(CommandLine, _T("-1024x768"));
  if (pC != NULL) {
    SCREENWIDTH = 1024;
    SCREENHEIGHT = 768;
  }

  pC = _tcsstr(CommandLine, _T("-800x480"));
  if (pC != NULL) {
    SCREENWIDTH = 800;
    SCREENHEIGHT = 480;
  }

  pC = _tcsstr(CommandLine, _T("-480x272"));
  if (pC != NULL) {
    SCREENWIDTH = 480;
    SCREENHEIGHT = 272;
  }

  pC = _tcsstr(CommandLine, _T("-272x480"));
  if (pC != NULL) {
    SCREENWIDTH = 272;
    SCREENHEIGHT = 480;
  }

  pC = _tcsstr(CommandLine, _T("-480x234"));
  if (pC != NULL) {
    SCREENWIDTH = 480;
    SCREENHEIGHT = 234;
  }

  pC = _tcsstr(CommandLine, _T("-320x480"));
  if (pC != NULL) {
    SCREENWIDTH = 320;
    SCREENHEIGHT = 480;
  }

  pC = _tcsstr(CommandLine, _T("-portrait"));
  if (pC != NULL) {
    SCREENWIDTH = 480;
    SCREENHEIGHT = 640;
  }

  pC = _tcsstr(CommandLine, _T("-square"));
  if (pC != NULL) {
    SCREENWIDTH = 480;
    SCREENHEIGHT = 480;
  }

  pC = _tcsstr(CommandLine, _T("-small"));
  if (pC != NULL) {
    SCREENWIDTH /= 2;
    SCREENHEIGHT /= 2;
  }

  pC = _tcsstr(CommandLine, _T("-320x240"));
  if (pC != NULL) {
    SCREENWIDTH = 320;
    SCREENHEIGHT = 240;
  }

  pC = _tcsstr(CommandLine, _T("-240x320"));
  if (pC != NULL) {
    SCREENWIDTH = 240;
    SCREENHEIGHT = 320;
  }
#endif
}
