/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_ROW_FORM_WIDGET_HPP
#define XCSOAR_ROW_FORM_WIDGET_HPP

#include "WindowWidget.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Base.hpp"
#include "Repository/FileType.hpp"
#include "Util/StaticArray.hxx"
#include "Util/EnumCast.hpp"
#include "Units/Group.hpp"

#include <assert.h>
#include <stdint.h>

struct DialogLook;
struct StaticEnumChoice;
class ActionListener;
class Angle;
class RoughTime;
class RoughTimeDelta;
class Button;

/**
 * A #Widget that contains #WndProperty controls, one in a row.
 * Derive from this class, and construct the controls in Initialise()
 * or Prepare().  Don't forget to call the base class in these two
 * methods!
 */
class RowFormWidget : public WindowWidget {
  struct Row {
    enum class Type : uint8_t {
      /**
       * A dummy entry without a #Window.  Its height is zero.
       */
      DUMMY,

      /**
       * A #Widget.
       */
      WIDGET,

      /**
       * A generic #Window.
       */
      GENERIC,

      /**
       * A #WndProperty.
       */
      EDIT,

      /**
       * A multi-line #WndProperty.
       */
      MULTI_LINE,

      /**
       * A #Button.
       */
      BUTTON,

      /**
       * A generic #Window that fills the remaining vertical space at
       * the bottom.  It must be the last row, and there can only be
       * one.
       */
      REMAINING,
    };

    Type type;

    /**
     * Only used for #type==WIDGET.
     */
    bool initialised, prepared, shown;

    /**
     * Shall this row be available?  If not, it is hidden and no
     * screen space is reserved for it.
     */
    bool available;

    /**
     * Shall this row be visible?  The "expert" flag overrides it in
     * global "non-expert" mode.
     */
    bool visible;

    /**
     * If true, then the row is only visible in "expert" mode.
     */
    bool expert;

    Widget *widget;

    Window *window;

    /**
     * The position determined by RowFormWidget::UpdateLayout().  This
     * attribute is strictly only necessary for widgets, because these
     * expect a PixelRect parameter in their Show() method.
     */
    PixelRect position;

    Row() = default;

    Row(Type _type)
      :type(_type), available(true), visible(false), expert(false),
       widget(nullptr), window(nullptr) {
      assert(_type == Type::DUMMY);
    }

    Row(Type _type, Window *_window)
      :type(_type), available(true), visible(true), expert(false),
       widget(nullptr), window(_window) {
      assert(_type != Type::DUMMY);
      assert(_window != nullptr);
    }

    Row(Widget *_widget)
      :type(Type::WIDGET),
       initialised(false), prepared(false), shown(false),
       available(true), visible(true), expert(false),
       widget(_widget), window(nullptr) {
      assert(_widget != nullptr);
    }

    /**
     * Determines whether this row is available.  A row that is not
     * available is hidden and will not be considered in the layout.
     *
     * @param expert_mode true if the user has enabled "expert" mode
     */
    bool IsAvailable(bool expert_mode) const {
      return type != Row::Type::DUMMY && available &&
        (!expert || expert_mode);
    }

    void Unprepare() {
      if (type == Type::WIDGET) {
        assert(widget != nullptr);
        assert(window == nullptr);

        if (shown) {
          widget->Hide();
          shown = false;
        }

        if (prepared) {
          widget->Unprepare();
          prepared = false;
        }
      }
    }

    /**
     * Delete the #Widget or #Window object.
     */
    void Delete() {
      Unprepare();

      if (type == Type::WIDGET) {
        assert(widget != nullptr);
        assert(window == nullptr);
        assert(!shown);
        assert(!prepared);

        delete widget;
        return;
      }

      delete window;
    }

    gcc_pure
    Widget &GetWidget() {
      assert(widget != nullptr);
      assert(window == nullptr);

      return *widget;
    }

    gcc_pure
    const Widget &GetWidget() const {
      assert(widget != nullptr);
      assert(window == nullptr);

      return *widget;
    }

    gcc_pure
    Window &GetWindow() {
      assert(window != nullptr);

      return *window;
    }

    gcc_pure
    const Window &GetWindow() const {
      assert(window != nullptr);

      return *window;
    }

    gcc_pure
    WndProperty &GetControl() {
      assert(type == Type::EDIT);
      assert(window != nullptr);

      return *(WndProperty *)window;
    }

    gcc_pure
    const WndProperty &GetControl() const {
      assert(type == Type::EDIT);
      assert(window != nullptr);

      return *(WndProperty *)window;
    }

