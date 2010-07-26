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

#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Protection.hpp"
#include "InfoBoxes/Content/Factory.hpp"
#include "InputEvents.h"
#include "Compatibility/string.h"
#include "Screen/Blank.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Hardware/Battery.h"
#include "MainWindow.hpp"
#include "Appearance.hpp"

#include <assert.h>

#include <algorithm>

using std::min;

BufferWindow InfoBoxManager::full_window;

static bool InfoBoxesDirty = false;
static bool InfoBoxesHidden = false;

InfoBoxWindow *InfoBoxes[MAXINFOWINDOWS];
const unsigned NUMSELECTSTRINGS = 75;

static InfoBoxLook info_box_look;

static const int InfoTypeDefault[MAXINFOWINDOWS] = {
  0x0E0E0E,
  0x0B1215,
  0x040000,
  0x012316,
  0x0A0A0A,
  0x222223,
  0x060606,
  0x191919
};

static const int InfoTypeAltairDefault[MAXINFOWINDOWS] = {
  0x340E0E0E,
  0x33120B0B,
  0x31030316,
  0x002B2B31,
  0x06263030,
  0x19212121,
  0x27291107,
  0x250F0F0F,
  0x1A2D2D2D
};

static int InfoType[MAXINFOWINDOWS];

typedef struct _SCREEN_INFO
{
  const TCHAR *Description;
  char next_screen;
  char prev_screen;
} SCREEN_INFO;

