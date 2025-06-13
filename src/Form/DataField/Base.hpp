// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cassert>
#include <cstdint>
#include <functional>

#include <tchar.h>

static constexpr unsigned OUTBUFFERSIZE = 128;

class DataFieldListener;
class ComboList;

/**
 * Most implementations have a method GetValue() for obtaining the raw
 * value with its native type; ModifyValue() sets a new value and
 * invokes DataFieldListener::OnModified(); SetValue() does the same,
 * but does not invoke the callback.
 */
class DataField
{
public:
  enum class Type : uint8_t {
    STRING,
    BOOLEAN,
    INTEGER,
    REAL,
    ANGLE,
    ENUM,
    FILE,
    MULTI_FILE,
    ROUGH_TIME,
    TIME,
    PREFIX,
    GEOPOINT,
    DATE,
  };

  using ModifiedCallback = std::function<void()>;

private:
  ModifiedCallback on_modified;

  DataFieldListener *listener; // deprecated

  // all Types dataField support combolist except DataFieldString.
  const bool supports_combolist;

protected:
  const Type type;

  bool item_help_enabled;

protected:
  DataField(Type type, bool supports_combolist,
            DataFieldListener *listener) noexcept;

public:
  virtual ~DataField() noexcept = default;

  void SetOnModified(ModifiedCallback &&_callback) noexcept {
    on_modified = std::move(_callback);
  }

  // deprecated
  void SetListener(DataFieldListener *_listener) noexcept {
    assert(listener == nullptr);
    assert(_listener != nullptr);

    listener = _listener;
  }

  Type GetType() const noexcept {
    return type;
  }

  bool SupportsCombolist() const noexcept {
    return supports_combolist;
  }

  virtual void Inc() noexcept;
  virtual void Dec() noexcept;

  [[gnu::pure]]
  virtual const TCHAR *GetAsString() const noexcept;

  [[gnu::pure]]
  virtual const TCHAR *GetAsDisplayString() const noexcept;

  virtual void EnableItemHelp([[maybe_unused]] bool value) noexcept {};

  /**
   * Create a #ComboList that allows the user to choose a value.
   *
   * @param reference the reference value for a long list, to
   * determine the range of displayed values; pass nullptr for the
   * "default" reference
   */
  [[gnu::pure]]
  virtual ComboList CreateComboList(const TCHAR *reference) const noexcept;

  virtual void SetFromCombo([[maybe_unused]] int iDataFieldIndex,
                            [[maybe_unused]] const TCHAR *sValue) noexcept
  {
    /* this method must be implemented by all classes which also
       implement CreateComboList() */
  }

  bool GetItemHelpEnabled() const noexcept {
    return item_help_enabled;
  }

protected:
  /**
   * Notify interested parties that the value of this object has
   * been modified.
   */
  void Modified() noexcept;
};
