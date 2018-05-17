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

#ifndef XCSOAR_VEGA_PARAMETERS_WIDGET_HPP
#define XCSOAR_VEGA_PARAMETERS_WIDGET_HPP

#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Base.hpp"
#include "Util/StaticArray.hxx"

class VegaDevice;

class VegaParametersWidget : public RowFormWidget {
public:
  struct StaticParameter {
    DataField::Type type;

    const char *name;

    const TCHAR *label, *help;

    const StaticEnumChoice *choices;

    int min_value, max_value, step;

    const TCHAR *format;
  };

private:
  struct Parameter {
    const char *name;

    /**
     * This is the value that was filled into the form, and is later
     * used to check if the user has changed it.  Only modified values
     * will be sent to the Vega.
     */
    int value;

    Parameter() = default;
    Parameter(const char *_name):name(_name), value(0) {}
  };

  VegaDevice &device;

  const StaticParameter *static_parameters;

  /**
   * Have the Vega's parameter values been copied to the UI yet?
   */
  bool loaded;

  StaticArray<Parameter, 32> parameters;

public:
  VegaParametersWidget(const DialogLook &look, VegaDevice &_device,
                       const StaticParameter *_parameters=NULL)
    :RowFormWidget(look), device(_device), static_parameters(_parameters),
     loaded(false) {}

private:
  void AddParameter(const char *name) {
    parameters.push_back(Parameter(name));
  }

  void AddParameter(const StaticParameter &p);

public:
  /* methods to construct the form */
  void AddBoolean(const char *name, const TCHAR *label,
                  const TCHAR *help=NULL);
  void AddInteger(const char *name, const TCHAR *label, const TCHAR *help,
                  int min_value, int max_value, const TCHAR *format);
  void AddEnum(const char *name, const TCHAR *label, const TCHAR *help,
               const StaticEnumChoice *list);

private:
  /**
   * Request all parameter values from the Vega.
   */
  bool RequestAll();

  /**
   * Copy values from the Vega to the UI, excluding the ones that have
   * been modified by the user.
   */
  void UpdateUI();

public:
  /**
   * Discard all values modified by the user and reload all parameters
   * from the Vega.
   */
  void Revert();

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual void Show(const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;
};

#endif
