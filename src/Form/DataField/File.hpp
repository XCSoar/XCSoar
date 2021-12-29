/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_FILE_DATA_FIELD_HPP
#define XCSOAR_FILE_DATA_FIELD_HPP

#include "Base.hpp"
#include "Repository/FileType.hpp"
#include "system/Path.hpp"
#include "util/StaticArray.hxx"
#include "util/StaticString.hxx"

#include <utility>

/**
 * #DataField specialisation that supplies options as a list of
 * files matching a suffix.  First entry is always blank for null entry.
 * 
 */
class FileDataField final : public DataField {
  typedef StaticArray<StaticString<32>, 8> PatternList;

public:
  /** FileList item */
  struct Item {
    /** Filename */
    Path filename;
    /** Path including Filename */
    AllocatedPath path;

    Item() noexcept:filename(nullptr), path(nullptr) {}

    Item(Item &&src) noexcept
      :filename(src.filename), path(std::move(src.path))
    {
      src.filename = nullptr;
      src.path = nullptr;
    }

    Item(const Item &) = delete;

    Item &operator=(Item &&src) noexcept {
      std::swap(filename, src.filename);
      std::swap(path, src.path);
      return *this;
    }

    void Set(Path _path) noexcept;
  };

private:
  static constexpr unsigned MAX_FILES = 512;

  /** Index of the active file */
  unsigned int current_index;
  /** FileList item array */
  StaticArray<Item, MAX_FILES> files;

  FileType file_type;

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
  AllocatedPath postponed_value;

  /**
   * Stores the patterns while !loaded.
   */
  PatternList postponed_patterns;

public:
  /**
   * Constructor of the FileDataField class
   * @param OnDataAccess
   */
  explicit FileDataField(DataFieldListener *listener=nullptr) noexcept;

  FileType GetFileType() const noexcept {
    return file_type;
  }

  void SetFileType(FileType _file_type) noexcept {
    file_type = _file_type;
  }

  /**
   * Adds a filename/filepath couple to the filelist
   */
  void AddFile(Path path) noexcept;

  /**
   * Adds an empty row to the filelist
   */
  void AddNull() noexcept;

  /**
   * Returns the number of files in the list
   * @return The number of files in the list
   */
  [[gnu::pure]]
  unsigned GetNumFiles() const noexcept;

  [[gnu::pure]]
  int Find(Path path) const noexcept;

  /**
   * Iterates through the file list and tries to find an item where the path
   * is equal to the given text, if found the selection is changed to
   * that item
   * @param text PathFile to search for
   */
  void SetValue(Path new_value) noexcept;
  void ModifyValue(Path new_value) noexcept;

  /**
   * Force the value to the given path.  If the path is not in the
   * file list, add it.  This method does not check whether the file
   * really exists.
   */
  void ForceModify(Path path) noexcept;

  /**
   * Returns the PathFile of the currently selected item
   * @return The PathFile of the currently selected item
   */
  [[gnu::pure]]
  Path GetValue() const noexcept;

  /**
   * Sets the selection to the given index
   * @param Value The array index to select
   */
  void SetIndex(unsigned new_value) noexcept;
  void ModifyIndex(unsigned new_value) noexcept;

  /** Sorts the filelist by filenames */
  void Sort() noexcept;
  void ScanDirectoryTop(const TCHAR *filter) noexcept;

  /**
   * Scan multiple shell patterns.  Each pattern is terminated by a
   * null byte, and the list ends with an empty pattern.
   */
  void ScanMultiplePatterns(const TCHAR *patterns) noexcept;

  /** For use by other classes */
  [[gnu::pure]]
  unsigned size() const noexcept;

  [[gnu::pure]]
  Path GetItem(unsigned index) const noexcept;

  /* virtual methods from class DataField */
  void Inc() noexcept override;
  void Dec() noexcept override;
  int GetAsInteger() const noexcept override;
  const TCHAR *GetAsString() const noexcept override;
  const TCHAR *GetAsDisplayString() const noexcept override;
  void SetAsInteger(int value) noexcept override;
  ComboList CreateComboList(const TCHAR *reference) const noexcept override;

protected:
  void EnsureLoaded() noexcept;

  /**
   * Hack for our "const" methods, to allow them to load on demand.
   */
  void EnsureLoadedDeconst() const noexcept {
    const_cast<FileDataField *>(this)->EnsureLoaded();
  }
};

#endif
