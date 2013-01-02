/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_FORM_EDIT_HPP
#define XCSOAR_FORM_EDIT_HPP

#include "Form/Control.hpp"
#include "Screen/EditWindow.hpp"
#include "Screen/Point.hpp"

struct DialogLook;
class DataField;
class ContainerWindow;

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
     * The OnMouseDown event is called when the mouse is pressed over the button
     * (derived from Window)
     */
    virtual bool OnMouseDown(PixelScalar x, PixelScalar y);

    virtual bool OnKeyCheck(unsigned key_code) const;

    /**
     * The OnKeyDown event is called when a key is pressed while the
     * button is focused
     * (derived from Window)
     */
    virtual bool OnKeyDown(unsigned key_code);
    /**
     * The OnKeyDown event is called when a key is released while the
     * button is focused
     * (derived from Window)
     */
    virtual bool OnKeyUp(unsigned key_code);

    virtual void OnSetFocus();
    virtual void OnKillFocus();
  };

  friend class Editor;

public:
  typedef int (*DataChangeCallback_t)(WindowControl *Sender, int Mode, int Value);
  typedef void (*ClickUpCallback_t)(WindowControl *Sender);
  typedef void (*ClickDownCallback_t)(WindowControl *Sender);

private:
  const DialogLook &look;

  /** Editor Control */
  Editor edit;

  /** Position of the Editor Control */
  PixelRect edit_rc;

  /** Width reserved for the caption of the Control */
  PixelScalar caption_width;

  /** Function to call when the Editor data has changed */
  DataChangeCallback_t mOnDataChangeNotify;

  DataField *mDataField;

public:
  /**
   * Constructor of the WndProperty
   * @param Parent The parent ContainerControl
   * @param Caption Caption of the Control
   * @param X x-Coordinate of the Control
   * @param Y y-Coordinate of the Control
   * @param Width Width of the Control
   * @param Height Heigth of the Control
   * @param CaptionWidth Width of the Caption of the Control
   * @param DataChangeNotify Function to call when the data changed
   * @param MultiLine If true, the Control can handle mutliple lines
   */
  WndProperty(ContainerWindow &parent, const DialogLook &look,
              const TCHAR *Caption,
              const PixelRect &rc, int CaptionWidth,
              const WindowStyle style,
              const EditWindowStyle edit_style,
              DataChangeCallback_t DataChangeNotify);

  /** Destructor */
  ~WndProperty();

protected:
  int CallSpecial();

  void on_editor_setfocus();
  void on_editor_killfocus();

public:
  void SetDataChangeCallback(DataChangeCallback_t data_change_callback) {
    mOnDataChangeNotify = data_change_callback;
  }

  /**
   * Returns the recommended caption width, measured by the dialog
   * font.
   */
  gcc_pure
  UPixelScalar GetRecommendedCaptionWidth() const;

  void SetCaptionWidth(PixelScalar caption_width);

  void RefreshDisplay();

  void SetEnabled(bool enabled) {
    WindowControl::SetEnabled(enabled);
    edit.SetEnabled(enabled);
  }

  void SetReadOnly(bool read_only=true) {
    edit.SetReadOnly(read_only);
  }

  gcc_pure
  bool IsReadOnly() const {
    return edit.IsReadOnly();
  }

  /**
   * Starts  interactively  editing  the  value.   If  a  ComboBox  is
   * available, then the ComboPicker  will be launched, otherwise, the
   * focus and cursor is set to the control.
   *
   * @return false on failure
   */
  bool BeginEditing();

protected:
  virtual void OnResize(UPixelScalar width, UPixelScalar height);

  /**
   * The OnMouseDown event is called when the mouse is pressed over the button
   * (derived from Window)
   */
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y);
  /**
   * The OnMouseUp event is called when the mouse is released over the button
   * (derived from Window)
   */
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y);

public:
  /**
   * Returns the Control's DataField
   * @return The Control's DataField
   */
  DataField *GetDataField() {
    return mDataField;
  }

  /**
   * Returns the Control's DataField
   * @return The Control's DataField
   */
  const DataField *GetDataField() const {
    return mDataField;
  }

  void SetDataField(DataField *Value);

  /**
   * Sets the Editors text to the given Value
   * @param Value The new text of the Editor Control
   */
  void SetText(const TCHAR *value) {
    edit.SetText(value);
  }

private:
  /**
   * The OnPaint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  virtual void OnPaint(Canvas &canvas);

  /** Increases the Editor value */
  int IncValue();
  /** Decreases the Editor value */
  int DecValue();

  void UpdateLayout();
};

#endif