// Groups:
//   Altitude 0,1,20,33
//   Aircraft info 3,6,23,32,37,47,54
//   LD 4,5,19,38,53,66
//   Vario 2,7,8,9,21,22,24,44
//   Wind 25,26,48,49,50
//   MacCready 10,34,35,43
//   Nav 11,12,13,15,16,17,18,27,28,29,30,31
//   Waypoint 14,36,39,40,41,42,45,46
static const SCREEN_INFO Data_Options[] = {
  // 0
  { _T("Height GPS"), 1, 33, },
  // 1
  { _T("Height AGL"), 20, 0, },
  // 2
  { _T("Thermal last 30 sec"), 7, 44, },
  // 3
  { _T("Bearing"), 6, 54, },
  // 4
  { _T("L/D instantaneous"), 5, 38, },
  // 5
  { _T("L/D cruise"), 19, 4, },
  // 6
  { _T("Speed ground"), 23, 3, },
  // 7
  { _T("Last Thermal Average"), 8, 2, },
  // 8
  { _T("Last Thermal Gain"), 9, 7, },
  // 9
  { _T("Last Thermal Time"), 21, 8, },
  // 10
  { _T("MacCready Setting"), 34, 43, },
  // 11
  { _T("Next Distance"), 12, 31, },
  // 12
  { _T("Next Altitude Difference"), 13, 11, },
  // 13
  { _T("Next Altitude Required"), 15, 12, },
  // 14
  { _T("Next Waypoint"), 36, 46, },
  // 15
  { _T("Final Altitude Difference"), 16, 13, },
  // 16
  { _T("Final Altitude Required"), 17, 15, },
  // 17
  { _T("Speed Task Average"), 18, 16, },
  // 18
  { _T("Final Distance"), 27, 17, },
  // 19
  { _T("Final LD"), 38, 5, },
  // 20
  { _T("Terrain Elevation"), 33, 1, },
  // 21
  { _T("Thermal Average"), 22, 9, },
  // 22
  { _T("Thermal Gain"), 24, 21, },
  // 23
  { _T("Track"), 32, 6, },
  // 24
  { _T("Vario"), 44, 22, },
  // 25
  { _T("Wind Speed"), 26, 50, },
  // 26
  { _T("Wind Bearing"), 48, 25, },
  // 27
  { _T("AA Time"), 28, 18, },
  // 28
  { _T("AA Distance Max"), 29, 27, },
  // 29
  { _T("AA Distance Min"), 30, 28, },
  // 30
  { _T("AA Speed Max"), 31, 29, },
  // 31
  { _T("AA Speed Min"), 51, 30, },
  // 32
  { _T("Airspeed IAS"), 37, 23, },
  // 33
  { _T("Pressure Altitude"), 0, 20, },
  // 34
  { _T("Speed MacCready"), 35, 10, },
  // 35
  { _T("Percentage climb"), 43, 34, },
  // 36
  { _T("Time of flight"), 39, 14, },
  // 37
  { _T("G load"), 47, 32, },
  // 38
  { _T("Next LD"), 53, 19, },
  // 39
  { _T("Time local"), 40, 36, },
  // 40
  { _T("Time UTC"), 41, 39, },
  // 41
  { _T("Task Time To Go"), 42, 40, },
  // 42
  { _T("Next Time To Go"), 45, 41, },
  // 43
  { _T("Speed Dolphin"), 10, 35, },
  // 44
  { _T("Netto Vario"), 2, 24, },
  // 45
  { _T("Task Arrival Time"), 46, 42, },
  // 46
  { _T("Next Arrival Time"), 14, 45, },
  // 47
  { _T("Bearing Difference"), 54, 37, },
  // 48
  { _T("Outside Air Temperature"), 49, 26, },
  // 49
  { _T("Relative Humidity"), 50, 48, },
  // 50
  { _T("Forecast Temperature"), 49, 25, },
  // 51
  { _T("AA Distance Tgt"), 52, 31, },
  // 52
  { _T("AA Speed Tgt"), 11, 51, },
  // 53
  { _T("L/D vario"), 4, 38, },
  // 54
  { _T("Airspeed TAS"), 3, 47, },
  // 55
  { _T("Own Team Code"), 56, 54, },
  // 56
  { _T("Team Bearing"), 57, 55, },
  // 57
  { _T("Team Bearing Diff"), 58, 56, },
  // 58
  { _T("Team Range"), 55, 57, },
  // 59
  { _T("Speed Task Instantaneous"), 18, 16, },
  // 60
  { _T("Distance Home"), 18, 16, },
  // 61
  { _T("Speed Task Achieved"), 18, 16, },
  // 62
  { _T("AA Delta Time"), 28, 18, },
  // 63
  { _T("Thermal All"), 8, 2, },
  // 64
  { _T("Distance Vario"), 8, 2, },
  // 65
#ifndef GNAV
  { _T("Battery Percent"), 49, 26, },
#else
  { _T("Battery Voltage"), 49, 26, },
#endif
  // 66  VENTA-ADDON added Final GR
  // VENTA-TODO: fix those 38,5 numbers to point correctly menu items
  { _T("Final GR"), 38, 5, },
  // 67 VENTA3-ADDON Alternate1 destinations infoboxes  TODO> fix 36 46 to something correct
  { _T("Alternate1 GR"), 36, 46, },
  // 68 Alternate 2
  { _T("Alternate2 GR"), 36, 46, },
  // 69 BestAlternate aka BestLanding
  { _T("Best Alternate"), 36, 46, },
  // 70
  { _T("QFE GPS"), 1, 33, },
  // 71 TODO FIX those 19,4 values
  { _T("L/D Average"), 19, 4, },
  // 72 //
  { _T("Experimental1"), 8, 2, },
  // 73 //
  { _T("Online Contest Distance"), 8, 2, },
  // 74 //
  { _T("Experimental2"), 8, 2, },
};

// TODO locking
void
InfoBoxManager::Hide()
{
  InfoBoxesHidden = true;

  for (unsigned i = 0; i < InfoBoxLayout::numInfoWindows; i++)
    InfoBoxes[i]->hide();

  full_window.hide();
}

void
InfoBoxManager::Show()
{
  InfoBoxesHidden = false;

  for (unsigned i = 0; i < InfoBoxLayout::numInfoWindows; i++)
    InfoBoxes[i]->show();
}

