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
  const char *display_string;
  const char *help;

  constexpr StaticEnumChoice(std::nullptr_t n) noexcept
    :id(0), display_string(n), help(n) {}

  template<typename T>
  requires(std::is_same_v<T, unsigned> || std::is_same_v<T, int> ||
           std::is_enum_v<T>)
  constexpr StaticEnumChoice(T _id, const char *_display_string,
                             const char *_help=nullptr) noexcept
    :id(static_cast<unsigned>(_id)), display_string(_display_string), help(_help) {}
};

class DataFieldEnum final : public DataField {
public:
  class Entry {
    unsigned id;
    char *string;
    char *display_string;
    char *help;

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

    const char *GetString() const noexcept {
      return string;
    }

    const char *GetDisplayString() const noexcept {
      return display_string;
    }

    const char *GetHelp() const noexcept {
      return help;
    }

    void SetString(const char *_string) noexcept;
    void SetDisplayString(const char *_string) noexcept;
    void Set(unsigned _id, const char *_string,
             const char *_display_string=nullptr,
             const char *_help=nullptr) noexcept;
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
  bool Exists(const char *text) const noexcept {
    return Find(text) >= 0;
  }

  void replaceEnumText(std::size_t index, const char *Text) noexcept;

  void SetDisplayString(std::size_t index, const char *_string) noexcept {
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

  bool AddChoice(unsigned id, const char *text,
                 const char *display_string=nullptr,
                 const char *help=nullptr) noexcept;

  /**
   * Add choices from the specified nullptr-terminated list (the last
   * entry has a nullptr display_string).  All display strings and help
   * texts are translated with gettext() by this method.
   */
  void AddChoices(const StaticEnumChoice *list) noexcept;

  bool addEnumText(const char *text, unsigned id,
                   const char *help=nullptr) noexcept {
    return AddChoice(id, text, nullptr, help);
  }

  unsigned addEnumText(const char *Text, const char *display_string=nullptr,
                       const char *ItemHelpText=nullptr) noexcept;
  void addEnumTexts(const char *const*list) noexcept;

  /**
   * @return help of current enum item or nullptr if current item has no help
   */
  const char *GetHelp() const noexcept;

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
  bool SetValue(const char *text) noexcept;

  bool ModifyValue(unsigned new_value) noexcept;

  template<typename T>
  requires(std::is_enum_v<T>)
  bool ModifyValue(T value) noexcept {
    return ModifyValue(unsigned(value));
  }

  bool ModifyValue(const char *text) noexcept;

  /**
   * Set the value to the specified string.  If there is no choice
   * with the string, a new one is added.
   *
   * @return the new integer value
   */
  int SetStringAutoAdd(const char *text) noexcept;

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
  const char *GetAsString() const noexcept override;
  const char *GetAsDisplayString() const noexcept override;
  ComboList CreateComboList(const char *reference) const noexcept override;
  void SetFromCombo(int iDataFieldIndex, const char *sValue) noexcept override;

protected:
  /**
   * Finds an entry with the specified text.  Returns -1 if not found.
   */
  [[gnu::pure]]
  int Find(const char *text) const noexcept;

  [[gnu::pure]]
  int Find(unsigned id) const noexcept;

  void SetIndex(std::size_t new_value, bool invoke_callback) noexcept;
};
