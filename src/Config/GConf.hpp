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

#ifndef XCSOAR_CONFIG_GCONF_HPP
#define XCSOAR_CONFIG_GCONF_HPP

#include <gconf/gconf.h>

class GConf {
protected:
  GConfEngine *engine;

public:
  GConf():engine(gconf_engine_get_default()) {}
  ~GConf() {
    gconf_engine_unref(engine);
  }

  bool get(const char *key, int &value) {
    GConfValue *cv = gconf_engine_get_without_default(engine, key, NULL);
    if (cv == NULL)
      return false;

    if (cv->type != GCONF_VALUE_INT) {
      gconf_value_free(cv);
      return false;
    }

    value = gconf_value_get_int(cv);
    gconf_value_free(cv);
    return true;
  }

  bool get(const char *key, char *value, size_t max_length) {
    gchar *buffer = gconf_engine_get_string(engine, key, NULL);
    if (buffer == NULL)
      return false;

    g_strlcpy(value, buffer, max_length);
    g_free(buffer);
    return true;
  }

  bool set(const char *key, int value) {
    return gconf_engine_set_int(engine, key, value, NULL);
  }

  bool set(const char *key, const char *value) {
    return gconf_engine_set_string(engine, key, value, NULL);
  }
};

#endif

