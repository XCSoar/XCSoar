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

#ifndef XCSOAR_OPERATION_HPP
#define XCSOAR_OPERATION_HPP

#include "Util/NonCopyable.hpp"

#include <tchar.h>

/**
 * An environment a complex operation runs in.  The operation may run
 * in a separate thread, and this class provides a bridge to the
 * calling thread.
 */
class OperationEnvironment : private NonCopyable {
public:
  /**
   * Sleep for a fixed amount of time.  May return earlier if an event
   * occurs.
   */
  virtual void Sleep(unsigned ms) = 0;

  /**
   * Show a human-readable (localized) short text describing the
   * current state of the operation.
   */
  virtual void SetText(const TCHAR *text) = 0;

  /**
   * Initialize the progress bar, and set the maximum value which will
   * mean "100% done".  The default value is 0, which means "no
   * progress bar".
   */
  virtual void SetProgressRange(unsigned range) = 0;

  /**
   * Set the current position of the progress bar.  Must not be bigger
   * than the configured range.
   */
  virtual void SetProgressPosition(unsigned position) = 0;
};

class NullOperationEnvironment : public OperationEnvironment {
public:
  virtual void Sleep(unsigned ms);
  virtual void SetText(const TCHAR *text);
  virtual void SetProgressRange(unsigned range);
  virtual void SetProgressPosition(unsigned position);
};

class QuietOperationEnvironment : public NullOperationEnvironment {
public:
  virtual void Sleep(unsigned ms);
};

class VerboseOperationEnvironment : public QuietOperationEnvironment {
public:
  virtual void SetText(const TCHAR *text);
  virtual void SetProgressRange(unsigned range);
  virtual void SetProgressPosition(unsigned position);
  void Hide();
};

#endif
