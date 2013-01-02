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

#ifndef XCSOAR_DATA_FIELD_ENUM_HPP
#define XCSOAR_DATA_FIELD_ENUM_HPP

#include "Base.hpp"
#include "Util/NonCopyable.hpp"
#include "Util/StaticArray.hpp"

/**
 * A struct that is used for static initialisation of the enum list.
 */
struct StaticEnumChoice {
  unsigned id;
  const TCHAR *display_string;
  const TCHAR *help;
};

class DataFieldEnum: public DataField
{
public:
  class Entry : private NonCopyable {
    unsigned id;
    TCHAR *string;
    TCHAR *display_string;
    TCHAR *mHelp;

  public:
    Entry():string(NULL), display_string(NULL), mHelp(NULL) {}
    ~Entry();

    Entry(Entry &&other)
      :id(other.id), string(other.string),
       display_string(other.display_string), mHelp(other.mHelp) {
      other.string = other.display_string = other.mHelp = NULL;
    }

    Entry &operator=(Entry &&other) {
        free(string);

        if (display_string != string)
          free(display_string);

        free(mHelp);

        id = other.id;
        string = other.string;
        display_string = other.display_string;
        mHelp = other.mHelp;

        other.string = other.display_string = other.mHelp = NULL;

        return *this;
    }

#if GCC_VERSION < 40500
    /* we need this copy constructor+operator because std::sort() does
       not use "move" semantics in older libstdc++ versions */

    Entry(const Entry &other)
      :NonCopyable(), id(other.id),
       string(other.string != NULL ? _tcsdup(other.string) : NULL),
       display_string(other.display_string != NULL
                      ? _tcsdup(other.display_string) : NULL),
       mHelp(other.mHelp != NULL ? _tcsdup(other.mHelp) : NULL) {
    }

    Entry &operator=(const Entry &other) {
        free(string);

        if (display_string != string)
          free(display_string);

        free(mHelp);

        id = other.id;

        string = other.string != NULL ? _tcsdup(other.string) : NULL;
        display_string = other.display_string != NULL
          ? _tcsdup(other.display_string) : NULL;
        mHelp = other.mHelp != NULL ? _tcsdup(other.mHelp) : NULL;

        return *this;
    }
#endif

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
      return mHelp;
    }

    void SetString(const TCHAR *_string);
    void Set(unsigned _id, const TCHAR *_string,
             const TCHAR *_display_string=NULL,
             const TCHAR *_help=NULL);
  };

private:
  StaticArray<Entry, 128> entries;
  unsigned int value;

public:
  DataFieldEnum(DataAccessCallback OnDataAccess) :
    DataField(Type::ENUM, true, OnDataAccess),
    value(0) {}

  gcc_pure
  bool Exists(const TCHAR *text) const {
    return Find(text) >= 0;
  }

  void Inc();
  void Dec();
  virtual ComboList *CreateComboList() const;

  void replaceEnumText(unsigned int i, const TCHAR *Text);

  bool AddChoice(unsigned id, const TCHAR *text, const TCHAR *display_string=NULL,
                 const TCHAR *help=NULL);

  /**
   * Add choices from the specified NULL-terminated list (the last
   * entry has a NULL display_string).  All display strings and help
   * texts are translated with gettext() by this method.
   */
  void AddChoices(const StaticEnumChoice *list);

  bool addEnumText(const TCHAR *text, unsigned id, const TCHAR *help=NULL) {
    return AddChoice(id, text, NULL, help);
  }

  unsigned addEnumText(const TCHAR *Text, const TCHAR *display_string=NULL,
                       const TCHAR *ItemHelpText=NULL);
  void addEnumTexts(const TCHAR *const*list);

  gcc_pure
  virtual int GetAsInteger() const;

  gcc_pure
  virtual const TCHAR *GetAsString() const;
  virtual const TCHAR *GetAsDisplayString() const;

  /**
   * @return help of current enum item or NULL if current item has no help
   */
  const TCHAR *GetHelp() const;

  /**
   * @param value True if display item help in text box below picker
   * Displays help strings associated with enums Items
   */
  void EnableItemHelp(bool value) { item_help_enabled = value; }

  void Set(int Value);

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

  virtual void SetAsInteger(int Value);
  virtual void SetAsString(const TCHAR *Value);
  void Sort(int startindex = 0);

  gcc_pure
  unsigned Count() const {
    return entries.size();
  }
  unsigned getItem(unsigned index) const;

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
