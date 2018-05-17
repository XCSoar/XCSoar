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

#include "IGC.hpp"
#include "Conversion.hpp"
#include "Checksum.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Util/Macros.hpp"

#include <cstdio>

namespace IMI
{
  void WriteString(const char *buffer, size_t max_length, FILE *file);
  void WriteSerialNumber(IMIWORD sn, FILE *file);
  const IMICHAR* GetDeviceName(unsigned i);
  const IMICHAR* GetGPSName(unsigned i);
  const IMICHAR* GetSensorName(unsigned i);
}

static unsigned siu = 0;
static IMI::FixB fixBLastFull = {0};
static IMI::FixB fixB1 = {0};
static IMI::FixB fixB2 = {0};

static constexpr IMI::IMICHAR snDigits[] = "0123456789ABCDEFHJKLMNPRSTUVWXYZ";

void
IMI::WriteString(const char *buffer, size_t max_length, FILE *file)
{
  size_t length = max_length;
  const char *zero = (const char *)memchr(buffer, '\0', max_length);
  if (zero != nullptr)
    length = zero - buffer;

  fwrite(buffer, sizeof(char), length, file);
}

void
IMI::WriteSerialNumber(IMIWORD sn, FILE *file)
{
  if (sn >= (32*32*32))
    sn = 0;

  IMIWORD x = sn >> 10; //(32*32 = 1024 = 2^10)
  putc(snDigits[x], file);
  sn -= (32 * 32) * x;

  x = sn >> 5; // 32 = 2^5
  putc(snDigits[x], file);
  sn -= 32 * x;

  putc(snDigits[sn], file);
}

static const IMI::IMICHAR *const UNKNOWN = "Unknown";
static const IMI::IMICHAR *const devices[] = {
  "Erixx",
  "SaiLog"
};

const IMI::IMICHAR*
IMI::GetDeviceName(unsigned i)
{
  return i >= ARRAY_SIZE(devices)
    ? UNKNOWN : devices[i];
}

static const IMI::IMICHAR *const gpsModules[] =
{
  "u-blox,LEA-4A,16,max15000m",
  "u-blox,LEA-4H,16,max15000m",
  "u-blox,LEA-5H,50,max15000m"
};

const IMI::IMICHAR*
IMI::GetGPSName(unsigned i)
{
  return i >= ARRAY_SIZE(gpsModules)
    ? UNKNOWN : gpsModules[i];
}

static const IMI::IMICHAR *const sensors[] =
{
  "Intersema,MS5534A,max10000m",
  "Intersema,MS5534B,max15000m",
  "No pressure sensor"
};

const IMI::IMICHAR*
IMI::GetSensorName(unsigned i)
{
  i &= (~IMINO_ENL_MASK);
  return i >= ARRAY_SIZE(sensors)
    ? UNKNOWN : sensors[i];
}

static unsigned
CountWaypoints(const IMI::TWaypoint *wps)
{
  unsigned count = 0;

  const IMI::TWaypoint *wp = wps + 1; // skip first (take off) WP
  for (unsigned i = 1; i < IMI::IMIDECL_MAX_WAYPOINTS; i++, wp++) {
    if (wp->name[0] == 0 && wp->lat == 0 && wp->lon == 0)
      break;

    count++;
  }

  if (count > 0)
    count--; // ignore last (landing) WP

  return count;
}

