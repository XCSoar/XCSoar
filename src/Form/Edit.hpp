// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/Control.hpp"
#include "ui/dim/Rect.hpp"

#include <string>

struct DialogLook;
class DataField;
class ContainerWindow;

/**
 * The WndProperty class implements a WindowControl with a caption label and
 * an editable field (the Editor).
 */
class WndProperty : public WindowControl {
public:
  // Alignment of the text: left, right or auto = left align with autoscroll
  enum class Alignment {
    LEFT,
    RIGHT,
    AUTO
  };

  typedef bool (*EditCallback)(const char *caption, DataField &df,
                               const char *help_text);

  const DialogLook &look;

  /** Position of the Editor Control */
  PixelRect edit_rc;

  /** Width reserved for the caption of the Control */
  int caption_width;

  std::string value;

  DataField *data_field = nullptr;

  EditCallback edit_callback;

  bool read_only = false;
  Alignment alignment = Alignment::LEFT;

  bool dragging = false, pressed = false;

public:
  /**
   * Constructor of the WndProperty
   * @param Parent The parent ContainerControl
   * @param Caption Caption of the Control
   * @param CaptionWidth Width of the Caption of the Control
   */
  WndProperty(ContainerWindow &parent, const DialogLook &look,
              const char *Caption,
              const PixelRect &rc, int CaptionWidth,
              const WindowStyle style) noexcept;

  WndProperty(const DialogLook &_look) noexcept;

  /** Destructor */
  ~WndProperty() noexcept;

  void Create(ContainerWindow &parent, const PixelRect &rc,
              const char *_caption,
              unsigned _caption_width,
              const WindowStyle style) noexcept;

public:
  /**
   * Returns the recommended caption width, measured by the dialog
   * font.
   */
  [[gnu::pure]]
  unsigned GetRecommendedCaptionWidth() const noexcept;

  void SetCaptionWidth(int caption_width) noexcept;

  void RefreshDisplay() noexcept;

  void SetReadOnly(bool _read_only=true) noexcept {
    read_only = _read_only;
  }

  [[gnu::pure]]
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

private:
  /**
   * Show full content in a dialog (for readonly fields with truncated content).
   */
  void ShowFullContent() noexcept;

protected:
  void OnResize(PixelSize new_size) noexcept override;
  void OnSetFocus() noexcept override;
  void OnKillFocus() noexcept override;

  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;

  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;

  void OnCancelMode() noexcept override;

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

  void SetAlignment(Alignment a) noexcept { alignment = a; }
  Alignment GetAlignment() const noexcept { return alignment; }

  void SetEditCallback(EditCallback _ec) noexcept {
    edit_callback = _ec;
  }

  const char *GetText() const noexcept {
    return value.c_str();
  }

  /**
   * Sets the Editors text to the given Value
   * @param Value The new text of the Editor Control
   */
  void SetText(const char *_value) noexcept;

private:
  /**
   * The OnPaint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  void OnPaint(Canvas &canvas) noexcept override;

  /** Increases the Editor value */
  int IncValue() noexcept;
  /** Decreases the Editor value */
  int DecValue() noexcept;

  void UpdateLayout() noexcept;
};
