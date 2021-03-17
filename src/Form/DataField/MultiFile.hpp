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

#ifndef XCSOAR_NFILE_DATA_FIELD_HPP
#define XCSOAR_NFILE_DATA_FIELD_HPP

#include "Base.hpp"
#include "File.hpp"
#include "Repository/FileType.hpp"
#include "system/Path.hpp"
#include "util/StaticArray.hxx"

#include <set>
#include <vector>

/**
 * FileDataField wrapper that allows for selection of multiple files at once
 *
 */
class MultiFileDataField final : public DataField {

	FileDataField file_datafield;

	// Currently selected files
	std::set<unsigned int> current_selection;

	// Copy of the original files after Lookup()
	std::set<unsigned int> original_selection;

	FileType file_type;

	#ifdef _UNICODE
		std::wstring display_string;
	#else
		std::string display_string;
	#endif

public:

	MultiFileDataField(DataFieldListener *listener=nullptr);

	FileType GetFileType() const {
		return file_type;
	}

	void SetFileType(FileType _file_type){
		file_type = _file_type;
		file_datafield.SetFileType(_file_type);
	}

	/*
	 * Return a reference to the FileDataField
	 */
	FileDataField * GetFileDataField(){ return &file_datafield;}


	gcc_pure
	int Find(Path path) const;

	/**
	 * Iterate through the list and find the item where path matches
	 * text. If found, the item is set as selected and added to the original_selection
	 */
	void Lookup(Path text);

	// Return the currently selected items
	gcc_pure
	std::vector<Path> GetPathFiles() const;

	//Set selection to the given index(es)
	void Set(unsigned new_value);
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

	gcc_pure
	Path GetItem(unsigned index) const;

	/**
	 * Virtual methods from the DataField class
	 */
	const TCHAR *GetAsString() const override;
	const TCHAR *GetAsDisplayString() const override;
	void SetAsInteger(int value) override;

	ComboList CreateComboList(const TCHAR *reference) const override;
	void SetFromCombo(int datafield_index, const TCHAR *string_value) override;

	/*
	 * Stub implementations for virtual methods from DataField class, because
	 * they don't make sense in the MultiFile scenario
	 */

	void Inc() override {}
	void Dec() override {}
	int GetAsInteger() const override {return 0;}

};

#endif