void
IMI::WriteHeader(const TDeclaration &decl, IMIBYTE tampered, FILE *file)
{
  fputs("AIMI", file);
  WriteSerialNumber(decl.header.sn, file);
  fputs("\r\n", file);

  BrokenDate start_date = ConvertToDateTime(decl.header.recStartDateTime);
  fprintf(file, "HFDTE%02d%02d%02d\r\n",
          start_date.day, start_date.month, start_date.year % 100);

  fputs("HFFXA010\r\n", file);

  fputs("HFPLTPILOT:", file);
  WriteString(decl.header.plt, sizeof(decl.header.plt), file);
  fputs("\r\n", file);

  if (decl.header.db1Day != 0)
    fprintf(file, "HFDB1PILOTBIRTHDATE:%02d%02d%02d\r\n",
            decl.header.db1Day + 1, decl.header.db1Month + 1,
            decl.header.db1Year % 100);

  fputs("HFGTYGLIDERTYPE:", file);
  WriteString(decl.header.gty, sizeof(decl.header.gty), file);
  fputs("\r\n", file);

  fputs("HFGIDGLIDERID:", file);
  WriteString(decl.header.gid, sizeof(decl.header.gid), file);
  fputs("\r\n", file);

  fputs("HFDTM100DATUM:WGS-1984\r\n", file);

  fprintf(file, "HFRFWFIRMWAREVERSION:%d.%d\r\n",
          (unsigned)(decl.header.swVersion >> 4),
          (unsigned)(decl.header.swVersion & 0x0F));

  fprintf(file, "HFRHWHARDWAREVERSION:%d.%d\r\n",
          (unsigned)(decl.header.hwVersion >> 4),
          (unsigned)(decl.header.hwVersion & 0x0F));

  fprintf(file, "HFFTYFRTYPE:IMI Gliding, %s\r\n",
          GetDeviceName(decl.header.device));

  fprintf(file, "HFGPSGPS:%s\r\n",
          GetGPSName(decl.header.gps));

  fprintf(file, "HFPRSPRESSALTSENSOR:%s\r\n",
          GetSensorName(decl.header.sensor));

  if (tampered)
    fputs("HFFRSSECURITYSUSPECTUSEVALIPROGRAM:Tamper detected, FR needs to be reset", file);

  if (decl.header.cid[0] != 0) {
    fputs("HFCIDCOMPETITIONID:", file);
    WriteString(decl.header.cid, sizeof(decl.header.cid), file);
    fputs("\r\n", file);
  }

  // This seems to be a bug in the original implementation
  // When the competition class line is missing the integrity check will fail!
  if (decl.header.cid[0] != 0) {
    fputs("HFCCLCOMPETITIONCLASS:", file);
    if (decl.header.ccl[0] != 0)
      WriteString(decl.header.ccl, sizeof(decl.header.ccl), file);
    fputs("\r\n", file);
  }

  if (decl.header.cm2[0] != '\0') {
    fputs("HFCM2SECONDCREW:", file);
    WriteString(decl.header.cm2, sizeof(decl.header.cm2), file);
    fputs("\r\n", file);

    if (decl.header.db2Day != 0)
      fprintf(file, "HFDB1SECONDCREWBIRTHDATE:%02d%02d%02d\r\n",
              decl.header.db2Day + 1, decl.header.db2Month + 1,
              decl.header.db2Year % 100);
  }

  if (decl.header.clb[0] != '\0') {
    fputs("HFCLBCLUB:", file);
    WriteString(decl.header.clb, sizeof(decl.header.clb), file);
    fputs("\r\n", file);
  }

  if (decl.header.sit[0] != '\0') {
    fputs("HFCLBSITE:", file);
    WriteString(decl.header.sit, sizeof(decl.header.sit), file);
    fputs("\r\n", file);
  }

  if ((decl.header.sensor & IMINO_ENL_MASK) != 0)
    fputs("I033638FXA3940SIU\r\n", file);
  else
    fputs("I033638FXA3940SIU4143ENL\r\n", file);

  fputs("J020810HDT1113GSP\r\n", file);

  unsigned count = CountWaypoints(decl.wp);
  if (count >= 2) {
    BrokenDateTime decl_date = ConvertToDateTime(decl.header.date);
    fprintf(file, "C%02d%02d%02d", decl_date.day, decl_date.month,
            decl_date.year % 100);
    fprintf(file, "%02d%02d%02d", decl_date.hour, decl_date.minute,
            decl_date.second);

    if (decl.header.tskYear != 0)
      fprintf(file, "%02d%02d%02d", decl.header.tskDay + 1,
              decl.header.tskMonth + 1, decl.header.tskYear % 100);
    else
      fputs("000000", file);

    fprintf(file, "%04d%02d", decl.header.tskNumber, count - 2);
    WriteString(decl.header.tskName, sizeof(decl.header.tskName), file);
    fputs("\r\n", file);

    const IMI::TWaypoint *wp = decl.wp;
    for (unsigned i = 0; i < count + 2; i++, wp++) {
      AngleConverter l;
      l.value = wp->lat;
      fprintf(file, "C%02d%05d%c", l.degrees, l.milliminutes,
              (l.sign ? 'S' : 'N'));

      l.value = wp->lon;
      fprintf(file, "%03d%05d%c", l.degrees, l.milliminutes,
              (l.sign ? 'W' : 'E'));

      WriteString(wp->name, sizeof(wp->name), file);
      fputs("\r\n", file);
    }
  }

  fputs("LIMIFLIGHTOFDAY:", file);
  fprintf(file, "%03d", decl.header.flightOfDay);
  fputs("\r\n", file);

  //first fix must be full
  fixBLastFull.id = 0;
}

