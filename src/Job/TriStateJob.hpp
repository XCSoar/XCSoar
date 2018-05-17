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

#ifndef XCSOAR_TRI_STATE_JOB_HPP
#define XCSOAR_TRI_STATE_JOB_HPP

#include "Job.hpp"
#include "Operation/Operation.hpp"

/* damn you, windows.h! */
#ifdef ERROR
#undef ERROR
#endif

enum class TriStateJobResult {
  SUCCESS, ERROR, CANCELLED
};

/**
 * A wrapper that keeps track of whether the job was successful,
 * cancelled or whether it failed.
 */
template<typename T>
class TriStateJob final : public Job, public T {
private:
  TriStateJobResult result;

public:
  TriStateJob() = default;

  template<typename... Args>
  explicit TriStateJob(Args&&... args)
    :T(std::forward<Args>(args)...) {}

  TriStateJobResult GetResult() const {
    return result;
  }

  virtual void Run(OperationEnvironment &env) {
    result = T::Run(env)
      ? TriStateJobResult::SUCCESS
      : (env.IsCancelled()
         ? TriStateJobResult::CANCELLED
         : TriStateJobResult::ERROR);
  }
};

#endif