int
InfoBoxManager::GetFocused()
{
  for (unsigned i = 0; i < InfoBoxLayout::numInfoWindows; i++)
    if (InfoBoxes[i]->has_focus())
      return i;

  return -1;
}

void
InfoBoxManager::Event_Select(int i)
{
  int InfoFocus = GetFocused();

  if (InfoFocus < 0) {
    InfoFocus = (i >= 0 ? 0 : InfoBoxLayout::numInfoWindows - 1);
  } else {
    InfoFocus += i;

    if (InfoFocus < 0 || (unsigned)InfoFocus >= InfoBoxLayout::numInfoWindows)
      InfoFocus = -1;
  }

  if (InfoFocus >= 0)
    main_window.map.set_focus();
  else
    InfoBoxes[i]->set_focus();
}

int
InfoBoxManager::getType(unsigned i, unsigned layer)
{
  assert(i < MAXINFOWINDOWS);
  assert(layer < 4);

  switch (layer) {
  case 0:
    return InfoType[i] & 0xff; // climb
  case 1:
    return (InfoType[i] >> 8) & 0xff; // cruise
  case 2:
    return (InfoType[i] >> 16) & 0xff; // final glide
  case 3:
    return (InfoType[i] >> 24) & 0xff; // auxiliary
  }

  return 0xdeadbeef; /* not reachable */
}

int
InfoBoxManager::getTypeAll(unsigned i)
{
  assert(i < MAXINFOWINDOWS);

  return InfoType[i];
}

void
InfoBoxManager::setTypeAll(unsigned i, unsigned j)
{
  assert(i < MAXINFOWINDOWS);

  InfoType[i] = j;
  // TODO: check it's within range
}

int
InfoBoxManager::getType(unsigned i)
{
  unsigned retval = 0;

  if (SettingsMap().EnableAuxiliaryInfo)
    retval = getType(i, 3);
  else if (MapProjection().GetDisplayMode() == dmCircling)
    retval = getType(i, 0);
  else if (MapProjection().GetDisplayMode() == dmFinalGlide)
    retval = getType(i, 2);
  else
    retval = getType(i, 1);

  return min(NUMSELECTSTRINGS - 1, retval);
}

bool
InfoBoxManager::IsEmpty(unsigned mode)
{
  for (unsigned i = 0; i < MAXINFOWINDOWS; ++i)
    if (InfoBoxManager::getType(i, mode) != 0)
      return false;

  return true;
}

bool
InfoBoxManager::IsEmpty()
{
  for (unsigned i = 0; i < MAXINFOWINDOWS; ++i)
    if (InfoBoxManager::getTypeAll(i) != 0)
      return false;

  return true;
}

void
InfoBoxManager::setType(unsigned i, char j, unsigned layer)
{
  assert(i < MAXINFOWINDOWS);

  switch (layer) {
  case 0:
    InfoType[i] &= 0xffffff00;
    InfoType[i] += j;
    break;
  case 1:
    InfoType[i] &= 0xffff00ff;
    InfoType[i] += (j << 8);
    break;
  case 2:
    InfoType[i] &= 0xff00ffff;
    InfoType[i] += (j << 16);
    break;
  case 3:
    InfoType[i] &= 0x00ffffff;
    InfoType[i] += (j << 24);
    break;
  }
}

void
InfoBoxManager::setType(unsigned i, char j)
{
  if (SettingsMap().EnableAuxiliaryInfo)
    setType(i, j, 3);
  else if (MapProjection().GetDisplayMode() == dmCircling)
    setType(i, j, 0);
  else if (MapProjection().GetDisplayMode() == dmFinalGlide)
    setType(i, j, 2);
  else
    setType(i, j, 1);
}