static IMI::IMIDWORD
AngleAdd(IMI::IMIDWORD dwA, long diff)
{
  IMI::AngleConverter a;
  long mmA;

  a.value = dwA;

  mmA = (60 * 1000) * a.degrees + a.milliminutes;
  if (a.sign)
    mmA = -mmA;

  mmA += diff;

  if (mmA < 0) {
    a.sign = 1;
    mmA = -mmA;
  } else
    a.sign = 0;

  a.degrees = mmA / (60 * 1000); //TODO: avoid dividing?
  a.milliminutes = mmA - (60 * 1000) * a.degrees;

  return a.value;
}

static
bool
SplitB2Fix(const IMI::FixB2 *fixB2, const IMI::FixB *fixFull,
           IMI::FixB *fix1, IMI::FixB *fix2)
{
  if (fixB2->id != IMIFIX_ID_B2_RECORD || fixFull->id != IMIFIX_ID_B_RECORD)
    return false;

  fix1->id = IMIFIX_ID_B_RECORD;
  fix1->fv = fixFull->fv;
  fix1->time = fixFull->time + fixB2->time1 + 1;
  fix1->enl = fixB2->enl1;
  fix1->alt = (long)fixFull->alt + fixB2->alt1;
  fix1->gpsalt = (long)fixFull->gpsalt + fixB2->gpsalt1;
  fix1->lat = AngleAdd((IMI::IMIDWORD)fixFull->lat, (long)fixB2->lat1);
  fix1->lon = AngleAdd((IMI::IMIDWORD)fixFull->lon, (long)fixB2->lon1);
  fix1->fxa = (long)fixFull->fxa + (long)fixB2->fxa1;
  fix1->checksum = IMI::FixChecksum(fix1, __builtin_offsetof(IMI::Fix, checksum));

  fix2->id = IMIFIX_ID_B_RECORD;
  fix2->fv = fixFull->fv;
  fix2->time = fix1->time + fixB2->time2 + 1;
  fix2->enl = fixB2->enl2;
  fix2->alt = (long)fix1->alt + fixB2->alt2;
  fix2->gpsalt = (long)fix1->gpsalt + fixB2->gpsalt2;
  fix2->lat = AngleAdd((IMI::IMIDWORD)fix1->lat, (long)fixB2->lat2);
  fix2->lon = AngleAdd((IMI::IMIDWORD)fix1->lon, (long)fixB2->lon2);
  fix2->fxa = (long)fix1->fxa + (long)fixB2->fxa2;
  fix2->checksum = IMI::FixChecksum(fix2, __builtin_offsetof(IMI::Fix, checksum));

  return true;
}

