// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Base.hpp"
#include "util/StaticArray.hxx"

class VegaDevice;

class VegaParametersWidget : public RowFormWidget {
public:
  struct StaticParameter {
    DataField::Type type;

    const char *name;

    const char *label, *help;

    const StaticEnumChoice *choices;

    int min_value, max_value, step;

    const char *format;
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
  void AddBoolean(const char *name, const char *label,
                  const char *help=NULL);
  void AddInteger(const char *name, const char *label, const char *help,
                  int min_value, int max_value, const char *format);
  void AddEnum(const char *name, const char *label, const char *help,
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
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