    /**
     * Will this row grow when there is excess screen space?
     */
    bool IsElastic(const DialogLook &look, bool vertical) const {
      return GetMaximumHeight(look, vertical)
        > GetMinimumHeight(look, vertical);
    }

    gcc_pure
    unsigned GetMinimumHeight(const DialogLook &look, bool vertical) const;

    gcc_pure
    unsigned GetMaximumHeight(const DialogLook &look, bool vertical) const;

    void UpdateLayout(ContainerWindow &parent, const PixelRect &_position,
                      int caption_width);

    void SetVisible(ContainerWindow &parent, bool _visible);

    /**
     * Show the Window/Widget, but do not update the #visible flag.
     */
    void Show(ContainerWindow &parent);

    /**
     * Hide the Window/Widget, but do not update the #visible flag.
     */
    void Hide();
  };

  const DialogLook &look;

  /**
   * True if "vertical" layout is enabled.  It means that edit control
   * captions are above the value instead of left of the value.
   */
  const bool vertical;

  StaticArray<Row, 32u> rows;

public:
  RowFormWidget(const DialogLook &look, bool vertical=false);
  virtual ~RowFormWidget();

protected:
  const DialogLook &GetLook() const {
    return look;
  }

  void Add(Row::Type type, Window *window);

  WndProperty *CreateEdit(const TCHAR *label, const TCHAR *help=nullptr,
                          bool read_only=false);

public:
  /**
   * Add a "dummy" row.  It does not have a #Window and its height is
   * zero.  This may be used to "reserve" a row index when a row is
   * not present in a certain instance of the form.
   */
  void AddDummy() {
    rows.push_back(Row::Type::DUMMY);
  }

  /**
   * Add a #Widget row.  The object will be deleted automatically.
   */
  void Add(Widget *widget) {
    rows.push_back(widget);
  }

  void Add(Window *window) {
    Add(Row::Type::GENERIC, window);
  }

  /**
   * Add a #Window that fills the remaining vertical space at the
   * bottom.  It must be the last row, and there can only be one.
   */
  void AddRemaining(Window *window) {
    Add(Row::Type::REMAINING, window);
  }

  WndProperty *Add(const TCHAR *label, const TCHAR *help=nullptr,
                   bool read_only=false);

  /**
   * Add a read-only control.  You can use SetText() to update its
   * text.
   */
  void AddReadOnly(const TCHAR *label, const TCHAR *help=nullptr,
                   const TCHAR *text=nullptr);

  /**
   * Add a read-only control displaying a floating-point value.  Use
   * LoadValue() to update the displayed value.
   */
  void AddReadOnly(const TCHAR *label, const TCHAR *help,
                   const TCHAR *display_format,
                   double value);

  /**
   * Add a read-only control displaying a floating-point value.  Use
   * LoadValue() to update the displayed value.
   */
  void AddReadOnly(const TCHAR *label, const TCHAR *help,
                   const TCHAR *display_format,
                   UnitGroup unit_group, double value);

  /**
   * Add a read-only control displaying a boolean value.  Use
   * LoadValue() to update the displayed value.
   */
  void AddReadOnly(const TCHAR *label, const TCHAR *help,
                   bool value);

  WndProperty *Add(const TCHAR *label, const TCHAR *help,
                   DataField *df);

  WndProperty *AddBoolean(const TCHAR *label, const TCHAR *help,
                          bool value=false,
                          DataFieldListener *listener=nullptr);

  WndProperty *AddInteger(const TCHAR *label, const TCHAR *help,
                          const TCHAR *display_format,
                          const TCHAR *edit_format,
                          int min_value, int max_value, int step, int value,
                          DataFieldListener *listener=nullptr);

  WndProperty *AddFloat(const TCHAR *label, const TCHAR *help,
                        const TCHAR *display_format,
                        const TCHAR *edit_format,
                        double min_value, double max_value,
                        double step, bool fine,
                        double value,
                        DataFieldListener *listener=nullptr);

  WndProperty *AddFloat(const TCHAR *label, const TCHAR *help,
                        const TCHAR *display_format,
                        const TCHAR *edit_format,
                        double min_value, double max_value,
                        double step, bool fine,
                        UnitGroup unit_group, double value,
                        DataFieldListener *listener=nullptr);

  WndProperty *AddAngle(const TCHAR *label, const TCHAR *help,
                        Angle value, unsigned step, bool fine,
                        DataFieldListener *listener=nullptr);

