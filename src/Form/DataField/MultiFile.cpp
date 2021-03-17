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

#include "MultiFile.hpp"
#include "ComboList.hpp"
#include "LocalPath.hpp"
#include "util/StringAPI.hxx"
#include "system/FileUtil.hpp"
#include "Language/Language.hpp"

#include <algorithm>

#include <windef.h> /* for MAX_PATH */
#include <cassert>
#include <stdlib.h>

#include <iostream>



MultiFileDataField::MultiFileDataField(DataFieldListener *listener) :
	DataField(Type::MULTI_FILE, false, listener){ }



void MultiFileDataField::SetAsInteger(int new_value) {

	Set(new_value);

}


// Find the index of path in files
int MultiFileDataField::Find(Path path) const {

	return file_datafield.Find(path);

}


void MultiFileDataField::Lookup(Path text){

	//file_datafield must be loaded to perform a Find()
	if (file_datafield.GetNumFiles() <= 0)
		return;

	auto i = Find(text);

	if (i >= 0){
		current_selection.insert(i);
		original_selection.insert(i);
	}

	UpdateDisplayString();
}

void MultiFileDataField::Restore(){

	current_selection.clear();
	current_selection = original_selection;

}

std::vector<Path> MultiFileDataField::GetPathFiles() const{

	std::vector<Path> paths;

	if (current_selection.empty()){
		paths.clear();
		return paths;
	}

	for (auto index : current_selection){

		paths.push_back(file_datafield.GetItem(index));
	}

	return paths;
}


void MultiFileDataField::ScanMultiplePatterns(const TCHAR *patterns){

	file_datafield.ScanMultiplePatterns(patterns);

}


void MultiFileDataField::Set(unsigned int new_value){

	if (current_selection.find(new_value) != current_selection.end()){
		return;
	} else {
		current_selection.insert(new_value);
	}

	UpdateDisplayString();
}


void MultiFileDataField::UnSet(Path path){

	auto i = Find(path);

	if (i >= 0){

		current_selection.erase(i);
	}

	UpdateDisplayString();
}


Path MultiFileDataField::GetItem(unsigned index) const {

	return file_datafield.GetItem(index);
}


ComboList MultiFileDataField::CreateComboList(const TCHAR *reference) const {

	return file_datafield.CreateComboList(reference);

}


void MultiFileDataField::SetFromCombo(int datafield_index, const TCHAR *string_value){

	current_selection.insert(datafield_index);

}


void MultiFileDataField::ForceModify(Path path){

	file_datafield.ForceModify(path);

	current_selection.insert(file_datafield.GetNumFiles() -1);
}


const TCHAR * MultiFileDataField::GetAsString() const { return _("");}

void MultiFileDataField::UpdateDisplayString() {

	display_string = _("");

	for (auto i : current_selection){

		display_string += file_datafield.GetItem(i).GetBase().c_str();
		display_string += _(" ");

	}
}

const TCHAR * MultiFileDataField::GetAsDisplayString() const {

	return display_string.c_str();
}
