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
#include "Screen/Point.hpp"
#include "Util/tstring.hpp"

struct DialogLook;
class DataField;
class ContainerWindow;

/**
 * The WndProperty class implements a WindowControl with a caption label and
 * an editable field (the Editor).
 */
class WndProperty : public WindowControl {
  const DialogLook &look;

  /** Position of the Editor Control */
  PixelRect edit_rc;

  /** Width reserved for the caption of the Control */
  PixelScalar caption_width;

  tstring value;

  DataField *mDataField;

  bool read_only;

  bool dragging, pressed;

public:
  /**
   * Constructor of the WndProperty
   * @param Parent The parent ContainerControl
   * @param Caption Caption of the Control
   * @param CaptionWidth Width of the Caption of the Control
   */
  WndProperty(ContainerWindow &parent, const DialogLook &look,
              const TCHAR *Caption,
              const PixelRect &rc, int CaptionWidth,
              const WindowStyle style);

  /** Destructor */
  ~WndProperty();

protected:
  int CallSpecial();

public:
  /**
   * Returns the recommended caption width, measured by the dialog
   * font.
   */
  gcc_pure
  UPixelScalar GetRecommendedCaptionWidth() const;

  void SetCaptionWidth(PixelScalar caption_width);

  void RefreshDisplay();

  void SetReadOnly(bool _read_only=true) {
    read_only = _read_only;
  }

  gcc_pure
  bool IsReadOnly() const {
    return read_only;
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
  virtual void OnResize(UPixelScalar width, UPixelScalar height) override;
  virtual void OnSetFocus() override;
  virtual void OnKillFocus() override;

  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y,
                           unsigned keys) override;

  virtual bool OnKeyCheck(unsigned key_code) const override;
  virtual bool OnKeyDown(unsigned key_code) override;
  virtual bool OnKeyUp(unsigned key_code) override;

  virtual void OnCancelMode() override;

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
  void SetText(const TCHAR *_value);

private:
  /**
   * The OnPaint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  virtual void OnPaint(Canvas &canvas) override;

  /** Increases the Editor value */
  int IncValue();
  /** Decreases the Editor value */
  int DecValue();

  void UpdateLayout();
};

#endif