  WndProperty *AddEnum(const TCHAR *label, const TCHAR *help,
                       const StaticEnumChoice *list, unsigned value=0,
                       DataFieldListener *listener=nullptr);

  WndProperty *AddEnum(const TCHAR *label, const TCHAR *help,
                       DataFieldListener *listener=nullptr);

  WndProperty *AddText(const TCHAR *label, const TCHAR *help,
                       const TCHAR *content,
                       DataFieldListener *listener=nullptr);

  /**
   * Add a password edit control.  The password is obfuscated while
   * not editing.
   */
  WndProperty *AddPassword(const TCHAR *label, const TCHAR *help,
                           const TCHAR *content);

  WndProperty *AddTime(const TCHAR *label, const TCHAR *help,
                       int min_value, int max_value, unsigned step,
                       int value, unsigned max_tokens = 2,
                       DataFieldListener *listener=nullptr);

  WndProperty *AddRoughTime(const TCHAR *label, const TCHAR *help,
                            RoughTime value, RoughTimeDelta time_zone,
                            DataFieldListener *listener=nullptr);

  void AddSpacer();

  WndProperty *AddFile(const TCHAR *label, const TCHAR *help,
                       const char *profile_key, const TCHAR *filters,
                       FileType file_type,
                       bool nullable = true);

  WndProperty *AddFile(const TCHAR *label, const TCHAR *help,
                       const char *profile_key, const TCHAR *filters,
                       bool nullable = true) {
    return AddFile(label, help, profile_key, filters, FileType::UNKNOWN,
                   nullable);
  }

  /**
   * Add a read-only multi-line control.  You can use
   * SetMultiLineText() to update its text.
   */
  void AddMultiLine(const TCHAR *text=nullptr);

  Button *AddButton(const TCHAR *label, ActionListener &listener, int id);

  gcc_pure
  Widget &GetRowWidget(unsigned i) {
    return rows[i].GetWidget();
  }

  gcc_pure
  Window &GetRow(unsigned i) {
    return rows[i].GetWindow();
  }

  gcc_pure
  const Window &GetRow(unsigned i) const {
    return rows[i].GetWindow();
  }

  void SetReadOnly(unsigned i, bool read_only=true) {
    GetControl(i).SetReadOnly(read_only);
  }

  void SetRowEnabled(unsigned i, bool enabled) {
    GetRow(i).SetEnabled(enabled);
  }

  /**
   * Modify the "available" flag on this row.
   */
  void SetRowAvailable(unsigned i, bool available);

  void SetRowVisible(unsigned i, bool visible);

  void ShowRow(unsigned i) {
    SetRowVisible(i, true);
  }

  void HideRow(unsigned i) {
    SetRowVisible(i, false);
  }

  /**
   * Enable the "expert" flag on this row: hide unless the form is in
   * "expert" mode.  This must be called before the first Show()
   * invocation.
   */
  void SetExpertRow(unsigned i);

  gcc_pure
  Window &GetGeneric(unsigned i) {
    return rows[i].GetWindow();
  }

  gcc_pure
  WndProperty &GetControl(unsigned i) {
    return rows[i].GetControl();
  }

  gcc_pure
  const WndProperty &GetControl(unsigned i) const {
    return rows[i].GetControl();
  }

  /**
   * Update the text of a multi line control.
   */
  void SetText(unsigned i, const TCHAR *text) {
    assert(text != nullptr);

    WndProperty &control = GetControl(i);
    assert(control.GetDataField() == nullptr);
    control.SetText(text);
  }

  void ClearText(unsigned i) {
    SetText(i, _T(""));
  }

  /**
   * Update the text of a multi line control.
   */
  void SetMultiLineText(unsigned i, const TCHAR *text);

  gcc_pure
  DataField &GetDataField(unsigned i) {
    DataField *df = GetControl(i).GetDataField();
    assert(df != nullptr);
    return *df;
  }

  gcc_pure
  const DataField &GetDataField(unsigned i) const {
    const DataField *df = GetControl(i).GetDataField();
    assert(df != nullptr);
    return *df;
  }

  /**
   * Compare a row's data field with the given reference (by their
   * addresses).
   */
  gcc_pure
  bool IsDataField(unsigned i, const DataField &df) const {
    return &df == &GetDataField(i);
  }

  void LoadValue(unsigned i, int value);
  void LoadValue(unsigned i, bool value);
  void LoadValueEnum(unsigned i, unsigned value);

  template<typename T>
  void LoadValueEnum(unsigned i, T value) {
    LoadValueEnum(i, unsigned(value));
  }

