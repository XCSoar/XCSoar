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

#ifndef XCSOAR_FORM_EDIT_HPP
#define XCSOAR_FORM_EDIT_HPP

#include "Form/Control.hpp"
#include "Screen/EditWindow.hpp"

class DataField;

class WndProperty : public WindowControl {
  class Editor : public EditWindow {
  private:
    WndProperty *parent;

  public:
    Editor(WndProperty *_parent):parent(_parent) {}

    virtual bool on_mouse_down(int x, int y);
    virtual bool on_key_down(unsigned key_code);
    virtual bool on_key_up(unsigned key_code);
    virtual bool on_setfocus();
    virtual bool on_killfocus();
  };

private:

  static Bitmap hBmpLeft32;
  static Bitmap hBmpRight32;
  static int InstCount;

  Editor edit;
  POINT mEditSize;
  POINT mEditPos;
  const Font *mhValueFont;
  int  mBitmapSize;
  int  mCaptionWidth;
  RECT mHitRectUp;
  RECT mHitRectDown;
  bool mDownDown;
  bool mUpDown;

  /** from class PaintWindow */
  virtual void on_paint(Canvas &canvas);

  void (*mOnClickUpNotify)(WindowControl *Sender);
  void (*mOnClickDownNotify)(WindowControl *Sender);

  int (*mOnDataChangeNotify)(WindowControl *Sender, int Mode, int Value);

  int IncValue(void);
  int DecValue(void);

  DataField *mDataField;

  void UpdateButtonData(int Value);

public:

  int CallSpecial(void);
  bool mDialogStyle;

  typedef int (*DataChangeCallback_t)(WindowControl *Sender, int Mode, int Value);

  WndProperty(WindowControl *Parent, TCHAR *Name, TCHAR *Caption,
              int X, int Y, int Width, int Height, int CaptionWidth,
              int (*DataChangeNotify)(WindowControl *Sender,
                                      int Mode, int Value),
              int MultiLine=false);
  ~WndProperty(void);

  virtual Window *GetCanFocus();

  void on_editor_setfocus();
  void on_editor_killfocus();

  bool SetReadOnly(bool Value);

  void RefreshDisplay(void);

  const Font *SetFont(const Font &font);

  virtual bool on_unhandled_key(unsigned key_code);

  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);
  virtual bool on_mouse_double(int x, int y);

  DataField *GetDataField(void) {
    return mDataField;
  }

  const DataField *GetDataField(void) const {
    return mDataField;
  }

  DataField *SetDataField(DataField *Value);
  void SetText(const TCHAR *Value);
  int SetButtonSize(int Value);
};

#endif
