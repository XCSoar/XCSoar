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

#ifndef XCSOAR_DATA_FIELD_FILE_READER_HPP
#define XCSOAR_DATA_FIELD_FILE_READER_HPP

#include "Base.hpp"
#include "Util/NonCopyable.hpp"
#include "Util/StaticArray.hpp"
#include "Util/StaticString.hpp"

/**
 * #DataField specialisation that supplies options as a list of
 * files matching a suffix.  First entry is always blank for null entry.
 * 
 */
class DataFieldFileReader final : public DataField {
  typedef StaticArray<StaticString<32>, 8> PatternList;

public:
  /** FileList item */
  struct Item : private NonCopyable {
    /** Filename */
    const TCHAR *filename;
    /** Path including Filename */
    TCHAR *path;

    Item():filename(nullptr), path(nullptr) {}
    ~Item();
  };

private:
#ifdef _WIN32_WCE
  static constexpr unsigned MAX_FILES = 100;
#else
  static constexpr unsigned MAX_FILES = 512;
#endif

  /** Index of the active file */
  unsigned int mValue;
  /** FileList item array */
  StaticArray<Item, MAX_FILES> files;

  /**
   * Has the file list already been loaded?  This class tries to
   * postpone disk access for as long as possible, to reduce UI
   * latency.
   */
  bool loaded;

  /**
   * Set to true if Sort() has been called before the file list was
   * loaded.  It will trigger a call to Sort() after loading.
   */
  bool postponed_sort;

  /**
   * Used to store the value while !loaded.
   */
  StaticString<512> postponed_value;

  /**
   * Stores the patterns while !loaded.
   */
  PatternList postponed_patterns;

public:
  /**
   * Constructor of the DataFieldFileReader class
   * @param OnDataAccess
   */
  DataFieldFileReader(DataFieldListener *listener=nullptr);

  /**
   * Adds a filename/filepath couple to the filelist
   * @param fname The filename
   * @param fpname The filepath
   */
  void AddFile(const TCHAR *fname, const TCHAR *fpname);

  /**
   * Adds an empty row to the filelist
   */
  void AddNull();

  /**
   * Returns the number of files in the list
   * @return The number of files in the list
   */
  gcc_pure
  unsigned GetNumFiles() const;

  /**
   * Iterates through the file list and tries to find an item where the path
   * is equal to the given text, if found the selection is changed to
   * that item
   * @param text PathFile to search for
   */
  void Lookup(const TCHAR* text);

  /**
   * Returns the PathFile of the currently selected item
   * @return The PathFile of the currently selected item
   */
  gcc_pure
  const TCHAR *GetPathFile() const;

  /**
   * Sets the selection to the given index
   * @param Value The array index to select
   */
  void Set(unsigned Value);

  /** Sorts the filelist by filenames */
  void Sort();
  void ScanDirectoryTop(const TCHAR *filter);

  /**
   * Scan multiple shell patterns.  Each pattern is terminated by a
   * null byte, and the list ends with an empty pattern.
   */
  void ScanMultiplePatterns(const TCHAR *patterns);

  /** For use by other classes */
  gcc_pure
  unsigned size() const;

  gcc_pure
  const TCHAR *GetItem(unsigned index) const;

  /* virtual methods from class DataField */
  virtual void Inc() override;
  virtual void Dec() override;
  virtual int GetAsInteger() const override;
  virtual const TCHAR *GetAsString() const override;
  virtual const TCHAR *GetAsDisplayString() const override;
  virtual void SetAsInteger(int value) override;
  virtual ComboList CreateComboList(const TCHAR *reference) const override;

protected:
  void EnsureLoaded();

  /**
   * Hack for our "const" methods, to allow them to load on demand.
   */
  void EnsureLoadedDeconst() const {
    const_cast<DataFieldFileReader *>(this)->EnsureLoaded();
  }
};

#endif
