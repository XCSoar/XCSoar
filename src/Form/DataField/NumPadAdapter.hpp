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

#ifndef SRC_FORM_DATAFIELD_NUMPADADAPTER_HPP_
#define SRC_FORM_DATAFIELD_NUMPADADAPTER_HPP_

#include "Form/DataField/NumPadWidgetInterface.hpp"
#include "ui/control/List.hpp"
#include <memory>
#include <utility>
#include <functional>

class NumPadWidget;
class DataField;
class ComboList;

class NumPadAdapter {
  protected:
  std::function< void()> refreshEditFieldFunction;
  std::function< void()> setFocusEditFieldFunction;
  NumPadWidgetInterface * numPad;
  DataField *dataField;
  const TCHAR *Caption;
  void returnFocus()
  {
 //  	dataField.SetFocus();
  }
  NumPadAdapter(NumPadWidgetInterface * _numPad): numPad(_numPad), dataField(nullptr), Caption(nullptr) { };
  public:
  virtual void UpdateButtons() noexcept =0;
  virtual bool CharacterFunction(unsigned ch){
    return false;
  };
  virtual bool
	OnKeyDown(unsigned key_code) noexcept{
		return false;
	}
  virtual void BeginEditing() noexcept;
  virtual bool OnKeyCheck(unsigned key_code) const noexcept
  {
    return false;
  }
  const TCHAR *GetCaption() noexcept
  {
    return Caption;
  }
  void SetCaption(const TCHAR * _Caption) noexcept
  {
     Caption = _Caption;
  }
  virtual void EndEditing() noexcept;
  void SetNumPadWidgetInterface(NumPadWidgetInterface * _numPad)
  {
    numPad = _numPad;
    if(numPad != nullptr)
      numPad->SetNumPadAdapter( this );
  }
  NumPadWidgetInterface *GetNumPadWidgetInterface()
  {
    return numPad;
  }
  void SetDataField(DataField *df)
  {
    dataField = df;
  }
  virtual void OnButton(unsigned buttonIndex ) noexcept= 0;
  virtual void SetComboList(ComboList * _list) noexcept {};
  virtual void OnDataFieldSetFocus() noexcept{
    if( numPad != nullptr)
    {
      numPad->SetNumPadAdapter( this );
      numPad->GetNumPadAdapter().SetCaption(GetCaption());
      numPad->OnDataFieldSetFocus();
    }
    // directs keyboard input to the matching datafield
  };
  virtual ComboList *GetComboList() noexcept{ return nullptr; };
  virtual void OnCursorMoved([[maybe_unused]] unsigned index) noexcept {
    if( numPad != nullptr)
      numPad->GetNumPadAdapter().OnCursorMoved(index);
  };
  void SetRefreshEditFieldFunction(std::function< void()> _refreshFunction){
    refreshEditFieldFunction = _refreshFunction;
  }
  // Even if the ListBox Control has no Tab, it aquires focus, when clicked.
  // Use this method to bring focus back to edit field
  void SetSetFocusEditFieldFunction(std::function< void()> _setFocusEditFieldFunction){
    setFocusEditFieldFunction = _setFocusEditFieldFunction;
  }
  virtual void OnModified() noexcept {};
  virtual ~NumPadAdapter() noexcept{};
  NumPadAdapter(NumPadAdapter &&rhs){};

};


#endif /* SRC_FORM_DATAFIELD_NUMPADADAPTER_HPP_ */
