/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "Form/DataField/NumPadAdapter.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/CharUtil.hxx"
#include "Screen/Layout.hpp"
#include "Renderer/TextButtonRenderer.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "ui/event/KeyCode.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"


#include <time.h>
#include <sys/time.h>
#include <cassert>
#include <string.h>




void NumPadAdapter::BeginEditing() noexcept{
  if(nullptr != numPad )
    numPad->BeginEditing();
}
void NumPadAdapter::EndEditing() noexcept{
  if(nullptr != numPad )
    numPad->EndEditing();

}


