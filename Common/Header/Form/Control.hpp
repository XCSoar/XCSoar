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

#ifndef XCSOAR_FORM_CONTROL_HPP
#define XCSOAR_FORM_CONTROL_HPP

#include "Screen/Color.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Pen.hpp"
#include "Screen/ContainerWindow.hpp"

#include <tchar.h>

#define BORDERTOP    (1<<bkTop)
#define BORDERRIGHT  (1<<bkRight)
#define BORDERBOTTOM (1<<bkBottom)
#define BORDERLEFT   (1<<bkLeft)

typedef enum{
  bkNone,
  bkTop,
  bkRight,
  bkBottom,
  bkLeft
} BorderKind_t;

class WindowControl : public ContainerWindow {
public:
  typedef void (*OnHelpCallback_t)(WindowControl *Sender);

private:
  WindowControl *mOwner;
  int  mBorderKind;
  Color mColorBack;
  Color mColorFore;
  Brush mhBrushBk;
  Pen mhPenBorder;
  Pen mhPenSelector;
  const Font *mhFont;
  TCHAR mName[64];
  TCHAR *mHelpText;

  OnHelpCallback_t mOnHelpCallback;

  int mTag;
  bool mReadOnly;
  bool mHasFocus;

  int  mBorderSize;

  WindowControl *mActiveClient;

  static int InstCount;
  static Brush hBrushDefaultBk;
  static Pen hPenDefaultBorder;
  static Pen hPenDefaultSelector;

protected:

  bool mCanFocus;
  TCHAR mCaption[254];
  bool mDontPaintSelector;

  WindowControl *mClients[50];
  int mClientCount;

  void PaintSelector(Canvas &canvas);
  WindowControl *SetOwner(WindowControl *Value);
  bool HasFocus(void) { return mHasFocus; }

public:
  TCHAR *GetCaption(void) { return mCaption; }

  virtual bool on_setfocus();
  virtual bool on_killfocus();

  virtual bool on_unhandled_key(unsigned key_code);

  virtual void AddClient(WindowControl *Client);

  virtual bool on_key_down(unsigned key_code);
  virtual bool on_key_up(unsigned key_code);

  /** from class PaintWindow */
  virtual void on_paint(Canvas &canvas);

  virtual int OnHelp();

  void SetOnHelpCallback(void(*Function)(WindowControl *Sender)) {
    mOnHelpCallback = Function;
  }

  bool SetFocused(bool Value);
  bool GetFocused(void);
  virtual Window *GetCanFocus();
  bool SetCanFocus(bool Value);

  bool GetReadOnly(void) { return mReadOnly; }
  bool SetReadOnly(bool Value);

  int  GetBorderKind(void);
  int  SetBorderKind(int Value);

  const Font *GetFont(void) { return mhFont; }
  virtual const Font *SetFont(const Font &font);

  const Font *SetFont(const Font *font) {
    return SetFont(*font);
  }

  virtual Color SetForeColor(Color Value);
  Color GetForeColor(void) { return mColorFore; }

  virtual Color SetBackColor(Color Value);
  Color GetBackColor(void) { return mColorBack; }

  Brush &GetBackBrush(void) {
    return mhBrushBk.defined()
      ? mhBrushBk
      : hBrushDefaultBk;
  }
  Pen &GetBorderPen(void) {
    return mhPenBorder.defined()
      ? mhPenBorder
      : hPenDefaultBorder;
  }
  Pen &GetSelectorPen(void) {
    return mhPenSelector.defined()
      ? mhPenSelector
      : hPenDefaultSelector;
  }

  virtual void SetCaption(const TCHAR *Value);
  void SetHelpText(const TCHAR *Value);

  virtual ContainerWindow &GetClientAreaWindow(void) { return *this; }
  WindowControl *GetOwner(void) { return mOwner; }

  int GetTag(void) { return mTag; }
  int SetTag(int Value) {
    mTag = Value; return mTag;
  }

  Window *FocusNext(WindowControl *Sender);
  Window *FocusPrev(WindowControl *Sender);

  WindowControl(WindowControl *Owner, ContainerWindow *Parent,
                const TCHAR *Name, int X, int Y, int Width, int Height,
                bool Visible=true);
  virtual ~WindowControl(void);

  void PaintSelector(bool Value) {mDontPaintSelector = Value;}

  WindowControl *FindByName(const TCHAR *Name);

  const WindowControl *FindByName(const TCHAR *Name) const {
    return const_cast<WindowControl *>(this)->FindByName(Name);
  }

  void FilterAdvanced(bool advanced);

};

#endif
