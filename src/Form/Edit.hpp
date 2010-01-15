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

/**
 * The WndProperty class implements a WindowControl with a caption label and
 * an editable field (the Editor).
 */
class WndProperty : public WindowControl {
  class Editor : public EditWindow {
  private:
    /** The parent Control */
    WndProperty *parent;

  public:
    /**
     * Constructor of the Editor class
     * @param _parent The parent Control the Editor belongs to
     */
    Editor(WndProperty *_parent):parent(_parent) {}

    /**
     * The on_mouse_down event is called when the mouse is pressed over the button
     * (derived from Window)
     */
    virtual bool on_mouse_down(int x, int y);
    /**
     * The on_key_down event is called when a key is pressed while the
     * button is focused
     * (derived from Window)
     */
    virtual bool on_key_down(unsigned key_code);
    /**
     * The on_key_down event is called when a key is released while the
     * button is focused
     * (derived from Window)
     */
    virtual bool on_key_up(unsigned key_code);
    /**
     * The on_setfocus event is called when the Control gets focused
     * button is focused
     * (derived from Window)
     */
    virtual bool on_setfocus();
    /**
     * The on_killfocus event is called when the Control loses focus
     * button is focused
     * (derived from Window)
     */
    virtual bool on_killfocus();
  };

public:
  typedef int (*DataChangeCallback_t)(WindowControl *Sender, int Mode, int Value);
  typedef void (*ClickUpCallback_t)(WindowControl *Sender);
  typedef void (*ClickDownCallback_t)(WindowControl *Sender);

private:
  /** Arrow left bitmap */
  static Bitmap hBmpLeft32;
  /** Arrow right bitmap */
  static Bitmap hBmpRight32;
  static int InstCount;

  /** Editor Control */
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

  DataChangeCallback_t mOnDataChangeNotify;
  ClickUpCallback_t mOnClickUpNotify;
  ClickDownCallback_t mOnClickDownNotify;

  int IncValue(void);
  int DecValue(void);

  DataField *mDataField;

  void UpdateButtonData(int Value);

public:
  int CallSpecial(void);
  bool mDialogStyle;

  /**
   * Constructor of the WndProperty
   * @param Parent The parent ContainerControl
   * @param Name Name of the Control
   * @param Caption Caption of the Control
   * @param X x-Coordinate of the Control
   * @param Y y-Coordinate of the Control
   * @param Width Width of the Control
   * @param Height Heigth of the Control
   * @param CaptionWidth Width of the Caption of the Control
   * @param DataChangeNotify Function to call when the data changed
   * @param MultiLine If true, the Control can handle mutliple lines
   */
  WndProperty(ContainerControl *Parent, TCHAR *Name, TCHAR *Caption,
              int X, int Y, int Width, int Height, int CaptionWidth,
              DataChangeCallback_t DataChangeNotify,
              int MultiLine=false);
  /** Destructor */
  ~WndProperty(void);

  virtual Window *GetCanFocus(bool forward);

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
