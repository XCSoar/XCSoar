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

#ifndef XCSOAR_IO_CONFIGURED_FILE_HPP
#define XCSOAR_IO_CONFIGURED_FILE_HPP

#include "Charset.hpp"

#include <memory>

class NLineReader;
class TLineReader;

/**
 * Opens a file whose name is configured in the profile.
 *
 * @param profile_key the profile key which is used to read the
 * file name
 * @return a NLineReader; nullptr
 * if there is no such setting, or if an error occurred opening the
 * file
 */
std::unique_ptr<NLineReader>
OpenConfiguredTextFileA(const char *profile_key);

/**
 * Opens a file whose name is configured in the profile.
 *
 * @param profile_key the profile key which is used to read the
 * file name
 * @param cs the character set of the input file
 * @return a TLineReader; nullptr if
 * there is no such setting, or if an error occurred opening the file
 */
std::unique_ptr<TLineReader>
OpenConfiguredTextFile(const char *profile_key,
                       Charset cs=Charset::UTF8);

/**
 * Opens a file whose name is configured in the profile.  If there is
 * no such setting, attempt to open a file from inside the map file.
 *
 * @param profile_key the profile key which is used to read the
 * file name
 * @param in_map_file if no profile setting is found, attempt to open
 * this file from inside the map file
 * @param cs the character set of the input file
 * @return a TLineReader; nullptr if
 * there is no such setting, or if an error occurred opening the file
 */
std::unique_ptr<TLineReader>
OpenConfiguredTextFile(const char *profile_key, const char *in_map_file,
                       Charset cs=Charset::UTF8);

#endif