void
InfoBoxManager::Event_Change(int i)
{
  int j = 0, k;

  int InfoFocus = GetFocused();
  if (InfoFocus < 0)
    return;

  k = getType(InfoFocus);
  if (i > 0)
    j = Data_Options[k].next_screen;
  else if (i < 0)
    j = Data_Options[k].prev_screen;

  // TODO code: if i==0, go to default or reset

  setType(InfoFocus, j);

  Update(*InfoBoxes[InfoFocus], j, true);
  Paint();
}

void
InfoBoxManager::Update(InfoBoxWindow &info_box, unsigned type, bool needupdate)
{
  if (info_box.UpdateContent())
    return;
}

void
InfoBoxManager::DisplayInfoBox()
{
  if (InfoBoxesHidden)
    return;

  int DisplayType[MAXINFOWINDOWS];
  static bool first = true;
  static int DisplayTypeLast[MAXINFOWINDOWS];

  // JMW note: this is updated every GPS time step

  for (unsigned i = 0; i < InfoBoxLayout::numInfoWindows; i++) {
    // All calculations are made in a separate thread. Slow calculations
    // should apply to the function DoCalculationsSlow()
    // Do not put calculations here!

    DisplayType[i] = getType(i);

    bool needupdate = ((DisplayType[i] != DisplayTypeLast[i]) || first);

    if (needupdate)
      InfoBoxes[i]->SetContentProvider(InfoBoxFactory::Create(DisplayType[i]));

    Update(*InfoBoxes[i], DisplayType[i], needupdate);

    DisplayTypeLast[i] = DisplayType[i];
  }

  Paint();

  first = false;
}

void
InfoBoxManager::ProcessKey(InfoBoxContent::InfoBoxKeyCodes keycode)
{
  int focus = GetFocused();
  if (focus < 0)
    return;

  if (InfoBoxes[focus] != NULL)
    InfoBoxes[focus]->HandleKey(keycode);

  InputEvents::HideMenu();

  SetDirty();

  // emulate update to trigger calculations
  TriggerGPSUpdate();

  ResetDisplayTimeOut();
}

bool
InfoBoxManager::HasFocus()
{
  return GetFocused() >= 0;
}

void
InfoBoxManager::InfoBoxDrawIfDirty()
{
  // No need to redraw map or infoboxes if screen is blanked.
  // This should save lots of battery power due to CPU usage
  // of drawing the screen

  if (InfoBoxesDirty && !SettingsMap().ScreenBlanked) {
    DisplayInfoBox();
    InfoBoxesDirty = false;
  }
}

void
InfoBoxManager::SetDirty()
{
  InfoBoxesDirty = true;
}

void
InfoBoxManager::ProcessTimer()
{
  static fixed lasttime;

  if (Basic().Time != lasttime) {
    SetDirty();
    lasttime = Basic().Time;
  }

  InfoBoxDrawIfDirty();
}

void InfoBoxManager::ResetInfoBoxes() {
  memcpy(InfoType,
         is_altair() ? InfoTypeAltairDefault : InfoTypeDefault,
         sizeof(InfoType));
}

const TCHAR *
InfoBoxManager::GetTypeDescription(unsigned i)
{
  return Data_Options[i].Description;
}

void
InfoBoxManager::Paint()
{
  if (!InfoBoxLayout::fullscreen) {
    full_window.hide();

    for (unsigned i = 0; i < InfoBoxLayout::numInfoWindows; i++)
      InfoBoxes[i]->invalidate();
  } else {
    Canvas &canvas = full_window.get_canvas();

    canvas.clear_white();

    for (unsigned i = 0; i < InfoBoxLayout::numInfoWindows; i++) {
      // JMW TODO: make these calculated once only.
      int x, y;
      int rx, ry;
      int rw;
      int rh;
      double fw, fh;

      if (Layout::landscape) {
        rw = 84;
        rh = 68;
      } else {
        rw = 120;
        rh = 80;
      }

      fw = rw / (double)InfoBoxLayout::ControlWidth;
      fh = rh / (double)InfoBoxLayout::ControlHeight;

      double f = min(fw, fh);
      rw = (int)(f * InfoBoxLayout::ControlWidth);
      rh = (int)(f * InfoBoxLayout::ControlHeight);

      if (Layout::landscape) {
        rx = i % 3;
        ry = i / 3;

        x = (rw + 4) * rx;
        y = (rh + 3) * ry;
      } else {
        rx = i % 2;
        ry = i / 4;

        x = (rw) * rx;
        y = (rh) * ry;
      }

      InfoBoxes[i]->PaintInto(canvas, IBLSCALE(x), IBLSCALE(y),
                              IBLSCALE(rw), IBLSCALE(rh));
    }

    full_window.invalidate();
    full_window.show();
  }
}

