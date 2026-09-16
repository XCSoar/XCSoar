// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Dialogs/GroupedListPicker.hpp"
#include "Widget/GroupedListWidget.hpp"

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
   * Add a switch which writes its state into @p value and fills the
   * list again, for the items which depend on it.  A tap beside the
   * switch only shows the explanation.
   */
  void AddToggleItem(const char *caption, const char *help,
                     bool &value) noexcept;

  /**
   * Add an item which opens the choice of a percentage, one choice
   * per step from @p min to @p max.
   */
  void AddPercentItem(const char *caption, const char *help,
                      int min, int max, int step, int &value) noexcept;

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
};
