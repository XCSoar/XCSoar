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

#ifndef XCSOAR_DATA_FIELD_ENUM_HPP
#define XCSOAR_DATA_FIELD_ENUM_HPP

#include "Base.hpp"
#include "Util/StaticArray.hxx"

#include <utility>

/**
 * A struct that is used for static initialisation of the enum list.
 */
struct StaticEnumChoice {
  unsigned id;
  const TCHAR *display_string;
  const TCHAR *help;
};

class DataFieldEnum final : public DataField {
public:
  class Entry {
    unsigned id;
    TCHAR *string;
    TCHAR *display_string;
    TCHAR *help;

  public:
    Entry():string(nullptr), display_string(nullptr), help(nullptr) {}
    ~Entry();

    Entry(const Entry &) = delete;

    Entry(Entry &&other)
      :id(other.id), string(other.string),
       display_string(other.display_string), help(other.help) {
      other.string = other.display_string = other.help = nullptr;
    }

    Entry &operator=(Entry &&other) {
      id = other.id;
      std::swap(string, other.string);
      std::swap(display_string, other.display_string);
      std::swap(help, other.help);
      return *this;
    }

    friend void swap(Entry &a, Entry &b) {
      std::swap(a.id, b.id);
      std::swap(a.string, b.string);
      std::swap(a.display_string, b.display_string);
      std::swap(a.help, b.help);
    }

    unsigned GetId() const {
      return id;
    }

    const TCHAR *GetString() const {
      return string;
    }

    const TCHAR *GetDisplayString() const {
      return display_string;
    }

    const TCHAR *GetHelp() const {
      return help;
    }

    void SetString(const TCHAR *_string);
    void Set(unsigned _id, const TCHAR *_string,
             const TCHAR *_display_string=nullptr,
             const TCHAR *_help=nullptr);
  };

private:
  StaticArray<Entry, 128> entries;
  unsigned int value;

public:
  DataFieldEnum(DataFieldListener *listener=nullptr)
    :DataField(Type::ENUM, true, listener), value(0) {}

  gcc_pure
  unsigned GetValue() const;

  gcc_pure
  bool Exists(const TCHAR *text) const {
    return Find(text) >= 0;
  }

  void replaceEnumText(unsigned int i, const TCHAR *Text);

  /**
   * Clear the list of choices.  This will not notify the
   * DataFieldListener.
   */
  void ClearChoices() {
    entries.clear();
    value = 0;
  }

  bool AddChoice(unsigned id, const TCHAR *text,
                 const TCHAR *display_string=nullptr,
                 const TCHAR *help=nullptr);

  /**
   * Add choices from the specified nullptr-terminated list (the last
   * entry has a nullptr display_string).  All display strings and help
   * texts are translated with gettext() by this method.
   */
  void AddChoices(const StaticEnumChoice *list);

  bool addEnumText(const TCHAR *text, unsigned id, const TCHAR *help=nullptr) {
    return AddChoice(id, text, nullptr, help);
  }

  unsigned addEnumText(const TCHAR *Text, const TCHAR *display_string=nullptr,
                       const TCHAR *ItemHelpText=nullptr);
  void addEnumTexts(const TCHAR *const*list);

  /**
   * @return help of current enum item or nullptr if current item has no help
   */
  const TCHAR *GetHelp() const;

  /**
   * @param value True if display item help in text box below picker
   * Displays help strings associated with enums Items
   */
  void EnableItemHelp(bool value) override {
    item_help_enabled = value;
  }

  void Set(unsigned Value);

  /**
   * Select the item with the specified text (not display string).
   * Does not trigger the "modified" event.
   *
   * @return false if an item with the specified text was not found,
   * and therefore the value was not changed
   */
  bool Set(const TCHAR *text);

  /**
   * Set the value to the specified string.  If there is no choice
   * with the string, a new one is added.
   *
   * @return the new integer value
   */
  int SetStringAutoAdd(const TCHAR *text);

  void Sort(unsigned startindex = 0);

  gcc_pure
  unsigned Count() const {
    return entries.size();
  }
  unsigned getItem(unsigned index) const;

  /* virtual methods from class DataField */
  void Inc() override;
  void Dec() override;
  int GetAsInteger() const override;
  const TCHAR *GetAsString() const override;
  const TCHAR *GetAsDisplayString() const override;
  void SetAsInteger(int value) override;
  void SetAsString(const TCHAR *value) override;
  ComboList CreateComboList(const TCHAR *reference) const override;

protected:
  /**
   * Finds an entry with the specified text.  Returns -1 if not found.
   */
  gcc_pure
  int Find(const TCHAR *text) const;

  gcc_pure
  int Find(unsigned id) const;

  void SetIndex(unsigned new_value, bool invoke_callback);
};

#endif
