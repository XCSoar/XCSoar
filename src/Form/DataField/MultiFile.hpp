// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifndef XCSOAR_MULTIFILE_DATA_FIELD_HPP
#define XCSOAR_MULTIFILE_DATA_FIELD_HPP

#include "Base.hpp"
#include "File.hpp"
#include "Repository/FileType.hpp"
#include "system/Path.hpp"
#include "util/tstring.hpp"

#include <vector>

/**
 * FileDataField wrapper that allows for selection of multiple files at once
 *
 */
class MultiFileDataField final : public DataField {

  FileDataField file_datafield;

  // Currently selected files
  std::vector<AllocatedPath> current_selection;

  // Copy of the original files after AddInitialPath()
  std::vector<AllocatedPath> original_selection;

  FileType file_type;

  tstring display_string;

public:
  explicit MultiFileDataField(DataFieldListener *listener = nullptr);

  FileType GetFileType() const { return file_type; }

  void SetFileType(FileType _file_type)
  {
    file_type = _file_type;
    file_datafield.SetFileType(_file_type);
  }

  /*
   * Return a reference to the FileDataField
   */
  FileDataField &GetFileDataField() { return file_datafield; }

  int Find(Path path) const;

  /**
   * Use this function to add paths when loading from config file to populate
   * original_selection and current_selection.
   */
  void AddInitialPath(Path text);

  // Return the currently selected items
  std::vector<Path> GetPathFiles() const;

  // Set selection to the given index(es)
  void AddValue(Path new_value);
  void ForceModify(Path path);
  void UnSet(Path path);

  // Restore the original_selection
  void Restore();

  void UpdateDisplayString();

  /**
   * Scan multiple shell patterns.  Each pattern is terminated by a
   * null byte, and the list ends with an empty pattern.
   */
  void ScanMultiplePatterns(const TCHAR *patterns);

  Path GetItem(unsigned index) const;

  /**
   * Virtual methods from the DataField class
   */
  const TCHAR *GetAsString() const noexcept override;
  const TCHAR *GetAsDisplayString() const noexcept override;

  ComboList CreateComboList(const TCHAR *reference) const noexcept override;
  void SetFromCombo(int datafield_index,
                    const TCHAR *string_value) noexcept override;

  /*
   * Stub implementations for virtual methods from DataField class, because
   * they don't make sense in the MultiFile scenario
   */

  void Inc() noexcept override {}
  void Dec() noexcept override {}
};

#endif
