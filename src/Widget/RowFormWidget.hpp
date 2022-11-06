/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "WindowWidget.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Base.hpp"
#include "time/BrokenDate.hpp"
#include "time/FloatDuration.hxx"
#include "Repository/FileType.hpp"
#include "Units/Group.hpp"

#include <boost/container/static_vector.hpp>

#include <cassert>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>

struct DialogLook;
struct StaticEnumChoice;
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

    const Type type;

    /**
     * Only used for #type==WIDGET.
     */
    bool initialised, prepared, shown;

    /**
     * Shall this row be available?  If not, it is hidden and no
     * screen space is reserved for it.
     */
    bool available = true;

    /**
     * Shall this row be visible?  The "expert" flag overrides it in
     * global "non-expert" mode.
     */
    bool visible;

    /**
     * If true, then the row is only visible in "expert" mode.
     */
    bool expert = false;

    std::unique_ptr<Widget> widget;

    std::unique_ptr<Window> window;

    /**
     * The position determined by RowFormWidget::UpdateLayout().  This
     * attribute is strictly only necessary for widgets, because these
     * expect a PixelRect parameter in their Show() method.
     */
    PixelRect position;

    Row(Type _type) noexcept
      :type(_type), visible(false) {
      assert(_type == Type::DUMMY);
    }

    Row(Type _type, std::unique_ptr<Window> &&_window) noexcept
      :type(_type), visible(true),
       window(std::move(_window))
    {
      assert(_type != Type::DUMMY);
      assert(window != nullptr);
    }

    Row(std::unique_ptr<Widget> &&_widget) noexcept
      :type(Type::WIDGET),
       initialised(false), prepared(false), shown(false),
       visible(true),
       widget(std::move(_widget))
    {
      assert(widget != nullptr);
    }

    ~Row() noexcept{
      Unprepare();

      if (type == Type::WIDGET) {
        assert(widget != nullptr);
        assert(window == nullptr);
        assert(!shown);
        assert(!prepared);
      }
    }

    Row(const Row &) = delete;
    Row &operator=(const Row &) = delete;

    /**
     * Determines whether this row is available.  A row that is not
     * available is hidden and will not be considered in the layout.
     *
     * @param expert_mode true if the user has enabled "expert" mode
     */
    bool IsAvailable(bool expert_mode) const noexcept {
      return type != Row::Type::DUMMY && available &&
        (!expert || expert_mode);
    }

    void Unprepare() noexcept {
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

    [[gnu::pure]]
    Widget &GetWidget() noexcept {
      assert(widget != nullptr);
      assert(window == nullptr);

      return *widget;
    }

    [[gnu::pure]]
    const Widget &GetWidget() const noexcept {
      assert(widget != nullptr);
      assert(window == nullptr);

      return *widget;
    }

    [[gnu::pure]]
    Window &GetWindow() noexcept {
      assert(window != nullptr);

      return *window;
    }

    [[gnu::pure]]
    const Window &GetWindow() const noexcept {
      assert(window != nullptr);

      return *window;
    }

    [[gnu::pure]]
    WndProperty &GetControl() noexcept {
      assert(type == Type::EDIT);
      assert(window != nullptr);

      return (WndProperty &)*window;
    }

    [[gnu::pure]]
    const WndProperty &GetControl() const noexcept {
      assert(type == Type::EDIT);
      assert(window != nullptr);

      return (WndProperty &)*window;
    }

    /**
     * Will this row grow when there is excess screen space?
     */
    bool IsElastic(const DialogLook &look, bool vertical) const noexcept {
      return GetMaximumHeight(look, vertical)
        > GetMinimumHeight(look, vertical);
    }

    [[gnu::pure]]
    unsigned GetMinimumHeight(const DialogLook &look,
                              bool vertical) const noexcept;

    [[gnu::pure]]
    unsigned GetMaximumHeight(const DialogLook &look,
                              bool vertical) const noexcept;

    void UpdateLayout(ContainerWindow &parent, const PixelRect &_position,
                      int caption_width) noexcept;

    void SetVisible(ContainerWindow &parent, bool _visible) noexcept;

    /**
     * Show the Window/Widget, but do not update the #visible flag.
     */
    void Show(ContainerWindow &parent) noexcept;

    /**
     * Hide the Window/Widget, but do not update the #visible flag.
     */
    void Hide() noexcept;

    [[gnu::pure]]
    bool HasFocus() const noexcept;
  };

  const DialogLook &look;

  /**
   * True if "vertical" layout is enabled.  It means that edit control
   * captions are above the value instead of left of the value.
   */
  const bool vertical;

  boost::container::static_vector<Row, 32u> rows;

