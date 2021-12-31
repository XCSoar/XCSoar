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

#include "IGC.hpp"
#include "Conversion.hpp"
#include "Checksum.hpp"
#include "io/BufferedOutputStream.hxx"
#include "time/BrokenDateTime.hpp"
#include "util/Macros.hpp"

namespace IMI
{
  void WriteString(BufferedOutputStream &os, const char *buffer, size_t max_length);
  void WriteSerialNumber(BufferedOutputStream &os, IMIWORD sn);
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
IMI::WriteString(BufferedOutputStream &os, const char *buffer, size_t max_length)
{
  size_t length = max_length;
  const char *zero = (const char *)memchr(buffer, '\0', max_length);
  if (zero != nullptr)
    length = zero - buffer;

  os.Write(buffer, length);
}

void
IMI::WriteSerialNumber(BufferedOutputStream &os, IMIWORD sn)
{
  if (sn >= (32*32*32))
    sn = 0;

  IMIWORD x = sn >> 10; //(32*32 = 1024 = 2^10)
  os.Write(snDigits[x]);
  sn -= (32 * 32) * x;

  x = sn >> 5; // 32 = 2^5
  os.Write(snDigits[x]);
  sn -= 32 * x;

  os.Write(snDigits[sn]);
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
IMI::WriteHeader(BufferedOutputStream &os, const TDeclaration &decl, IMIBYTE tampered)
{
  os.Write("AIMI");
  WriteSerialNumber(os, decl.header.sn);
  os.Write("\r\n");

  BrokenDate start_date = ConvertToDateTime(decl.header.recStartDateTime);
  os.Format("HFDTE%02d%02d%02d\r\n",
            start_date.day, start_date.month, start_date.year % 100);

  os.Write("HFFXA010\r\n");

  os.Write("HFPLTPILOT:");
  WriteString(os, decl.header.plt, sizeof(decl.header.plt));
  os.Write("\r\n");

  if (decl.header.db1Day != 0)
    os.Format("HFDB1PILOTBIRTHDATE:%02d%02d%02d\r\n",
              decl.header.db1Day + 1, decl.header.db1Month + 1,
              decl.header.db1Year % 100);

  os.Write("HFGTYGLIDERTYPE:");
  WriteString(os, decl.header.gty, sizeof(decl.header.gty));
  os.Write("\r\n");

  os.Write("HFGIDGLIDERID:");
  WriteString(os, decl.header.gid, sizeof(decl.header.gid));
  os.Write("\r\n");

  os.Write("HFDTM100DATUM:WGS-1984\r\n");

  os.Format("HFRFWFIRMWAREVERSION:%d.%d\r\n",
            (unsigned)(decl.header.swVersion >> 4),
            (unsigned)(decl.header.swVersion & 0x0F));

  os.Format("HFRHWHARDWAREVERSION:%d.%d\r\n",
            (unsigned)(decl.header.hwVersion >> 4),
            (unsigned)(decl.header.hwVersion & 0x0F));

  os.Format("HFFTYFRTYPE:IMI Gliding, %s\r\n",
            GetDeviceName(decl.header.device));

  os.Format("HFGPSGPS:%s\r\n",
            GetGPSName(decl.header.gps));

  os.Format("HFPRSPRESSALTSENSOR:%s\r\n",
            GetSensorName(decl.header.sensor));

  if (tampered)
    os.Write("HFFRSSECURITYSUSPECTUSEVALIPROGRAM:Tamper detected, FR needs to be reset");

  if (decl.header.cid[0] != 0) {
    os.Write("HFCIDCOMPETITIONID:");
    WriteString(os, decl.header.cid, sizeof(decl.header.cid));
    os.Write("\r\n");
  }

  // This seems to be a bug in the original implementation
  // When the competition class line is missing the integrity check will fail!
  if (decl.header.cid[0] != 0) {
    os.Write("HFCCLCOMPETITIONCLASS:");
    if (decl.header.ccl[0] != 0)
      WriteString(os, decl.header.ccl, sizeof(decl.header.ccl));
    os.Write("\r\n");
  }

  if (decl.header.cm2[0] != '\0') {
    os.Write("HFCM2SECONDCREW:");
    WriteString(os, decl.header.cm2, sizeof(decl.header.cm2));
    os.Write("\r\n");

    if (decl.header.db2Day != 0)
      os.Format("HFDB1SECONDCREWBIRTHDATE:%02d%02d%02d\r\n",
                decl.header.db2Day + 1, decl.header.db2Month + 1,
                decl.header.db2Year % 100);
  }

  if (decl.header.clb[0] != '\0') {
    os.Write("HFCLBCLUB:");
    WriteString(os, decl.header.clb, sizeof(decl.header.clb));
    os.Write("\r\n");
  }

  if (decl.header.sit[0] != '\0') {
    os.Write("HFCLBSITE:");
    WriteString(os, decl.header.sit, sizeof(decl.header.sit));
    os.Write("\r\n");
  }

  if ((decl.header.sensor & IMINO_ENL_MASK) != 0)
    os.Write("I033638FXA3940SIU\r\n");
  else
    os.Write("I033638FXA3940SIU4143ENL\r\n");

  os.Write("J020810HDT1113GSP\r\n");

  unsigned count = CountWaypoints(decl.wp);
  if (count >= 2) {
    BrokenDateTime decl_date = ConvertToDateTime(decl.header.date);
    os.Format("C%02d%02d%02d", decl_date.day, decl_date.month,
              decl_date.year % 100);
    os.Format("%02d%02d%02d", decl_date.hour, decl_date.minute,
              decl_date.second);

    if (decl.header.tskYear != 0)
      os.Format("%02d%02d%02d", decl.header.tskDay + 1,
                decl.header.tskMonth + 1, decl.header.tskYear % 100);
    else
      os.Write("000000");

    os.Format("%04d%02d", decl.header.tskNumber, count - 2);
    WriteString(os, decl.header.tskName, sizeof(decl.header.tskName));
    os.Write("\r\n");

    const IMI::TWaypoint *wp = decl.wp;
    for (unsigned i = 0; i < count + 2; i++, wp++) {
      AngleConverter l;
      l.value = wp->lat;
      os.Format("C%02d%05d%c", l.degrees, l.milliminutes,
                (l.sign ? 'S' : 'N'));

      l.value = wp->lon;
      os.Format("%03d%05d%c", l.degrees, l.milliminutes,
                (l.sign ? 'W' : 'E'));

      WriteString(os, wp->name, sizeof(wp->name));
      os.Write("\r\n");
    }
  }

  os.Write("LIMIFLIGHTOFDAY:");
  os.Format("%03d", decl.header.flightOfDay);
  os.Write("\r\n");

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
  using namespace IMI;

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
IMI::WriteFix(BufferedOutputStream &os, const Fix &fix, bool fromB2, int no_enl)
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

    os.Write('B');
    BrokenTime time = ConvertToDateTime(fix.time);
    os.Format("%02d%02d%02d", time.hour, time.minute, time.second);

    angle.value = (IMIDWORD)fix_b->lat;
    os.Format("%02d%05d", angle.degrees, angle.milliminutes);

    os.Write(angle.sign ? 'S' : 'N');

    angle.value = (IMIDWORD)fix_b->lon;
    os.Format("%03d%05d", angle.degrees, angle.milliminutes);

    os.Write(angle.sign ? 'W' : 'E');

    os.Write("VA??"[fix_b->fv]);

    int alt = (int)fix_b->alt - 1000;
    os.Format(alt < 0 ? "%04d" : "%05d", alt);
    int gpsalt = (int)fix_b->gpsalt - 1000;
    os.Format(alt < 0 ? "%04d" : "%05d", gpsalt);

    os.Format("%03d", fix_b->fxa);
    os.Format("%02d", siu);

    if (!no_enl)
      os.Format("%03d", fix_b->enl);

    append_line_break = true;

    if (!fromB2)
      memcpy(&fixBLastFull, &fix, sizeof(fixBLastFull));
  } else if (fix.id == IMIFIX_ID_B2_RECORD) {
    const FixB2 *fix_b2 = (const FixB2 *)&fix;
    if (SplitB2Fix(fix_b2, &fixBLastFull, &fixB1, &fixB2)) {
      WriteFix(os, *(Fix*)&fixB1, true, no_enl);
      WriteFix(os, *(Fix*)&fixB2, true, no_enl);
    }
  } else if (fix.id == IMIFIX_ID_K_RECORD) {
    const FixK *fix_k = (const FixK *)&fix;

    os.Write('K');
    BrokenTime time = ConvertToDateTime(fix.time);
    os.Format("%02d%02d%02d", time.hour, time.minute, time.second);

    os.Format("%03d", fix_k->hdt);
    os.Format("%03d", fix_k->gsp);
    append_line_break = true;
  } else if (fix.id == IMIFIX_ID_E_RECORD) {
    const FixE *fix_e = (const FixE *)&fix;
    if (fix_e->type == IMIFIX_E_TYPE_SATELLITES) {
      siu = 0;
      os.Write('F');
      BrokenTime time = ConvertToDateTime(fix.time);
      os.Format("%02d%02d%02d", time.hour, time.minute, time.second);

      for (unsigned i = 0; i < sizeof(fix_e->text); i++) {
        if (fix_e->text[i] > 0) {
          os.Format("%02d", fix_e->text[i]);
          siu++;
        }
      }
      append_line_break = true;
    } else if (fix_e->type == IMIFIX_E_TYPE_COMMENT) {
      os.Write("LIMI");
      BrokenTime time = ConvertToDateTime(fix.time);
      os.Format("%02d%02d%02d", time.hour, time.minute, time.second);
      WriteString(os, (const char *)fix_e->text, sizeof(fix_e->text));

      append_line_break = true;
    } else if (fix_e->type == IMIFIX_E_TYPE_PEV) {
      os.Write('E');
      BrokenTime time = ConvertToDateTime(fix.time);
      os.Format("%02d%02d%02d", time.hour, time.minute, time.second);
      WriteString(os, (const char *)fix_e->text, sizeof(fix_e->text));

      append_line_break = true;
    } else if (fix_e->type == IMIFIX_E_TYPE_TASK) {
      os.Write('E');
      BrokenTime time = ConvertToDateTime(fix.time);
      os.Format("%02d%02d%02d", time.hour, time.minute, time.second);

      if (fix_e->text[0] == 1) {
        os.Write("STA");
        WriteString(os, (const char *)fix_e->text + 2, sizeof(fix_e->text) - 2);
      } else {
        if (fix_e->text[0] == 2)
          os.Write("ONT");
        else if (fix_e->text[0] == 3)
          os.Write("FIN");
        else
          os.Write("TPC");

        os.Format("%02d", fix_e->text[1]);
        WriteString(os, (const char *)fix_e->text + 2, sizeof(fix_e->text) - 2);
      }

      append_line_break = true;
    }
  }

  if (append_line_break)
    os.Write("\r\n");
}

void
IMI::WriteSignature(BufferedOutputStream &os, const Signature &sig, IMIWORD sn)
{
  const IMICHAR _hexChars[] = "0123456789ABCDEF";

  if (sn == 0)
    os.Write("GINVALID:Invalid serial number\r\n");
  else if (sig.rsaBits != 512 && sig.rsaBits != 768 && sig.rsaBits != 1024)
    os.Write("GINVALID:Invalid RSA key size\r\n");
  else if (sig.tampered)
    os.Write("GINVALID:Tamper detected\r\n");
  else {
    os.Write('G');

    const IMICHAR *signature = (const IMICHAR*)sig.signature;
    for (unsigned i = 0; i < (sig.rsaBits / 8u);) {
      IMICHAR ch = signature[i++];
      os.Write(_hexChars[(ch & 0xF0) >> 4]);
      os.Write(_hexChars[(ch & 0x0F)]);

      if ((i & (32 - 1)) == 0) {
        os.Write("\r\n");

        if (i + 1 < (sig.rsaBits / 8u))
          os.Write('G');
      }
    }
  }
}
