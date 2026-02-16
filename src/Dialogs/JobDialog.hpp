// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Job/Runner.hpp"

#include <tchar.h>

struct DialogLook;
namespace UI { class SingleWindow; }
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
JobDialog(UI::SingleWindow &parent, const DialogLook &dialog_look,
          const char *caption, Job &job,
          bool cancellable=false);

class DialogJobRunner : public JobRunner {
  UI::SingleWindow &parent;
  const DialogLook &dialog_look;
  const char *caption;
  bool cancellable;

public:
  DialogJobRunner(UI::SingleWindow &_parent, const DialogLook &_dialog_look,
                  const char *_caption, bool _cancellable=false)
    :parent(_parent), dialog_look(_dialog_look),
     caption(_caption), cancellable(_cancellable) {}

  bool Run(Job &job) override;
};
