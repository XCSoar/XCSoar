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

#ifndef XCSOAR_TASK_CALCULATOR_PANEL_HPP
#define XCSOAR_TASK_CALCULATOR_PANEL_HPP

#include "Form/XMLWidget.hpp"
#include "Form/Form.hpp"
#include "Math/fixed.hpp"

class TaskCalculatorPanel : public XMLWidget {
  WndForm &wf;

  const bool *task_modified;

  fixed emc;
  fixed cruise_efficiency;

public:
  TaskCalculatorPanel(WndForm &_wf, const bool *_task_modified)
    :wf(_wf), task_modified(_task_modified) {}

  const DialogLook &GetLook() {
    return wf.GetLook();
  }

  bool IsTaskModified() const {
    return *task_modified;
  }

  void GetCruiseEfficiency();

  void SetCruiseEfficiency(fixed value) {
    cruise_efficiency = value;
  }

  void Refresh();

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

#endif
