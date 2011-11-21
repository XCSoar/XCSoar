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

#include "Hardware/Display.hpp"

#ifdef WIN32
#include <windows.h>
#endif

#include <cstdio>

static void
PrintScreenSize()
{
#ifdef WIN32
  unsigned width = GetSystemMetrics(SM_CXSCREEN);
  unsigned height = GetSystemMetrics(SM_CYSCREEN);
  printf("Width: %d px | Height: %d px\n", width, height);
#endif
}

static void
PrintDPI()
{
  printf("DPI X: %d | DPI Y: %d\n", Display::GetXDPI(), Display::GetYDPI());
}

static void
PrintRotationInfo()
{
#if defined(DM_DISPLAYORIENTATION) && defined(_WIN32_WCE) && _WIN32_WCE >= 0x400
  DEVMODE dm;
  memset(&dm, 0, sizeof(dm));
  dm.dmSize = sizeof(dm);
  dm.dmFields = DM_DISPLAYQUERYORIENTATION;

  if (ChangeDisplaySettingsEx(NULL, &dm, NULL, CDS_TEST, NULL)
      != DISP_CHANGE_SUCCESSFUL) {
    printf("Display Rotation not supported\n");
    return;
  }

  printf("Display Rotation supported\n");

  DWORD dwSupported = dm.dmDisplayOrientation;

  printf("Supported Orientations: 0");
  if (DMDO_90 & dwSupported)
    printf(" 90");
  if (DMDO_180 & dwSupported)
    printf(" 180");
  if (DMDO_270 & dwSupported)
    printf(" 270");
  printf(" deg\n");

  dm.dmFields = DM_DISPLAYORIENTATION;
  if (ChangeDisplaySettingsEx(NULL, &dm, NULL, CDS_TEST, NULL)
      != DISP_CHANGE_SUCCESSFUL) {
    printf("Current Display Rotation not available\n");
    return;
  }

  printf("Current Display Rotation: ");
  if (dm.dmDisplayOrientation == DMDO_0)
    printf("0 deg");
  else if (dm.dmDisplayOrientation == DMDO_90)
    printf("90 deg");
  else if (dm.dmDisplayOrientation == DMDO_180)
    printf("180 deg");
  else if (dm.dmDisplayOrientation == DMDO_270)
    printf("270 deg");
  else
    printf("unknown");
  printf("\n");
#endif
}

int
main(int argc, char **argv)
{
  printf("Display Information\n\n");

  PrintScreenSize();
  PrintDPI();
  PrintRotationInfo();

#ifdef _WIN32_WCE
  // Console on WinCE is closing automatically
  // We prevent this by requesting user input
  getchar();
#endif

  return 0;
}
