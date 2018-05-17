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

#ifndef XCSOAR_JOB_DIALOG_HPP
#define XCSOAR_JOB_DIALOG_HPP

#include "Job/Runner.hpp"

#include <tchar.h>

struct DialogLook;
class SingleWindow;
class Job;

/**
 * Run the specified #Job in a separate thread, and show a modal
 * dialog while it is running.
 *
 * @param cancellable show a "cancel" button that allows aborting the
 * job
 * @return true if the job has finished (may have failed), false if
 * the job was cancelled by the user
 */
bool
JobDialog(SingleWindow &parent, const DialogLook &dialog_look,
          const TCHAR *caption, Job &job,
          bool cancellable=false);

class DialogJobRunner : public JobRunner {
  SingleWindow &parent;
  const DialogLook &dialog_look;
  const TCHAR *caption;
  bool cancellable;

public:
  DialogJobRunner(SingleWindow &_parent, const DialogLook &_dialog_look,
                  const TCHAR *_caption, bool _cancellable=false)
    :parent(_parent), dialog_look(_dialog_look),
     caption(_caption), cancellable(_cancellable) {}

  bool Run(Job &job) override;
};

#endif