void
IMI::WriteFix(const Fix &fix, bool fromB2, int no_enl, FILE *file)
{
  bool append_line_break = false;

  unsigned offset = __builtin_offsetof(Fix, checksum);
  if (fix.checksum != FixChecksum(&fix, offset)) {
    fixBLastFull.id = 0;
    return;
  }

  if (fix.id == IMIFIX_ID_B_RECORD) {
    const FixB *fix_b = (const FixB *)&fix;
    AngleConverter angle;

    fputc('B', file);
    BrokenTime time = ConvertToDateTime(fix.time);
    fprintf(file, "%02d%02d%02d", time.hour, time.minute, time.second);

    angle.value = (IMIDWORD)fix_b->lat;
    fprintf(file, "%02d%05d", angle.degrees, angle.milliminutes);

    fputc((angle.sign) ? 'S' : 'N', file);

    angle.value = (IMIDWORD)fix_b->lon;
    fprintf(file, "%03d%05d", angle.degrees, angle.milliminutes);

    fputc((angle.sign) ? 'W' : 'E', file);

    fputc("VA??"[fix_b->fv], file);

    int alt = (int)fix_b->alt - 1000;
    fprintf(file, alt < 0 ? "%04d" : "%05d", alt);
    int gpsalt = (int)fix_b->gpsalt - 1000;
    fprintf(file, alt < 0 ? "%04d" : "%05d", gpsalt);

    fprintf(file, "%03d", fix_b->fxa);
    fprintf(file, "%02d", siu);

    if (!no_enl)
      fprintf(file, "%03d", fix_b->enl);

    append_line_break = true;

    if (!fromB2)
      memcpy(&fixBLastFull, &fix, sizeof(fixBLastFull));
  } else if (fix.id == IMIFIX_ID_B2_RECORD) {
    const FixB2 *fix_b2 = (const FixB2 *)&fix;
    if (SplitB2Fix(fix_b2, &fixBLastFull, &fixB1, &fixB2)) {
      WriteFix(*(Fix*)&fixB1, true, no_enl, file);
      WriteFix(*(Fix*)&fixB2, true, no_enl, file);
    }
  } else if (fix.id == IMIFIX_ID_K_RECORD) {
    const FixK *fix_k = (const FixK *)&fix;

    fputc('K', file);
    BrokenTime time = ConvertToDateTime(fix.time);
    fprintf(file, "%02d%02d%02d", time.hour, time.minute, time.second);

    fprintf(file, "%03d", fix_k->hdt);
    fprintf(file, "%03d", fix_k->gsp);
    append_line_break = true;
  } else if (fix.id == IMIFIX_ID_E_RECORD) {
    const FixE *fix_e = (const FixE *)&fix;
    if (fix_e->type == IMIFIX_E_TYPE_SATELLITES) {
      siu = 0;
      fputc('F', file);
      BrokenTime time = ConvertToDateTime(fix.time);
      fprintf(file, "%02d%02d%02d", time.hour, time.minute, time.second);

      for (unsigned i = 0; i < sizeof(fix_e->text); i++) {
        if (fix_e->text[i] > 0) {
          fprintf(file, "%02d", fix_e->text[i]);
          siu++;
        }
      }
      append_line_break = true;
    } else if (fix_e->type == IMIFIX_E_TYPE_COMMENT) {
      fputs("LIMI", file);
      BrokenTime time = ConvertToDateTime(fix.time);
      fprintf(file, "%02d%02d%02d", time.hour, time.minute, time.second);
      WriteString((const char *)fix_e->text, sizeof(fix_e->text), file);

      append_line_break = true;
    } else if (fix_e->type == IMIFIX_E_TYPE_PEV) {
      fputc('E', file);
      BrokenTime time = ConvertToDateTime(fix.time);
      fprintf(file, "%02d%02d%02d", time.hour, time.minute, time.second);
      WriteString((const char *)fix_e->text, sizeof(fix_e->text), file);

      append_line_break = true;
    } else if (fix_e->type == IMIFIX_E_TYPE_TASK) {
      fputc('E', file);
      BrokenTime time = ConvertToDateTime(fix.time);
      fprintf(file, "%02d%02d%02d", time.hour, time.minute, time.second);

      if (fix_e->text[0] == 1) {
        fputs("STA", file);
        WriteString((const char *)fix_e->text + 2, sizeof(fix_e->text) - 2, file);
      } else {
        if (fix_e->text[0] == 2)
          fputs("ONT", file);
        else if (fix_e->text[0] == 3)
          fputs("FIN", file);
        else
          fputs("TPC", file);

        fprintf(file, "%02d", fix_e->text[1]);
        WriteString((const char *)fix_e->text + 2, sizeof(fix_e->text) - 2, file);
      }

      append_line_break = true;
    }
  }

  if (append_line_break)
    fputs("\r\n", file);
}

void
IMI::WriteSignature(const Signature &sig, IMIWORD sn, FILE *file)
{
  const IMICHAR _hexChars[] = "0123456789ABCDEF";

  if (sn == 0)
    fputs("GINVALID:Invalid serial number\r\n", file);
  else if (sig.rsaBits != 512 && sig.rsaBits != 768 && sig.rsaBits != 1024)
    fputs("GINVALID:Invalid RSA key size\r\n", file);
  else if (sig.tampered)
    fputs("GINVALID:Tamper detected\r\n", file);
  else {
    fputc('G', file);

    const IMICHAR *signature = (const IMICHAR*)sig.signature;
    for (unsigned i = 0; i < (sig.rsaBits / 8u);) {
      IMICHAR ch = signature[i++];
      fputc(_hexChars[(ch & 0xF0) >> 4], file);
      fputc(_hexChars[(ch & 0x0F)], file);

      if ((i & (32 - 1)) == 0) {
        fputs("\r\n", file);

        if (i + 1 < (sig.rsaBits / 8u))
          fputc('G', file);
      }
    }
  }
}
