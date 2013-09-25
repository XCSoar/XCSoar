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

#ifndef IGC_REPLAY_HPP
#define IGC_REPLAY_HPP

#include "AbstractReplay.hpp"
#include "IGC/IGCExtensions.hpp"
#include "Compiler.h"

class NLineReader;
struct IGCFix;

class IgcReplay: public AbstractReplay
{
  NLineReader *reader;

  IGCExtensions extensions;

public:
  IgcReplay(NLineReader *reader);
  virtual ~IgcReplay();

  virtual bool Update(NMEAInfo &data) override;

private:
  /**
   * Parse a line.
   *
   * @return true if a new fix was found
   */
  bool ScanBuffer(const char *buffer, IGCFix &fix, NMEAInfo &basic);

  /**
   * Read from the IGC file until a new fix was found.
   *
   * @return false on end-of-file
   */
  bool ReadPoint(IGCFix &fix, NMEAInfo &basic);
};

#endif
