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
#include "Widget/NumPadWidget.hpp"


 void NumPadWidgetInterface::SetNumPadAdapter( NumPadAdapter *_numPad) noexcept{
     numPadAdapter = _numPad;
  };
 void NumPadWidgetInterface::OnDataFieldSetFocus()
 {
   numPadWidget->OnDataFieldSetFocus();
 }
 void NumPadWidgetInterface::BeginEditing() noexcept
 {
   assert( numPadWidget != nullptr);
     numPadWidget->BeginEditing();
 }
 void NumPadWidgetInterface::EndEditing() noexcept
 {
   assert( numPadWidget != nullptr);
     numPadWidget->EndEditing();
 }
 unsigned NumPadWidgetInterface::GetNumButtons() noexcept{
   assert( numPadWidget != nullptr);
   return numPadWidget->GetNumButtons();
 };
 Button *NumPadWidgetInterface::NumPadWidgetInterface::GetButtons() noexcept{
   assert( numPadWidget != nullptr);
   return numPadWidget->GetButtons();
 };
 bool NumPadWidgetInterface::HasFocus() noexcept{
   assert( numPadWidget != nullptr);
   return numPadWidget->HasFocus();
  }



 bool NumPadWidgetInterface::CharacterFunction( unsigned ch) noexcept
 {
   assert( numPadAdapter != nullptr);
   return numPadAdapter->CharacterFunction(ch);
 }

 void NumPadWidgetInterface::OnButton( unsigned buttonIndex)
 {
   assert( numPadAdapter != nullptr);
   numPadAdapter->OnButton(buttonIndex);
 }
 void NumPadWidgetInterface::SetCursorIndex(unsigned index) noexcept{
   numPadWidget->SetCursorIndex(index);
 }

