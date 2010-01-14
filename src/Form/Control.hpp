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

class ContainerControl;

typedef enum {
  bkNone,
  bkTop,
  bkRight,
  bkBottom,
  bkLeft
} BorderKind_t;

/**
 * The WindowControl class is the base class for every other control
 * including the forms/windows itself, using the ContainerControl.
 */
class WindowControl : public ContainerWindow {
public:
  /**
   * Constructor of the WindowControl class
   * @param Owner
   * @param Parent
   * @param Name Name of the Control
   * @param X x-Coordinate of the Control
   * @param Y y-Coordinate of the Control
   * @param Width Width of the Control
   * @param Height Height of the Control
   * @param Visible Visibility of the Control
   */
  WindowControl(ContainerControl *Owner, ContainerWindow *Parent,
      const TCHAR *Name, int X, int Y, int Width, int Height, bool Visible = true);
  /** Destructor */
  virtual ~WindowControl(void);

  virtual bool on_setfocus();
  virtual bool on_killfocus();

  virtual bool on_unhandled_key(unsigned key_code);

  /**
   * The on_key_down event is called when a key is pressed while the
   * button is focused
   */
  virtual bool on_key_down(unsigned key_code);

  /**
   * The on_key_up event is called when a key is released while the
   * button is focused
   */
  virtual bool on_key_up(unsigned key_code);

  /**
   * The on_paint event is called when the WindowControl needs to be drawn
   * (derived from PaintWindow)
   */
  virtual void on_paint(Canvas &canvas);

  virtual int OnHelp();

  typedef void (*OnHelpCallback_t)(WindowControl *Sender);

  void SetOnHelpCallback(void(*Function)(WindowControl *Sender)) {
    mOnHelpCallback = Function;
  }

  /**
   * Tells the Control that it is focused now and returns the old value
   * @param Value Whether the Control is focused at the moment
   * @return The old value
   */
  bool SetFocused(bool Value);
  /**
   * Checks whether the Control is focused at the moment
   * @return True if the Control is focused at the moment, False otherwise
   */
  bool GetFocused(void) { return mHasFocus; }
  /**
   * Checks whether the Control can be focused and if so returns itself
   * @return NULL if the Control can not be focused, itself otherwise
   */
  virtual Window *GetCanFocus(bool forward);
  /**
   * If the given Value is true, the Control will be able to get focused
   * @param Value Whether the Control can be focused or not
   * @return The old value
   */
  bool SetCanFocus(bool Value);

  /**
   * Returns whether this is a read-only Control
   * @return True if this is a read-only Control, False otherwise
   */
  bool GetReadOnly(void) { return mReadOnly; }
  /**
   * Sets whether this is a read-only Control
   * @param Value Whether this is a read-only Control
   * @return The old value
   */
  bool SetReadOnly(bool Value);

  /**
   * Returns the border kind
   * @return The border kind
   */
  int GetBorderKind(void) { return mBorderKind; }
  /**
   * Sets the border kind
   * @param Value The new border kind
   * @return The old border kind
   */
  int SetBorderKind(int Value);

  /**
   * Returns the Control's font
   * @return The Control's font
   */
  const Font *GetFont(void) { return mhFont; }
  /**
   * Sets the Control's font
   * @param font The new font
   * @return The old font
   */
  virtual const Font *SetFont(const Font &font);
  /**
   * Sets the Control's font
   * @param font The new font
   * @return The old font
   */
  const Font *SetFont(const Font *font) { return SetFont(*font); }

  /**
   * Sets the Control's foreground color
   * @param Value The new foreground color
   * @return The old foreground color
   */
  virtual Color SetForeColor(Color Value);
  /**
   * Return the Control's foreground color
   * @return The Control's foreground color
   */
  Color GetForeColor(void) { return mColorFore; }

  /**
   * Sets the Control's background color
   * @param Value The new background color
   * @return The old background color
   */
  virtual Color SetBackColor(Color Value);
  /**
   * Return the Control's background color
   * @return The Control's background color
   */
  Color GetBackColor(void) { return mColorBack; }

  /**
   * Returns the brush for painting the background
   * @return The brush for painting the background
   */
  Brush &GetBackBrush(void) {
    return mhBrushBk.defined() ? mhBrushBk : hBrushDefaultBk;
  }

