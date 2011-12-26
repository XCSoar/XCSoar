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

#ifndef XCSOAR_ROW_FORM_WIDGET_HPP
#define XCSOAR_ROW_FORM_WIDGET_HPP

#include "WindowWidget.hpp"
#include "Edit.hpp"
#include "DataField/Base.hpp"
#include "Util/StaticArray.hpp"
#include "Units/Settings.hpp"

#include <assert.h>

struct DialogLook;
struct StaticEnumChoice;

/**
 * A #Widget that contains #WndProperty controls, one in a row.
 * Derive from this class, and construct the controls in Initialise()
 * or Prepare().  Don't forget to call the base class in these two
 * methods!
 */
class RowFormWidget : public WindowWidget {
  const DialogLook &look;
  UPixelScalar caption_width;

  StaticArray<WndProperty *, 32u> controls;

public:
  RowFormWidget(const DialogLook &look, UPixelScalar caption_width);
  virtual ~RowFormWidget();

  void Add(WndProperty *edit);

  WndProperty *Add(const TCHAR *label, const TCHAR *help=NULL,
                   bool read_only=false);

  WndProperty *Add(const TCHAR *label, const TCHAR *help,
                   DataField *df);

  WndProperty *AddBoolean(const TCHAR *label, const TCHAR *help,
                          bool value=false,
                          DataField::DataAccessCallback_t callback=NULL);

  WndProperty *AddInteger(const TCHAR *label, const TCHAR *help,
                          const TCHAR *display_format,
                          const TCHAR *edit_format,
                          int min_value, int max_value, int step, int value,
                          DataField::DataAccessCallback_t callback=NULL);

  WndProperty *AddFloat(const TCHAR *label, const TCHAR *help,
                        const TCHAR *display_format,
                        const TCHAR *edit_format,
                        fixed min_value, fixed max_value,
                        fixed step, bool fine,
                        fixed value,
                        DataField::DataAccessCallback_t callback=NULL);

  WndProperty *AddFloat(const TCHAR *label, const TCHAR *help,
                        const TCHAR *display_format,
                        const TCHAR *edit_format,
                        fixed min_value, fixed max_value,
                        fixed step, bool fine,
                        UnitGroup unit_group, fixed value,
                        DataField::DataAccessCallback_t callback=NULL);

  WndProperty *AddEnum(const TCHAR *label, const TCHAR *help,
                       const StaticEnumChoice *list, unsigned value=0,
                       DataField::DataAccessCallback_t callback=NULL);

  gcc_pure
  WndProperty &GetControl(unsigned i) {
    assert(i < (unsigned)controls.size());
    assert(controls[i] != NULL);

    return *controls[i];
  }

  gcc_pure
  const WndProperty &GetControl(unsigned i) const {
    assert(i < (unsigned)controls.size());
    assert(controls[i] != NULL);

    return *controls[i];
  }

  gcc_pure
  DataField &GetDataField(unsigned i) {
    DataField *df = GetControl(i).GetDataField();
    assert(df != NULL);
    return *df;
  }

  gcc_pure
  const DataField &GetDataField(unsigned i) const {
    const DataField *df = GetControl(i).GetDataField();
    assert(df != NULL);
    return *df;
  }

  gcc_pure
  bool GetValueBoolean(unsigned i) const;

  gcc_pure
  int GetValueInteger(unsigned i) const;

  gcc_pure
  fixed GetValueFloat(unsigned i) const;

  bool SaveValue(unsigned i, bool &value) const;
  bool SaveValue(unsigned i, int &value) const;
  bool SaveValue(unsigned i, fixed &value) const;
  bool SaveValue(unsigned i, TCHAR *string, size_t max_size) const;

  bool SaveValue(unsigned i, unsigned &value) const {
    return SaveValue(i, (int &)value);
  }

  bool SaveValue(unsigned i, const TCHAR *registry_key, bool &value) const;
  bool SaveValue(unsigned i, const TCHAR *registry_key, int &value) const;
  bool SaveValue(unsigned i, const TCHAR *registry_key, fixed &value) const;

  bool SaveValue(unsigned i, const TCHAR *registry_key,
                 unsigned &value) const {
    return SaveValue(i, registry_key, (int &)value);
  }

  bool SaveValue(unsigned i, UnitGroup unit_group,
                 const TCHAR *registry_key, fixed &value) const;

  bool SaveValue(unsigned i, UnitGroup unit_group,
                 const TCHAR *registry_key, unsigned int &value) const;

  template<typename T>
  bool SaveValueEnum(unsigned i, T &value) const {
    if (sizeof(T) == sizeof(int))
      return SaveValue(i, (int &)value);

    int value2 = (int)value;
    if (!SaveValue(i, value2))
      return false;

    value = (T)value2;
    return true;
  }

  template<typename T>
  bool SaveValueEnum(unsigned i, const TCHAR *registry_key, T &value) const {

    int value2 = (int)value;
    if (!SaveValue(i, registry_key, value2))
      return false;

    value = (T)value2;
    return true;
  }

protected:
  void NextControlRect(PixelRect &rc, UPixelScalar height) {
    assert(IsDefined());

    rc.top = rc.bottom;
    rc.bottom = rc.top + height;
  }

  gcc_pure
  PixelRect NextControlRect(const PixelRect &rc, UPixelScalar height) {
    assert(IsDefined());

    PixelRect next = rc;
    if (!controls.empty())
      next.top = controls.back()->get_position().bottom;
    next.bottom = next.top + height;
    return next;
  }

  virtual void Initialise(ContainerWindow &parent, const PixelRect &rc);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
  virtual void Move(const PixelRect &rc);
  virtual bool SetFocus();
};

#endif
