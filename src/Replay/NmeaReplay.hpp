/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef NMEA_REPLAY_HPP
#define NMEA_REPLAY_HPP

#include "Math/fixed.hpp"
#include "AbstractReplay.hpp"

#include <tchar.h>
#include <windef.h> /* for MAX_PATH */

class FileLineReaderA;

class NmeaReplay: public AbstractReplay
{
  TCHAR file_name[MAX_PATH];
  FileLineReaderA *reader;

public:
  NmeaReplay();
  ~NmeaReplay();

  bool Update();
  void Stop();
  void Start();
  const TCHAR* GetFilename();
  void SetFilename(const TCHAR *name);

protected:
  virtual bool UpdateTime();
  virtual void ResetTime() = 0;
  virtual void on_bad_file() = 0;
  virtual void on_sentence(const char *line) = 0;

private:
  bool OpenFile();
  void CloseFile();
  bool ReadUntilRMC(bool ignore);
};

#endif
