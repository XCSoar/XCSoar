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
#include "Language.hpp"
#include "DataField/Enum.hpp"
#include "Form/Edit.hpp"
#include "Form/Form.hpp"
#include "Profile/Profile.hpp"

#include <assert.h>

#include <algorithm>

using std::min;

InfoBoxFullWindow InfoBoxManager::full_window;

static bool InfoBoxesDirty = false;
static bool InfoBoxesHidden = false;

InfoBoxWindow *InfoBoxes[MAXINFOWINDOWS];

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

void
InfoBoxFullWindow::on_paint(Canvas &canvas)
{
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
}

// TODO locking
void
InfoBoxManager::Hide()
{
  InfoBoxesHidden = true;

  for (unsigned i = 0; i < InfoBoxLayout::numInfoWindows; i++)
    InfoBoxes[i]->fast_hide();

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

enum InfoBoxManager::mode
InfoBoxManager::GetCurrentMode()
{
  if (SettingsMap().EnableAuxiliaryInfo)
    return MODE_AUXILIARY;
  else if (MapProjection().GetDisplayMode() == dmCircling)
    return MODE_CIRCLING;
  else if (MapProjection().GetDisplayMode() == dmFinalGlide)
    return MODE_FINAL_GLIDE;
  else
    return MODE_CRUISE;
}

int
InfoBoxManager::getType(unsigned i, enum mode mode)
{
  assert(i < MAXINFOWINDOWS);

  switch (mode) {
  case MODE_CIRCLING:
    return InfoType[i] & 0xff;
  case MODE_CRUISE:
    return (InfoType[i] >> 8) & 0xff;
  case MODE_FINAL_GLIDE:
    return (InfoType[i] >> 16) & 0xff;
  case MODE_AUXILIARY:
    return (InfoType[i] >> 24) & 0xff;
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
  unsigned retval = getType(i, GetCurrentMode());
  return min(InfoBoxFactory::NUM_TYPES - 1, retval);
}

bool
InfoBoxManager::IsEmpty(enum mode mode)
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
InfoBoxManager::setType(unsigned i, char j, enum mode mode)
{
  assert(i < MAXINFOWINDOWS);

  switch (mode) {
  case MODE_CIRCLING:
    InfoType[i] &= 0xffffff00;
    InfoType[i] += j;
    break;
  case MODE_CRUISE:
    InfoType[i] &= 0xffff00ff;
    InfoType[i] += (j << 8);
    break;
  case MODE_FINAL_GLIDE:
    InfoType[i] &= 0xff00ffff;
    InfoType[i] += (j << 16);
    break;
  case MODE_AUXILIARY:
    InfoType[i] &= 0x00ffffff;
    InfoType[i] += (j << 24);
    break;
  }
}

void
InfoBoxManager::setType(unsigned i, char j)
{
  setType(i, j, GetCurrentMode());
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
    j = InfoBoxFactory::GetNext(k);
  else if (i < 0)
    j = InfoBoxFactory::GetPrevious(k);

  // TODO code: if i==0, go to default or reset

  setType(InfoFocus, j);

  InfoBoxes[InfoFocus]->UpdateContent();
  Paint();
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

    if (needupdate) {
      InfoBoxes[i]->SetTitle(gettext(InfoBoxFactory::GetCaption(DisplayType[i])));
      InfoBoxes[i]->SetContentProvider(InfoBoxFactory::Create(DisplayType[i]));
    }

    InfoBoxes[i]->UpdateContent();

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
  return InfoBoxFactory::GetName(i);
}

void
InfoBoxManager::Paint()
{
  if (!InfoBoxLayout::fullscreen) {
    full_window.hide();
  } else {
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
  info_box_look.background_brush.set(Appearance.InverseInfoBox
                                     ? Color::BLACK : Color::WHITE);

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
    ? Graphics::inv_redColor : Color::RED;
  info_box_look.colors[2] = Appearance.InverseInfoBox
    ? Graphics::inv_blueColor : Color::BLUE;
  info_box_look.colors[3] = Appearance.InverseInfoBox
    ? Graphics::inv_greenColor : Color::GREEN;
  info_box_look.colors[4] = Appearance.InverseInfoBox
    ? Graphics::inv_yellowColor : Color::YELLOW;
  info_box_look.colors[5] = Appearance.InverseInfoBox
    ? Graphics::inv_magentaColor : Color::MAGENTA;

  WindowStyle style;
  style.hide();
  full_window.set(main_window, rc.left, rc.top,
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

static void
OnInfoBoxHelp(WindowControl *Sender)
{
  WndProperty *wp = (WndProperty*)Sender;
  int type = wp->GetDataField()->GetAsInteger();

  TCHAR caption[100];
  _stprintf(caption, _T("%s: %s"), _("InfoBox"), InfoBoxFactory::GetName(type));

  const TCHAR* text = InfoBoxFactory::GetDescription(type);
  if (text)
    dlgHelpShowModal(XCSoarInterface::main_window, caption, gettext(text));
  else
    dlgHelpShowModal(XCSoarInterface::main_window, caption,
                     _("No help available on this item"));
}

void
InfoBoxManager::SetupFocused()
{
  int i = GetFocused();
  if (i < 0)
    return;

  const enum mode mode = GetCurrentMode();
  int old_type = getType(i, mode);

  /* create a fake WndProperty for dlgComboPicker() */
  /* XXX reimplement properly */

  WindowStyle style;
  style.hide();

  WndForm form(main_window, _T(""), 0, 0, 256, 128, style);
  WndProperty control(form, _("InfoBox"), 0, 0, 256, 128, 128,
                      style, EditWindowStyle(), NULL);
  control.SetOnHelpCallback(OnInfoBoxHelp);

  DataFieldEnum *dfe = new DataFieldEnum(_T(""), _T(""), old_type, NULL);
  for (unsigned i = 0; i < InfoBoxFactory::NUM_TYPES; i++)
    dfe->addEnumText(gettext(GetTypeDescription(i)));
  dfe->Sort(0);
  dfe->Set(old_type);
  control.RefreshDisplay();

  control.SetDataField(dfe);

  /* let the user select */

  dlgComboPicker(main_window, &control);

  /* was there a modification? */

  int new_type = dfe->GetAsInteger();
  if (new_type == old_type)
    return;

  /* yes: apply and save it */

  setType(i, new_type, mode);
  DisplayInfoBox();
  Profile::SetInfoBoxes(i, getTypeAll(i));
}