  void LoadValue(unsigned i, const TCHAR *value);

  void LoadValue(unsigned i, double value);
  void LoadValue(unsigned i, Angle value);
  void LoadValue(unsigned i, double value, UnitGroup unit_group);

  void LoadValue(unsigned i, RoughTime value);

  /**
   * Load a value into a control created by AddTime().
   */
  void LoadValueTime(unsigned i, int value);

  /**
   * Clear the value of the specified row.  This bypasses the
   * DataField which may be attached to the control.  Use this method
   * to indicate that there's no valid value currently.
   */
  void ClearValue(unsigned i) {
    GetControl(i).SetText(_T(""));
  }

  gcc_pure
  bool GetValueBoolean(unsigned i) const;

  gcc_pure
  int GetValueInteger(unsigned i) const;

  gcc_pure
  double GetValueFloat(unsigned i) const;

  gcc_pure
  Angle GetValueAngle(unsigned i) const;

  gcc_pure
  unsigned GetValueIntegerAngle(unsigned i) const;

  gcc_pure
  RoughTime GetValueRoughTime(unsigned i) const;

  gcc_pure
  const TCHAR *GetValueString(unsigned i) const {
    return GetDataField(i).GetAsString();
  }

  bool SaveValue(unsigned i, bool &value, bool negated = false) const;
  bool SaveValue(unsigned i, int &value) const;
  bool SaveValue(unsigned i, uint8_t &value) const;
  bool SaveValue(unsigned i, uint16_t &value) const;
  bool SaveValue(unsigned i, double &value) const;
  bool SaveValue(unsigned i, Angle &value_r) const;
  bool SaveValue(unsigned i, RoughTime &value_r) const;
  bool SaveValue(unsigned i, TCHAR *string, size_t max_size) const;

  template<size_t max>
  bool SaveValue(unsigned i, StringBuffer<TCHAR, max> &value) const {
    return SaveValue(i, value.data(), value.capacity());
  }

  bool SaveValue(unsigned i, const char *profile_key, TCHAR *string, size_t max_size) const;

  template<size_t max>
  bool SaveValue(unsigned i, const char *profile_key,
                 StringBuffer<TCHAR, max> &value) const {
    return SaveValue(i, profile_key, value.data(), value.capacity());
  }

  bool SaveValue(unsigned i, unsigned &value) const {
    return SaveValue(i, (int &)value);
  }

  bool SaveValue(unsigned i, const char *profile_key, bool &value, bool negated = false) const;
  bool SaveValue(unsigned i, const char *profile_key, int &value) const;
  bool SaveValue(unsigned i, const char *profile_key, uint8_t &value) const;
  bool SaveValue(unsigned i, const char *profile_key, uint16_t &value) const;
  bool SaveValue(unsigned i, const char *profile_key, double &value) const;

  bool SaveValue(unsigned i, const char *registry_key,
                 unsigned &value) const {
    return SaveValue(i, registry_key, (int &)value);
  }

  bool SaveValue(unsigned i, UnitGroup unit_group, double &value) const;

  bool SaveValue(unsigned i, UnitGroup unit_group,
                 const char *profile_key, double &value) const;

  bool SaveValue(unsigned i, UnitGroup unit_group,
                 const char *profile_key, unsigned int &value) const;

  template<typename T>
  bool SaveValueEnum(unsigned i, T &value) const {
    return SaveValue(i, EnumCast<T>()(value));
  }

  template<typename T>
  bool SaveValueEnum(unsigned i, const char *registry_key, T &value) const {
    return SaveValue(i, registry_key, EnumCast<T>()(value));
  }

  bool SaveValueFileReader(unsigned i, const char *profile_key);

protected:
  gcc_pure
  unsigned GetRecommendedCaptionWidth() const;

  void NextControlRect(PixelRect &rc, unsigned height) {
    assert(IsDefined());

    rc.top = rc.bottom;
    rc.bottom = rc.top + height;
  }

  gcc_pure
  PixelRect InitialControlRect(unsigned height) {
    assert(IsDefined());

    PixelRect rc = GetWindow().GetClientRect();
    rc.bottom = rc.top + height;
    return rc;
  }

  /**
   * Recalculate all button positions.
   */
  void UpdateLayout();

public:
  /* virtual methods from Widget */
  PixelSize GetMinimumSize() const override;
  PixelSize GetMaximumSize() const override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) override;
  void Unprepare() override;
  void Show(const PixelRect &rc) override;
  void Move(const PixelRect &rc) override;
  bool SetFocus() override;
};

#endif
