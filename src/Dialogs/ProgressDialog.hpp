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

#ifndef XCSOAR_PROGRESS_DIALOG_HPP
#define XCSOAR_PROGRESS_DIALOG_HPP

#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Operation/Operation.hpp"
#include "ProgressWindow.hpp"

#include <functional>

class ProgressDialog
  : public WndForm, public QuietOperationEnvironment {

  ProgressWindow progress;

  Button cancel_button;
  std::function<void()> cancel_callback;

public:
  ProgressDialog(SingleWindow &parent, const DialogLook &dialog_look,
                 const TCHAR *caption);

  void AddCancelButton(std::function<void()> &&callback);

  /* virtual methods from class OperationEnvironment */

  void SetText(const TCHAR *text) override {
    progress.SetMessage(text);
  }

  void SetProgressRange(unsigned range) override {
    progress.SetRange(0, range);
  }

  void SetProgressPosition(unsigned position) override {
    progress.SetValue(position);
  }

  /* virtual methods from ActionListener */
  void OnAction(int id) override;
};

#endif