public:
  RowFormWidget(const DialogLook &look, bool vertical=false) noexcept;
  ~RowFormWidget() noexcept override;

protected:
  const DialogLook &GetLook() const noexcept {
    return look;
  }

  Window &Add(Row::Type type, std::unique_ptr<Window> window) noexcept;

  std::unique_ptr<WndProperty> CreateEdit(const TCHAR *label,
                                          const TCHAR *help=nullptr,
                                          bool read_only=false) noexcept;

public:
  /**
   * Add a "dummy" row.  It does not have a #Window and its height is
   * zero.  This may be used to "reserve" a row index when a row is
   * not present in a certain instance of the form.
   */
  void AddDummy() noexcept {
    rows.emplace_back(Row::Type::DUMMY);
  }

  /**
   * Add a #Widget row.  The object will be deleted automatically.
   */
  Widget &Add(std::unique_ptr<Widget> widget) noexcept {
    rows.emplace_back(std::move(widget));
    return *rows.back().widget;
  }

  Window &Add(std::unique_ptr<Window> window) noexcept {
    Add(Row::Type::GENERIC, std::move(window));
    return *rows.back().window;
  }

  /**
   * Add a #Window that fills the remaining vertical space at the
   * bottom.  It must be the last row, and there can only be one.
   */
  void AddRemaining(std::unique_ptr<Window> window) noexcept {
    Add(Row::Type::REMAINING, std::move(window));
  }

  WndProperty *Add(const TCHAR *label, const TCHAR *help=nullptr,
                   bool read_only=false) noexcept;

  /**
   * Add a read-only control.  You can use SetText() to update its
   * text.
   */
  void AddReadOnly(const TCHAR *label, const TCHAR *help=nullptr,
                   const TCHAR *text=nullptr) noexcept;

  /**
   * Add a read-only control displaying a floating-point value.  Use
   * LoadValue() to update the displayed value.
   */
  void AddReadOnly(const TCHAR *label, const TCHAR *help,
                   const TCHAR *display_format,
                   double value) noexcept;

  /**
   * Add a read-only control displaying a floating-point value.  Use
   * LoadValue() to update the displayed value.
   */
  void AddReadOnly(const TCHAR *label, const TCHAR *help,
                   const TCHAR *display_format,
                   UnitGroup unit_group, double value) noexcept;

  /**
   * Add a read-only control displaying a boolean value.  Use
   * LoadValue() to update the displayed value.
   */
  void AddReadOnly(const TCHAR *label, const TCHAR *help,
                   bool value) noexcept;

  WndProperty *Add(const TCHAR *label, const TCHAR *help,
                   DataField *df) noexcept;

  WndProperty *AddBoolean(const TCHAR *label, const TCHAR *help,
                          bool value=false,
                          DataFieldListener *listener=nullptr) noexcept;

  WndProperty *AddInteger(const TCHAR *label, const TCHAR *help,
                          const TCHAR *display_format,
                          const TCHAR *edit_format,
                          int min_value, int max_value, int step, int value,
                          DataFieldListener *listener=nullptr) noexcept;

  WndProperty *AddFloat(const TCHAR *label, const TCHAR *help,
                        const TCHAR *display_format,
                        const TCHAR *edit_format,
                        double min_value, double max_value,
                        double step, bool fine,
                        double value,
                        DataFieldListener *listener=nullptr) noexcept;

  WndProperty *AddFloat(const TCHAR *label, const TCHAR *help,
                        const TCHAR *display_format,
                        const TCHAR *edit_format,
                        double min_value, double max_value,
                        double step, bool fine,
                        UnitGroup unit_group, double value,
                        DataFieldListener *listener=nullptr) noexcept;

  WndProperty *AddAngle(const TCHAR *label, const TCHAR *help,
                        Angle value, unsigned step, bool fine,
                        DataFieldListener *listener=nullptr) noexcept;

  WndProperty *AddEnum(const TCHAR *label, const TCHAR *help,
                       const StaticEnumChoice *list, unsigned value=0,
                       DataFieldListener *listener=nullptr) noexcept;

  WndProperty *AddEnum(const TCHAR *label, const TCHAR *help,
                       DataFieldListener *listener=nullptr) noexcept;

  WndProperty *AddText(const TCHAR *label, const TCHAR *help,
                       const TCHAR *content,
                       DataFieldListener *listener=nullptr) noexcept;

  /**
   * Add a password edit control.  The password is obfuscated while
   * not editing.
   */
  WndProperty *AddPassword(const TCHAR *label, const TCHAR *help,
                           const TCHAR *content) noexcept;

  WndProperty *AddDuration(const TCHAR *label, const TCHAR *help,
                           std::chrono::seconds min_value,
                           std::chrono::seconds max_value,
                           std::chrono::seconds step,
                           std::chrono::seconds value,
                           unsigned max_tokens = 2,
                           DataFieldListener *listener=nullptr) noexcept;

  template<class Rep, class Period>
  WndProperty *AddDuration(const TCHAR *label, const TCHAR *help,
                           std::chrono::seconds min_value,
                           std::chrono::seconds max_value,
                           std::chrono::seconds step,
                           const std::chrono::duration<Rep,Period> &value,
                           unsigned max_tokens = 2,
                           DataFieldListener *listener=nullptr) noexcept {
    return AddDuration(label, help, min_value, max_value, step,
                       std::chrono::round<std::chrono::seconds>(value),
                       max_tokens, listener);
  }

  WndProperty *AddDate(const TCHAR *label, const TCHAR *help,
                       BrokenDate date,
                       DataFieldListener *listener=nullptr) noexcept;

  WndProperty *AddRoughTime(const TCHAR *label, const TCHAR *help,
                            RoughTime value, RoughTimeDelta time_zone,
                            DataFieldListener *listener=nullptr) noexcept;

  void AddSpacer() noexcept;

  WndProperty *AddFile(const TCHAR *label, const TCHAR *help,
                       const char *profile_key, const TCHAR *filters,
                       FileType file_type,
                       bool nullable = true) noexcept;

  WndProperty *AddFile(const TCHAR *label, const TCHAR *help,
                       const char *profile_key, const TCHAR *filters,
                       bool nullable = true) noexcept {
    return AddFile(label, help, profile_key, filters, FileType::UNKNOWN,
                   nullable);
  }

  /**
   * Add a read-only multi-line control.  You can use
   * SetMultiLineText() to update its text.
   */
  void AddMultiLine(const TCHAR *text=nullptr) noexcept;

  Button *AddButton(const TCHAR *label, std::function<void()> callback) noexcept;

  [[gnu::pure]]
  Widget &GetRowWidget(unsigned i) noexcept {
    return rows[i].GetWidget();
  }

  [[gnu::pure]]
  Window &GetRow(unsigned i) noexcept {
    return rows[i].GetWindow();
  }

  [[gnu::pure]]
  const Window &GetRow(unsigned i) const noexcept {
    return rows[i].GetWindow();
  }

  void SetReadOnly(unsigned i, bool read_only=true) noexcept {
    GetControl(i).SetReadOnly(read_only);
  }

  void SetRowEnabled(unsigned i, bool enabled) noexcept {
    GetRow(i).SetEnabled(enabled);
  }

  /**
   * Modify the "available" flag on this row.
   */
  void SetRowAvailable(unsigned i, bool available) noexcept;

  void SetRowVisible(unsigned i, bool visible) noexcept;

  void ShowRow(unsigned i) noexcept {
    SetRowVisible(i, true);
  }

  void HideRow(unsigned i) noexcept {
    SetRowVisible(i, false);
  }

  /**
   * Enable the "expert" flag on this row: hide unless the form is in
   * "expert" mode.  This must be called before the first Show()
   * invocation.
   */
  void SetExpertRow(unsigned i) noexcept;

  [[gnu::pure]]
  Window &GetGeneric(unsigned i) noexcept {
    return rows[i].GetWindow();
  }

  [[gnu::pure]]
  WndProperty &GetControl(unsigned i) noexcept {
    return rows[i].GetControl();
  }

  [[gnu::pure]]
  const WndProperty &GetControl(unsigned i) const noexcept {
    return rows[i].GetControl();
  }

  /**
   * Update the text of a multi line control.
   */
  void SetText(unsigned i, const TCHAR *text) noexcept {
    assert(text != nullptr);

    WndProperty &control = GetControl(i);
    assert(control.GetDataField() == nullptr);
    control.SetText(text);
  }

  void ClearText(unsigned i) noexcept {
    SetText(i, _T(""));
  }

  /**
   * Update the text of a multi line control.
   */
  void SetMultiLineText(unsigned i, const TCHAR *text) noexcept;

  [[gnu::pure]]
  DataField &GetDataField(unsigned i) noexcept {
    DataField *df = GetControl(i).GetDataField();
    assert(df != nullptr);
    return *df;
  }

  [[gnu::pure]]
  const DataField &GetDataField(unsigned i) const noexcept {
    const DataField *df = GetControl(i).GetDataField();
    assert(df != nullptr);
    return *df;
  }

  /**
   * Compare a row's data field with the given reference (by their
   * addresses).
   */
  [[gnu::pure]]
  bool IsDataField(unsigned i, const DataField &df) const noexcept {
    return &df == &GetDataField(i);
  }

  void LoadValue(unsigned i, int value) noexcept;
  void LoadValue(unsigned i, bool value) noexcept;
  void LoadValueEnum(unsigned i, const TCHAR *text) noexcept;
  void LoadValueEnum(unsigned i, unsigned value) noexcept;

  template<typename T>
  requires(std::is_enum_v<T>)
  void LoadValueEnum(unsigned i, T value) noexcept {
    LoadValueEnum(i, unsigned(value));
  }

  void LoadValue(unsigned i, const TCHAR *value) noexcept;

  void LoadValue(unsigned i, double value) noexcept;
  void LoadValue(unsigned i, Angle value) noexcept;
  void LoadValue(unsigned i, double value, UnitGroup unit_group) noexcept;

  void LoadValue(unsigned i, RoughTime value) noexcept;

  /**
   * Load a value into a control created by AddDuration().
   */
  void LoadValueDuration(unsigned i, std::chrono::seconds value) noexcept;

  /**
   * Return the raw text of the #WndProperty, bypassing the
   * #DataField.
   */
  const TCHAR *GetText(unsigned i) const noexcept {
    return GetControl(i).GetText();
  }

  /**
   * Clear the value of the specified row.  This bypasses the
   * DataField which may be attached to the control.  Use this method
   * to indicate that there's no valid value currently.
   */
  void ClearValue(unsigned i) noexcept {
    GetControl(i).SetText(_T(""));
  }

  [[gnu::pure]]
  bool GetValueBoolean(unsigned i) const noexcept;

  [[gnu::pure]]
  int GetValueInteger(unsigned i) const noexcept;

  [[gnu::pure]]
  double GetValueFloat(unsigned i) const noexcept;

  [[gnu::pure]]
  Angle GetValueAngle(unsigned i) const noexcept;

  [[gnu::pure]]
  unsigned GetValueIntegerAngle(unsigned i) const noexcept;

  [[gnu::pure]]
  unsigned GetValueEnum(unsigned i) const noexcept;

  [[gnu::pure]]
  std::chrono::seconds GetValueTime(unsigned i) const noexcept;

  [[gnu::pure]]
  RoughTime GetValueRoughTime(unsigned i) const noexcept;

  [[gnu::pure]]
  const TCHAR *GetValueString(unsigned i) const noexcept {
    return GetDataField(i).GetAsString();
  }

  bool SaveValue(unsigned i, bool &value, bool negated = false) const noexcept;