int
InfoBoxManager::GetInfoBoxBorder(unsigned i)
{
  if (Appearance.InfoBoxBorder == apIbTab)
    return 0;

  if (InfoBoxLayout::InfoBoxGeometry == InfoBoxLayout::ibGNav) {
    if (i < 6)
      return BORDERTOP | BORDERRIGHT;
    else
      return BORDERTOP;
  }

  if (!Layout::landscape) {
    if (i < 4)
      return BORDERBOTTOM | BORDERRIGHT;
    else
      return BORDERTOP | BORDERRIGHT;
  }

  return BORDERRIGHT | BORDERBOTTOM;
}

void
InfoBoxManager::Create(RECT rc)
{
  ResetInfoBoxes();

  info_box_look.value.fg_color
    = info_box_look.title.fg_color
    = info_box_look.comment.fg_color
    = Appearance.InverseInfoBox ? Color::WHITE : Color::BLACK;
  info_box_look.value.bg_color
    = info_box_look.title.bg_color
    = info_box_look.comment.bg_color
    = Appearance.InverseInfoBox ? Color::BLACK : Color::WHITE;

  Color border_color = Color(80, 80, 80);
  info_box_look.border_pen.set(InfoBoxWindow::BORDER_WIDTH, border_color);
  info_box_look.selector_pen.set(IBLSCALE(1) + 2,
                                 info_box_look.value.fg_color);

  info_box_look.value.font = &Fonts::InfoBox;
  info_box_look.title.font = &Fonts::Title;
  info_box_look.comment.font = &Fonts::Title;
  info_box_look.small_font = &Fonts::InfoBoxSmall;

  info_box_look.colors[0] = border_color;
  info_box_look.colors[1] = Appearance.InverseInfoBox
    ? MapGfx.inv_redColor : Color::RED;
  info_box_look.colors[2] = Appearance.InverseInfoBox
    ? MapGfx.inv_blueColor : Color::BLUE;
  info_box_look.colors[3] = Appearance.InverseInfoBox
    ? MapGfx.inv_greenColor : Color::GREEN;
  info_box_look.colors[4] = Appearance.InverseInfoBox
    ? MapGfx.inv_yellowColor : Color::YELLOW;
  info_box_look.colors[5] = Appearance.InverseInfoBox
    ? MapGfx.inv_magentaColor : Color::MAGENTA;

  WindowStyle style;
  style.hide();
  full_window.set(main_window, rc.left, rc.right,
                  rc.right - rc.left, rc.bottom - rc.top);

  // create infobox windows
  for (unsigned i = 0; i < InfoBoxLayout::numInfoWindows; i++) {
    int xoff, yoff, sizex, sizey;
    InfoBoxLayout::GetInfoBoxPosition(i, rc, &xoff, &yoff, &sizex, &sizey);
    int Border = GetInfoBoxBorder(i);

    InfoBoxes[i] = new InfoBoxWindow(main_window, xoff, yoff, sizex, sizey,
                               Border, info_box_look);
  }
}

void
InfoBoxManager::Destroy()
{
  for (unsigned i = 0; i < InfoBoxLayout::numInfoWindows; i++)
    delete (InfoBoxes[i]);

  full_window.reset();
}
