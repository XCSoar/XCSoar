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

#include "Convert.hpp"
#include "LXN.hpp"
#include "OS/ByteOrder.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Context {
  uint8_t flight_no;
  char date[7];
  LXN::FlightInfo flight_info;
  unsigned time, origin_time;
  int origin_latitude, origin_longitude;
  bool is_event;
  LXN::Event event;
  char fix_stat;
  char vendor[3];
  LXN::ExtensionConfig k_ext, b_ext;

  Context()
    :flight_no(0),
     time(0), origin_time(0),
     origin_latitude(0), origin_longitude(0),
     is_event(false) {
    memset(date, 0, sizeof(date));
    flight_info.competition_class_id = 0xff;
    memset(vendor, 0, sizeof(vendor));
    k_ext.num = 0;
    b_ext.num = 0;
  }
};

static bool
ValidString(const char *p, size_t size)
{
  return memchr(p, 0, size) != nullptr;
}

static void
HandlePosition(FILE *file, Context &context,
               const struct LXN::Position &position)
{
    int latitude, longitude;

    context.fix_stat = position.cmd == LXN::POSITION_OK ? 'A' : 'V';
    context.time = context.origin_time + (int16_t)FromBE16(position.time);
    latitude = context.origin_latitude + (int16_t)FromBE16(position.latitude);
    longitude = context.origin_longitude + (int16_t)FromBE16(position.longitude);

    if (context.is_event) {
        fprintf(file,"E%02d%02d%02d%s\r\n",
                context.time / 3600, context.time % 3600 / 60, context.time % 60,
                context.event.foo);
        context.is_event = 0;
    }

    fprintf(file, "B%02d%02d%02d",
            context.time / 3600, context.time % 3600 / 60, context.time % 60);
    fprintf(file, "%02d%05d%c" "%03d%05d%c",
            abs(latitude) / 60000, abs(latitude) % 60000,
            latitude >= 0 ? 'N' : 'S',
            abs(longitude) / 60000, abs(longitude) % 60000,
            longitude >= 0 ? 'E' : 'W');
    fprintf(file, "%c", context.fix_stat);
    fprintf(file, "%05d%05d",
            /* altitudes can be negative, so cast the uint16_t to
               int16_t to interpret the most significant bit as sign
               bit */
            (int16_t)FromBE16(position.aalt),
            (int16_t)FromBE16(position.galt));

    if (context.b_ext.num == 0)
        fprintf(file, "\r\n");
}

static void
HandleExtConfig(FILE *file, const struct LXN::ExtConfig &packet,
                LXN::ExtensionConfig &config,
                char record, unsigned column)
{
  /* count bits in extension mask and write information about each
     extension*/
  const unsigned ext_dat = FromBE16(packet.dat);
  config.num = 0;
  for (unsigned bit = 0; bit < 16; ++bit) {
    if (ext_dat & (1 << bit)) {
      config.extensions[config.num++] = LXN::extension_defs[bit];
    }
  }

  if (config.num == 0)
    return;

  /* begin record */
  fprintf(file, "%c%02d", record, config.num);

  /* write information about each extension */
  for (unsigned i = 0; i < config.num; ++i) {
    fprintf(file, "%02d%02d%s", column,
            column + config.extensions[i].width - 1,
            config.extensions[i].name);
    column += config.extensions[i].width;
  }

  fprintf(file, "\r\n");
}