#if defined(__clang__) && __clang_major__ < 15
  // C++20 concepts not implemented in libc++ 14 (Android NDK r25)
  template<typename T>
  requires std::is_integral_v<T>
#else
  template<std::integral T>
#endif
  bool SaveValueInteger(unsigned i, T &value) const noexcept {
    int new_value = GetValueInteger(i);

    if constexpr (std::is_unsigned_v<T>)
      if (new_value < 0)
        return false;

    const T new_t = static_cast<T>(new_value);
    if (new_t == value)
      return false;

    value = new_t;
    return true;
  }

  bool SaveValue(unsigned i, double &value) const noexcept;
  bool SaveValue(unsigned i, Angle &value_r) const noexcept;
  bool SaveValue(unsigned i, std::chrono::seconds &value) const noexcept;

  template<class Rep, class Period>
  bool SaveValue(unsigned i,
                 std::chrono::duration<Rep,Period> &value_r) const noexcept {
    auto value = std::chrono::round<std::chrono::seconds>(value_r);
    if (!SaveValue(i, value))
      return false;

    value_r = std::chrono::duration_cast<std::chrono::duration<Rep,Period>>(value);
    return true;
  }

  bool SaveValue(unsigned i, RoughTime &value_r) const noexcept;
  bool SaveValue(unsigned i, TCHAR *string, size_t max_size) const noexcept;

  template<size_t max>
  bool SaveValue(unsigned i, BasicStringBuffer<TCHAR, max> &value) const noexcept {
    return SaveValue(i, value.data(), value.capacity());
  }

  bool SaveValue(unsigned i, const char *profile_key,
                 TCHAR *string, size_t max_size) const noexcept;

  template<size_t max>
  bool SaveValue(unsigned i, const char *profile_key,
                 BasicStringBuffer<TCHAR, max> &value) const noexcept {
    return SaveValue(i, profile_key, value.data(), value.capacity());
  }

  bool SaveValue(unsigned i, const char *profile_key, bool &value,
                 bool negated = false) const noexcept;

  template<typename T>
  bool SaveValueInteger(unsigned i, const char *registry_key,
                        T &value) const noexcept {
    bool result = SaveValueInteger(i, value);
    if (result)
      SetProfile(registry_key, value);

    return result;
  }

  bool SaveValue(unsigned i, const char *profile_key, double &value) const noexcept;
  bool SaveValue(unsigned i, const char *profile_key, BrokenDate &value) const noexcept;
  bool SaveValue(unsigned i, const char *profile_key,
                 std::chrono::seconds &value) const noexcept;

  template<class Rep, class Period>
  bool SaveValue(unsigned i, const char *profile_key,
                 std::chrono::duration<Rep,Period> &value_r) const noexcept {
    auto value = std::chrono::round<std::chrono::seconds>(value_r);
    if (!SaveValue(i, profile_key, value))
      return false;

    value_r = std::chrono::duration_cast<std::chrono::duration<Rep,Period>>(value);
    return true;
  }

  bool SaveValue(unsigned i, UnitGroup unit_group, double &value) const noexcept;

  bool SaveValue(unsigned i, UnitGroup unit_group,
                 const char *profile_key, double &value) const noexcept;

  bool SaveValue(unsigned i, UnitGroup unit_group,
                 const char *profile_key, unsigned int &value) const noexcept;