  /**
   * Returns the pen for drawing borders
   * @return The pen for drawing borders
   */
  Pen &GetBorderPen(void) {
    return mhPenBorder.defined() ? mhPenBorder : hPenDefaultBorder;
  }

  /**
   * Returns the pen for drawing the "Selector"
   * @return The pen for drawing the "Selector"
   */
  Pen &GetSelectorPen(void) {
    return mhPenSelector.defined() ? mhPenSelector : hPenDefaultSelector;
  }

  /**
   * Returns the Caption/Text of the Control
   * @return The Caption/Text of the Control
   */
  TCHAR *GetCaption(void) { return mCaption; }
  /**
   * Sets the Caption/Text of the Control
   * @param Value The new Caption/Text of the Control
   */
  virtual void SetCaption(const TCHAR *Value);

  /**
   * Sets the Helptext of the Control
   * @param Value The new Helptext of the Control
   */
  void SetHelpText(const TCHAR *Value);

  virtual ContainerWindow &GetClientAreaWindow(void) { return *this; }

  /**
   * Returns the parent ContainerControl
   * @return The parent ContainerControl
   */
  ContainerControl *GetOwner(void) { return mOwner; }

  /**
   * Return the tag of the Control
   * @return The tag of the Control
   */
  int GetTag(void) { return mTag; }
  /**
   * Sets the tag of the Control
   * @param Value The new tag of the Control
   * @return If successful the new tag of the Control
   */
  int SetTag(int Value) {
    mTag = Value;
    return mTag;
  }

  /**
   * Sets whether to draw the "Selector" or not
   * @param Value If false, the "Selector" will be drawn
   */
  void SetPaintSelector(bool Value) {
    mPaintSelector = Value;
  }

  /**
   * Checks whether the given Name is equal to the Control's Name
   * @param Name Name of the Control that is searched
   * @return The Control if equal, otherwise NULL
   */
  virtual WindowControl *FindByName(const TCHAR *Name);

  /**
   * Checks whether the given Name is equal to the Control's Name
   * @param Name Name of the Control that is searched
   * @return The Control if equal, otherwise NULL
   */
  const WindowControl *FindByName(const TCHAR *Name) const {
    return const_cast<WindowControl *>(this)->FindByName(Name);
  }

  /**
   * Shows/Hides the WindowControl depending on the given value of advanced and
   * whether the caption includes an asterisk.
   * @param advanced True if advanced mode activated
   */
  virtual void FilterAdvanced(bool advanced);

protected:
  /** If true, the Control can get focused */
  bool mCanFocus;
  /** Caption/Text of the Control */
  TCHAR mCaption[254];
  /** If true, the "Selector" is not painted */
  bool mPaintSelector;

  /** Paints the "Selector" */
  void PaintSelector(Canvas &canvas, const RECT rc);
  /** Paints the "Selector", but checks first if it should be painted */
  void PaintSelector(Canvas &canvas);

private:
  /** Parent ContainerControl */
  ContainerControl *mOwner;
  int mBorderKind;
  /** Background color */
  Color mColorBack;
  /** Foreground color */
  Color mColorFore;
  /** Brush for painting the background */
  Brush mhBrushBk;
  /** Pen for drawing borders */
  Pen mhPenBorder;
  /** Pen for drawing the "Selector" */
  Pen mhPenSelector;
  /** Font of the Control */
  const Font *mhFont;
  /** Name of the Control */
  TCHAR mName[64];
  /** Helptext of the Control */
  TCHAR *mHelpText;

  OnHelpCallback_t mOnHelpCallback;

  /** Tag of the Control */
  int mTag;
  /** Whether the Control is read-only */
  bool mReadOnly;
  /** True if the Control has been focused right now */
  bool mHasFocus;

  int mBorderSize;

  /** True if the default brushes and pens are already initialized */
  static bool initialized;
  /** The default Brush for painting the background */
  static Brush hBrushDefaultBk;
  /** The default Pen for drawing borders */
  static Pen hPenDefaultBorder;
  /** The default Pen for drawing the "Selector" */
  static Pen hPenDefaultSelector;
};

#endif