bool
LX::ConvertLXNToIGC(const void *_data, size_t _length,
                    FILE *file)
{
  const uint8_t *data = (const uint8_t *)_data, *end = data + _length;
  Context context;
  char ch;
  unsigned l;

  while (data < end) {
    union LXN::Packet packet = { data };

    unsigned packet_length;
    switch ((LXN::Command)*packet.cmd) {
    case LXN::EMPTY:
      packet_length = 0;
      while (data < end && *data == LXN::EMPTY) {
        ++packet_length;
        ++data;
      }

      fprintf(file, "LFILEMPTY%u\r\n", packet_length);
      break;

    case LXN::END:
      return true;

    case LXN::VERSION:
      data += sizeof(*packet.version);
      if (data > end)
        return false;

      fprintf(file,
              "HFRFWFIRMWAREVERSION:%3.1f\r\n"
              "HFRHWHARDWAREVERSION:%3.1f\r\n",
              packet.version->software / 10.,
              packet.version->hardware / 10.);
      break;

    case LXN::START:
      data += sizeof(*packet.start);
      if (data > end ||
          memcmp(packet.start->streraz, "STReRAZ", 8) != 0)
        return false;

      context.flight_no = packet.start->flight_no;
      break;

    case LXN::ORIGIN:
      data += sizeof(*packet.origin);
      if (data > end)
        return false;

      context.origin_time = FromBE32(packet.origin->time);
      context.origin_latitude = (int32_t)FromBE32(packet.origin->latitude);
      context.origin_longitude = (int32_t)FromBE32(packet.origin->longitude);

      fprintf(file, "L%.*sORIGIN%02d%02d%02d" "%02d%05d%c" "%03d%05d%c\r\n",
              (int)sizeof(context.vendor), context.vendor,
              context.origin_time / 3600, context.origin_time % 3600 / 60,
              context.origin_time % 60,
              abs(context.origin_latitude) / 60000,
              abs(context.origin_latitude) % 60000,
              context.origin_latitude >= 0 ? 'N' : 'S',
              abs(context.origin_longitude) / 60000,
              abs(context.origin_longitude) % 60000,
              context.origin_longitude >= 0 ? 'E' : 'W');
      break;

    case LXN::SECURITY_OLD:
      data += sizeof(*packet.security_old);
      if (data > end)
        return false;

      fprintf(file, "G%22.22s\r\n", packet.security_old->foo);
      break;

    case LXN::SERIAL:
      data += sizeof(*packet.serial);
      if (data > end)
        return false;

      if (!ValidString(packet.serial->serial, sizeof(packet.serial->serial)))
        return false;

      fprintf(file, "A%sFLIGHT:%u\r\nHFDTE%s\r\n",
              packet.serial->serial, context.flight_no, context.date);
      break;

    case LXN::POSITION_OK:
    case LXN::POSITION_BAD:
      data += sizeof(*packet.position);
      if (data > end)
        return false;

      HandlePosition(file, context, *packet.position);
      break;

    case LXN::SECURITY:
      data += sizeof(*packet.security);
      if (data > end ||
          packet.security->length > sizeof(packet.security->foo))
        return false;

      if (packet.security->type == LXN::SECURITY_HIGH)
        ch = '2';
      else if (packet.security->type == LXN::SECURITY_MED)
        ch = '1';
      else if (packet.security->type == LXN::SECURITY_LOW)
        ch = '0';
      else
        return false;

      fprintf(file, "G%c", ch);

      for (unsigned i = 0; i < packet.security->length; ++i)
        fprintf(file, "%02X", packet.security->foo[i]);

      fprintf(file, "\r\n");
      break;

    case LXN::SECURITY_7000:
      data += sizeof(*packet.security_7000);
      if (data > end ||
          packet.security_7000->x40 != 0x40)
        return false;

      fprintf(file, "G3");
      for (auto ch : packet.security_7000->line1)
        fprintf(file, "%02X", ch);

      fprintf(file, "\r\nG");
      for (auto ch : packet.security_7000->line2)
        fprintf(file, "%02X", ch);

      fprintf(file, "\r\nG");
      for (auto ch : packet.security_7000->line3)
        fprintf(file, "%02X", ch);

      fprintf(file, "\r\n");
      break;

    case LXN::COMPETITION_CLASS:
      data += sizeof(*packet.competition_class);
      if (data > end)
        return false;

      if (!ValidString(packet.competition_class->class_id,
                       sizeof(packet.competition_class->class_id)))
        return false;

      if (context.flight_info.competition_class_id == 7)
        fprintf(file,
                "HFFXA%03d\r\n"
                "HFPLTPILOT:%s\r\n"
                "HFGTYGLIDERTYPE:%s\r\n"
                "HFGIDGLIDERID:%s\r\n"
                "HFDTM%03dGPSDATUM:%s\r\n"
                "HFCIDCOMPETITIONID:%s\r\n"
                "HFCCLCOMPETITIONCLASS:%s\r\n"
                "HFGPSGPS:%s\r\n",
                context.flight_info.fix_accuracy,
                context.flight_info.pilot,
                context.flight_info.glider,
                context.flight_info.registration,
                context.flight_info.gps_date,
                LXN::FormatGPSDate(context.flight_info.gps_date),
                context.flight_info.competition_class,
                packet.competition_class->class_id,
                context.flight_info.gps);
      break;

    case LXN::TASK:
      data += sizeof(*packet.task);
      if (data > end)
        return false;

      context.time = FromBE32(packet.task->time);

      // from a valid IGC file read with LXe:
      // C 11 08 11 14 11 18 11 08 11 0001 -2

      fprintf(file, "C%02d%02d%02d%02d%02d%02d"
              "%02d%02d%02d" "%04d%02d\r\n",
              packet.task->day, packet.task->month, packet.task->year,
              context.time / 3600, context.time % 3600 / 60, context.time % 60,
              packet.task->day2, packet.task->month2, packet.task->year2,
              FromBE16(packet.task->task_id), packet.task->num_tps);

      for (unsigned i = 0; i < sizeof(packet.task->usage); ++i) {
        if (packet.task->usage[i]) {
          int latitude = (int32_t)FromBE32(packet.task->latitude[i]);
          int longitude = (int32_t)FromBE32(packet.task->longitude[i]);

          if (!ValidString(packet.task->name[i], sizeof(packet.task->name[i])))
            return false;

          fprintf(file, "C%02d%05d%c" "%03d%05d%c" "%s\r\n",
                  abs(latitude) / 60000, abs(latitude) % 60000,
                  latitude >= 0 ?  'N' : 'S',
                  abs(longitude) / 60000, abs(longitude) % 60000,
                  longitude >= 0 ? 'E' : 'W',
                  packet.task->name[i]);
        }
      }
      break;

    case LXN::EVENT:
      data += sizeof(*packet.event);
      if (data > end ||
          !ValidString(packet.event->foo, sizeof(packet.event->foo)))
        return false;

      context.event = *packet.event;
      context.is_event = true;
      break;

    case LXN::B_EXT:
      data += sizeof(*packet.b_ext) +
        context.b_ext.num * sizeof(packet.b_ext->data[0]);
      if (data > end)
        return false;

      for (unsigned i = 0; i < context.b_ext.num; ++i)
        fprintf(file, "%0*u",
                (int)context.b_ext.extensions[i].width,
                FromBE16(packet.b_ext->data[i]));

      fprintf(file, "\r\n");
      break;

    case LXN::K_EXT:
      data += sizeof(*packet.k_ext) +
        context.k_ext.num * sizeof(packet.k_ext->data[0]);
      if (data > end)
        return false;

      l = context.time + packet.k_ext->foo;
      fprintf(file, "K%02d%02d%02d",
              l / 3600, l % 3600 / 60, l % 60);

      for (unsigned i = 0; i < context.k_ext.num; ++i)
        fprintf(file, "%0*u",
                context.k_ext.extensions[i].width,
                FromBE16(packet.k_ext->data[i]));

      fprintf(file, "\r\n");
      break;

    case LXN::DATE:
      data += sizeof(*packet.date);
      if (data > end ||
          packet.date->day > 31 || packet.date->month > 12)
        return false;

      snprintf(context.date, sizeof(context.date),
               "%02d%02d%02d",
               packet.date->day % 100, packet.date->month % 100,
               FromBE16(packet.date->year));
      break;

    case LXN::FLIGHT_INFO:
      data += sizeof(*packet.flight_info);
      if (data > end ||
          !ValidString(packet.flight_info->pilot,
                       sizeof(packet.flight_info->pilot)) ||
          !ValidString(packet.flight_info->glider,
                       sizeof(packet.flight_info->glider)) ||
          !ValidString(packet.flight_info->registration,
                       sizeof(packet.flight_info->registration)) ||
          !ValidString(packet.flight_info->competition_class,
                       sizeof(packet.flight_info->competition_class)) ||
          !ValidString(packet.flight_info->gps, sizeof(packet.flight_info->gps)))
        return false;

      if (packet.flight_info->competition_class_id > 7)
        return false;

      if (packet.flight_info->competition_class_id < 7)
        fprintf(file,
                "HFFXA%03d\r\n"
                "HFPLTPILOT:%s\r\n"
                "HFGTYGLIDERTYPE:%s\r\n"
                "HFGIDGLIDERID:%s\r\n"
                "HFDTM%03dGPSDATUM:%s\r\n"
                "HFCIDCOMPETITIONID:%s\r\n"
                "HFCCLCOMPETITIONCLASS:%s\r\n"
                "HFGPSGPS:%s\r\n",
                packet.flight_info->fix_accuracy,
                packet.flight_info->pilot,
                packet.flight_info->glider,
                packet.flight_info->registration,
                packet.flight_info->gps_date,
                LXN::FormatGPSDate(packet.flight_info->gps_date),
                packet.flight_info->competition_class,
                LXN::FormatCompetitionClass(packet.flight_info->competition_class_id),
                packet.flight_info->gps);

      context.flight_info = *packet.flight_info;
      break;

    case LXN::K_EXT_CONFIG:
      data += sizeof(*packet.ext_config);
      if (data > end)
        return false;

      HandleExtConfig(file, *packet.ext_config, context.k_ext, 'J', 8);
      break;

    case LXN::B_EXT_CONFIG:
      data += sizeof(*packet.ext_config);
      if (data > end)
        return false;

      HandleExtConfig(file, *packet.ext_config, context.b_ext, 'I', 36);
      break;

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
    default:
      if (*packet.cmd < 0x40) {
        data += sizeof(*packet.string);
        if (data > end)
          return false;

        data += packet.string->length;
        if (data > end)
          return false;

        fprintf(file, "%.*s\r\n",
                (int)packet.string->length, packet.string->value);

        if (packet.string->length >= 12 + sizeof(context.vendor) &&
            memcmp(packet.string->value, "HFFTYFRTYPE:", 12) == 0)
          memcpy(context.vendor, packet.string->value + 12, sizeof(context.vendor));
      } else
        return false;
    }
  }

  return false;
}
