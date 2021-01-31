/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "ui/dim/Rect.hpp"
#include "util/tstring.hpp"

struct DialogLook;
class DataField;
class ContainerWindow;

/**
 * The WndProperty class implements a WindowControl with a caption label and
 * an editable field (the Editor).
 */
class WndProperty : public WindowControl {
  typedef bool (*EditCallback)(const TCHAR *caption, DataField &df,
                               const TCHAR *help_text);

  const DialogLook &look;

  /** Position of the Editor Control */
  PixelRect edit_rc;

  /** Width reserved for the caption of the Control */
  int caption_width;

  tstring value;

  DataField *data_field = nullptr;

  EditCallback edit_callback;

  bool read_only = false;

  bool dragging = false, pressed = false;

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
              const WindowStyle style) noexcept;

  WndProperty(const DialogLook &_look) noexcept;

  /** Destructor */
  ~WndProperty() noexcept;

  void Create(ContainerWindow &parent, const PixelRect &rc,
              const TCHAR *_caption,
              unsigned _caption_width,
              const WindowStyle style) noexcept;

public:
  /**
   * Returns the recommended caption width, measured by the dialog
   * font.
   */
  gcc_pure
  unsigned GetRecommendedCaptionWidth() const noexcept;

  void SetCaptionWidth(int caption_width) noexcept;

  void RefreshDisplay() noexcept;

  void SetReadOnly(bool _read_only=true) noexcept {
    read_only = _read_only;
  }

  gcc_pure
  bool IsReadOnly() const noexcept {
    return read_only;
  }

  /**
   * Starts  interactively  editing  the  value.   If  a  ComboBox  is
   * available, then the ComboPicker  will be launched, otherwise, the
   * focus and cursor is set to the control.
   *
   * @return true if the value has been modified
   */
  bool BeginEditing() noexcept;

protected:
  void OnResize(PixelSize new_size) override;
  void OnSetFocus() override;
  void OnKillFocus() override;

  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseUp(PixelPoint p) override;
  bool OnMouseMove(PixelPoint p, unsigned keys) override;

  bool OnKeyCheck(unsigned key_code) const override;
  bool OnKeyDown(unsigned key_code) override;

  void OnCancelMode() override;

public:
  /**
   * Returns the Control's DataField
   * @return The Control's DataField
   */
  DataField *GetDataField() noexcept {
    return data_field;
  }

  /**
   * Returns the Control's DataField
   * @return The Control's DataField
   */
  const DataField *GetDataField() const noexcept {
    return data_field;
  }

  void SetDataField(DataField *Value) noexcept;

  void SetEditCallback(EditCallback _ec) noexcept {
    edit_callback = _ec;
  }

  /**
   * Sets the Editors text to the given Value
   * @param Value The new text of the Editor Control
   */
  void SetText(const TCHAR *_value) noexcept;

private:
  /**
   * The OnPaint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  void OnPaint(Canvas &canvas) override;

  /** Increases the Editor value */
  int IncValue() noexcept;
  /** Decreases the Editor value */
  int DecValue() noexcept;

  void UpdateLayout() noexcept;
};

#endif
