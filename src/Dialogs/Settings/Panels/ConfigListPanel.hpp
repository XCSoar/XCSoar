// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Dialogs/GroupedListPicker.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "util/StaticString.hxx"

#include <algorithm>
#include <chrono>
#include <vector>

/**
 * A page of the configuration which is a #GroupedListWidget.  It
 * keeps copies of the settings it edits, fills its list from them,
 * and fills it again when a value or the user level has changed;
 * Save() of the page writes the copies back.
 */
class ConfigListPanel : public GroupedListWidget {
  /** the user level the list was filled for */
  bool expert;

protected:
  /** a duration in seconds, as the settings hold them */
  using Duration = std::chrono::duration<unsigned>;

  ConfigListPanel() noexcept;

  /** Is the page filled for the expert user level? */
  bool IsExpert() const noexcept {
    return expert;
  }

  /** Copy the settings the page edits; called once by Prepare(). */
  virtual void LoadSettings() noexcept = 0;

  /** Add the groups and the items of the page, from the copies. */
  virtual void Fill() noexcept = 0;

  /**
   * Fill the list again, after a value or the user level has
   * changed.  A page which shows a view beside the list updates it
   * here.
   */
  virtual void Refresh() noexcept;

  /**
   * Add an item which opens the choice of an enumeration; the value
   * is taken over as soon as it is picked.
   *
   * @param help an explanation of the setting, shown above the
   * choice; nullptr for none
   */
  template<typename T>
  void AddEnumItem(const char *caption, const char *help,
                   const StaticEnumChoice *list, T &value) noexcept {
    AddItem(caption, [this, caption, help, list, &value](){
      unsigned v = (unsigned)value;
      if (PickEnum(caption, help, list, v)) {
        value = (T)v;
        Refresh();
      }
    }, {.value = GetEnumCaption(list, (unsigned)value), .chevron = true});
  }

  /**
   * Let the user pick a number from @p min to @p max, one choice per
   * step; @p format writes the caption of a value.
   *
   * @return true if the value has changed
   */
  template<typename F>
  static bool
  PickNumber(const char *caption, const char *help,
             int min, int max, int step, int &value, F &&format) noexcept
  {
    const unsigned n = (max - min) / step + 1;

    std::vector<StaticString<32>> captions(n);
    std::vector<PickerChoice> choices(n);

    for (unsigned i = 0; i < n; ++i) {
      format(captions[i], min + step * (int)i);
      choices[i] = {captions[i].c_str()};
    }

    /* the choice nearest to the value */
    const int current = std::clamp((value - min + step / 2) / step,
                                   0, (int)n - 1);

    const int picked = PickChoice(caption, help, choices, current);
    if (picked < 0)
      return false;

    const int new_value = min + step * picked;
    if (new_value == value)
      return false;

    value = new_value;
    return true;
  }

  /**
   * Add a switch which writes its state into @p value and fills the
   * list again, for the items which depend on it.  A tap beside the
   * switch only shows the explanation.
   *
   * @param subtitle a text below the caption; nullptr for none
   */
  void AddToggleItem(const char *caption, const char *help,
                     bool &value, const char *subtitle=nullptr) noexcept;

  /**
   * Add an item which opens the choice of a percentage, one choice
   * per step from @p min to @p max.
   */
  void AddPercentItem(const char *caption, const char *help,
                      int min, int max, int step, int &value) noexcept;

  /**
   * Add an item which opens the choice of an altitude, one choice
   * per step from @p min to @p max in the unit of the user; the
   * value is in metres.
   */
  void AddAltitudeItem(const char *caption, const char *help,
                       unsigned min, unsigned max, unsigned step,
                       unsigned &value) noexcept;

  /**
   * Add an item which opens the choice of a duration, one choice per
   * step from @p min to @p max seconds.
   */
  void AddDurationItem(const char *caption, const char *help,
                       unsigned min, unsigned max, unsigned step,
                       Duration &value) noexcept;

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
};
