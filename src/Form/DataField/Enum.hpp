// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Base.hpp"
#include "util/StaticArray.hxx"

#include <type_traits>
#include <utility>

/**
 * A struct that is used for static initialisation of the enum list.
 */
struct StaticEnumChoice {
  unsigned id;
  const TCHAR *display_string;
  const TCHAR *help;

  constexpr StaticEnumChoice(std::nullptr_t n) noexcept
    :id(0), display_string(n), help(n) {}

  template<typename T>
  requires(std::is_same_v<T, unsigned> || std::is_same_v<T, int> ||
           std::is_enum_v<T>)
  constexpr StaticEnumChoice(T _id, const TCHAR *_display_string,
                             const TCHAR *_help=nullptr) noexcept
    :id(static_cast<unsigned>(_id)), display_string(_display_string), help(_help) {}
};

class DataFieldEnum final : public DataField {
public:
  class Entry {
    unsigned id;
    TCHAR *string;
    TCHAR *display_string;
    TCHAR *help;

  public:
    Entry() noexcept:string(nullptr), display_string(nullptr), help(nullptr) {}
    ~Entry() noexcept;

    Entry(const Entry &) = delete;

    Entry(Entry &&other) noexcept
      :id(other.id), string(other.string),
       display_string(other.display_string), help(other.help) {
      other.string = other.display_string = other.help = nullptr;
    }

    Entry &operator=(Entry &&other) noexcept {
      id = other.id;
      std::swap(string, other.string);
      std::swap(display_string, other.display_string);
      std::swap(help, other.help);
      return *this;
    }

    friend void swap(Entry &a, Entry &b) noexcept {
      std::swap(a.id, b.id);
      std::swap(a.string, b.string);
      std::swap(a.display_string, b.display_string);
      std::swap(a.help, b.help);
    }

    unsigned GetId() const noexcept {
      return id;
    }

    const TCHAR *GetString() const noexcept {
      return string;
    }

    const TCHAR *GetDisplayString() const noexcept {
      return display_string;
    }

    const TCHAR *GetHelp() const noexcept {
      return help;
    }

    void SetString(const TCHAR *_string) noexcept;
    void SetDisplayString(const TCHAR *_string) noexcept;
    void Set(unsigned _id, const TCHAR *_string,
             const TCHAR *_display_string=nullptr,
             const TCHAR *_help=nullptr) noexcept;
  };

private:
  StaticArray<Entry, 128> entries;
  std::size_t value = 0;

public:
  DataFieldEnum(DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::ENUM, true, listener) {}

  [[gnu::pure]]
  unsigned GetValue() const noexcept;

  [[gnu::pure]]
  bool Exists(const TCHAR *text) const noexcept {
    return Find(text) >= 0;
  }

  void replaceEnumText(std::size_t index, const TCHAR *Text) noexcept;

  void SetDisplayString(std::size_t index, const TCHAR *_string) noexcept {
    entries[index].SetDisplayString(_string);
  }

  /**
   * Clear the list of choices.  This will not notify the
   * DataFieldListener.
   */
  void ClearChoices() noexcept {
    entries.clear();
    value = 0;
  }

  bool AddChoice(unsigned id, const TCHAR *text,
                 const TCHAR *display_string=nullptr,
                 const TCHAR *help=nullptr) noexcept;

  /**
   * Add choices from the specified nullptr-terminated list (the last
   * entry has a nullptr display_string).  All display strings and help
   * texts are translated with gettext() by this method.
   */
  void AddChoices(const StaticEnumChoice *list) noexcept;

  bool addEnumText(const TCHAR *text, unsigned id,
                   const TCHAR *help=nullptr) noexcept {
    return AddChoice(id, text, nullptr, help);
  }

  unsigned addEnumText(const TCHAR *Text, const TCHAR *display_string=nullptr,
                       const TCHAR *ItemHelpText=nullptr) noexcept;
  void addEnumTexts(const TCHAR *const*list) noexcept;

  /**
   * @return help of current enum item or nullptr if current item has no help
   */
  const TCHAR *GetHelp() const noexcept;

  /**
   * @param value True if display item help in text box below picker
   * Displays help strings associated with enums Items
   */
  void EnableItemHelp(bool value) noexcept override {
    item_help_enabled = value;
  }

  void SetValue(unsigned Value) noexcept;

  template<typename T>
  requires(std::is_enum_v<T>)
  void SetValue(T value) noexcept {
    SetValue(unsigned(value));
  }

  /**
   * Select the item with the specified text (not display string).
   * Does not trigger the "modified" event.
   *
   * @return false if an item with the specified text was not found,
   * and therefore the value was not changed
   */
  bool SetValue(const TCHAR *text) noexcept;

  bool ModifyValue(unsigned new_value) noexcept;

  template<typename T>
  requires(std::is_enum_v<T>)
  bool ModifyValue(T value) noexcept {
    return ModifyValue(unsigned(value));
  }

  bool ModifyValue(const TCHAR *text) noexcept;

  /**
   * Set the value to the specified string.  If there is no choice
   * with the string, a new one is added.
   *
   * @return the new integer value
   */
  int SetStringAutoAdd(const TCHAR *text) noexcept;

  void Sort(std::size_t startindex = 0) noexcept;

  [[gnu::pure]]
  std::size_t Count() const noexcept {
    return entries.size();
  }

  const auto &operator[](std::size_t index) const noexcept {
    return entries[index];
  }

  /* virtual methods from class DataField */
  void Inc() noexcept override;
  void Dec() noexcept override;
  const TCHAR *GetAsString() const noexcept override;
  const TCHAR *GetAsDisplayString() const noexcept override;
  ComboList CreateComboList(const TCHAR *reference) const noexcept override;
  void SetFromCombo(int iDataFieldIndex, const TCHAR *sValue) noexcept override;

protected:
  /**
   * Finds an entry with the specified text.  Returns -1 if not found.
   */
  [[gnu::pure]]
  int Find(const TCHAR *text) const noexcept;

  [[gnu::pure]]
  int Find(unsigned id) const noexcept;

  void SetIndex(std::size_t new_value, bool invoke_callback) noexcept;
};