#if defined(__clang__) && __clang_major__ < 15
  // C++20 concepts not implemented in libc++ 14 (Android NDK r25)
  template<typename T>
  requires std::is_integral_v<T> && std::is_unsigned_v<T>
#else
  template<std::unsigned_integral T>
#endif
  bool SaveValueEnum(unsigned i, T &value) const noexcept {
    const auto new_value = static_cast<T>(GetValueEnum(i));
    if (new_value == value)
      return false;

    value = new_value;
    return true;
  }

  template<typename T>
  requires std::is_enum_v<T>
  bool SaveValueEnum(unsigned i, T &value) const noexcept {
    return SaveValueEnum(i, reinterpret_cast<std::underlying_type_t<T> &>(value));
  }

  template<typename T>
  bool SaveValueEnum(unsigned i, const char *registry_key,
                     T &value) const noexcept {
    bool result = SaveValueEnum(i, value);
    if (result)
      SetProfile(registry_key, static_cast<unsigned>(value));

    return result;
  }

  bool SaveValueFileReader(unsigned i, const char *profile_key) noexcept;

private:
  static void SetProfile(const char *registry_key, unsigned value) noexcept;

protected:
  [[gnu::pure]]
  unsigned GetRecommendedCaptionWidth() const noexcept;

  void NextControlRect(PixelRect &rc, unsigned height) noexcept {
    assert(IsDefined());

    rc.top = rc.bottom;
    rc.bottom = rc.top + height;
  }

  [[gnu::pure]]
  PixelRect InitialControlRect(unsigned height) noexcept {
    assert(IsDefined());

    PixelRect rc = GetWindow().GetClientRect();
    rc.bottom = rc.top + height;
    return rc;
  }

  /**
   * Recalculate all button positions.
   */
  void UpdateLayout() noexcept;

public:
  /* virtual methods from Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
};
