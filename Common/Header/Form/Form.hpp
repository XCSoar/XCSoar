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

#ifndef XCSOAR_FORM_FORM_HPP
#define XCSOAR_FORM_FORM_HPP

#include "Form/Control.hpp"
#include "Dialogs.h"

class PeriodClock;

class WndForm : public WindowControl {
protected:
  int mModalResult;
  Color mColorTitle;
  const Font *mhTitleFont;
  WindowControl *mClientWindow;
  RECT mClientRect;
  RECT mTitleRect;

  int (*mOnTimerNotify)(WindowControl *Sender);
  bool (*mOnKeyDownNotify)(WindowControl *Sender, unsigned key_code);
  bool (*mOnUserMsgNotify)(WindowControl *Sender, unsigned id);

  /** from class PaintWindow */
  virtual void on_paint(Canvas &canvas);

  timer_t cbTimerID;

public:

  WndForm(ContainerWindow *Parent,
          const TCHAR *Name, const TCHAR *Caption,
          int X, int Y, int Width, int Height);

  ContainerWindow &GetClientAreaWindow(void);
  void AddClient(WindowControl *Client);

  int OnLButtonUp(WPARAM wParam, LPARAM lParam) {
    (void)wParam; (void)lParam;
    return 0;
  }

  DWORD enterTime;

  int GetModalResult(void) { return mModalResult; }
  int SetModalResult(int Value) {
    mModalResult = Value;
    return Value;
  }

  const Font *SetTitleFont(const Font &font);

  int ShowModal(bool bEnableMap);
  int ShowModal(void);
  void Show(void);

  void SetCaption(const TCHAR *Value);

  virtual bool on_unhandled_key(unsigned key_code);

  /** from class Window */
  virtual bool on_timer(timer_t id);
  virtual bool on_user(unsigned id);

  Color SetForeColor(Color Value);
  Color SetBackColor(Color Value);
  const Font *SetFont(const Font &Value);

  void SetKeyDownNotify(bool (*KeyDownNotify)(WindowControl *Sender,
                                              unsigned key_code));
  void SetLButtonUpNotify(int (*LButtonUpNotify)(WindowControl *Sender,
                                                 WPARAM wParam, LPARAM lParam));

  void SetTimerNotify(int (*OnTimerNotify)(WindowControl *Sender));

  void SetUserMsgNotify(bool (*OnUserMsgNotify)(WindowControl *Sender,
                                                unsigned id));
private:
  static PeriodClock timeAnyOpenClose; // when any dlg opens or child closes
};

#endif
