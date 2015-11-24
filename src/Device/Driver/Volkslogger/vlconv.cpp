/***********************************************************************
**
**   vlconv.cpp
**
**   This file is part of libkfrgcs.
**
************************************************************************
**
**   Copyright (c):  2002 by Garrecht Ingenieurgesellschaft
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

/*
 * Konvertierroutinen
 *
 * vom binären Volkslogger-Format  GCS ins IGC-Format
 * vom binären Directory-Format ins Directory-Array
 *
 *
 * Conversion methods
 *
 * from the binary Volkslogger format GCS to the IGC format
 * from the binary directory format to the directory array
 */

// VLAPI-Includes
#include "vlconv.h"
#include "vlapihlp.h"

// C-Includes
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

// Conversion-Constants

#define MFR_ID "GCS"   // manufacturer three-letter-code
#define MFR_ID2  "A"   // manufacturer letter

/* DS-Haupttypen */
#define rectyp_msk   0xE0 //Haupttypmaske

#define	rectyp_vrt   0x00 //Variabel, timed
#define	rectyp_vrb   0x20 //Variabel, untimed
#define	rectyp_sep   0x40 //separator (head)
#define rectyp_end   0x60 //Security
#define	rectyp_pos   0x80 //Pos-DS (Fix)
#define	rectyp_tnd   0xA0 //Time&Date
#define rectyp_fil   0xC0 //Füllzeichen
#define rectyp_poc   0xE0 //komprimierter Pos-DS

// höchste, von diesem Programm verdaute Binärdateiversion
// bfw = "binary file version"
const int max_bfv=1;

// Größe der Fix-Datensätze in den verschiedenen Binärdateiversionen
const int  pos_ds_size[max_bfv+1][2] = {
  {11,0},
  {12,9}
};

/**
 * Structure of a coordinate, including latitude and longitude
 */
class KOORD
{
public:
  /** Latitude */
  long lat;
  /** Longitude */
  long lon;

  /** Constructor */
  KOORD()
  {
    lat = 0;
    lon = 0;
  }

  void
  print(FILE *aus)
  {
    int lat_deg, lon_deg;
    long lat_tm, lon_tm;
    long t;

    char lat_sgn, lon_sgn;
    lat_sgn = ((lat >= 0) ? 'N' : 'S');
    lon_sgn = ((lon >= 0) ? 'E' : 'W');

    t = labs(lat);
    if (t >= 5400000L)
      t = 5400000L;

    lat_deg = t / 60000L;
    lat_tm = t % 60000L;

    t = labs(lon);
    if (t >= 10800000L)
      t = 10800000L;

    lon_deg = t / 60000L;
    lon_tm = t % 60000L;

    fprintf(aus, "%02u%05lu%c%03u%05lu%c", lat_deg, lat_tm, lat_sgn, lon_deg,
        lon_tm, lon_sgn);
  }
};

/**
 * Structure of a waypoint for C records
 */
class C2
{
public:
  /** Waypoint name */
  char name[7];
  /** Waypoint type */
  int typ;
  /** Waypoint location */
  KOORD koord;
  /** Waypoint ID */
  int i;

  /** Constructor */
  C2()
  {
    typ = 0;
    strcpy(name, "      ");
  }

  void
  packed2unpacked(const uint8_t *packed)
  {
    //_fmemcpy(name,packed,6);
    for (i = 0; i < 6; i++)
      name[i] = packed[i];

    name[6] = 0; //6,10-12
    typ = packed[6] & 0x7F;
    koord.lat = 65536L * (packed[7] & 0x7F) + 256L * packed[8] + packed[9];
    koord.lat = (packed[7] & 0x80 ? -koord.lat : koord.lat);
    koord.lon = 65536L * packed[10] + 256L * packed[11] + packed[12];
    koord.lon = (packed[6] & 0x80 ? -koord.lon : koord.lon);
  }

  void
  print(int version, FILE *aus, const char *descr)
  {
    fprintf(aus, "C");
    koord.print(aus);
    igc_filter(name);
    if (version < 413)
      // Waypoint description only in old file versions
      fprintf(aus, "%s:%s\n", descr, name);
    else
      fprintf(aus, "%s\n", name);
  }
};

// Struktur für Flugaufgabe
//
/**
 * Structure of a C record (task)
 */
class C_RECORD
{
public:
  /** Number of waypoints */
  unsigned NTP;
  /** Task-ID */
  unsigned TID;
  /** Time of declaration */
  BrokenDateTime TDECL;
  int hasdeclaration;
  char sTDECL[20];
  /** Expected date of flight */
  uint8_t FDT[3];
  /** Location of takeoff */
  C2 TKF;
  /** Location of task start */
  C2 STA;
  /** Location of task end */
  C2 FIN;
  /** Location of landing */
  C2 LDG;
  /** Location of the turnpoints */
  C2 TP[12];
  BrokenDateTime T_FDT;
  int zz_min;

public:
  void
  print(int version, FILE *aus)
  {
    char is[8];

    if (hasdeclaration) {
      strcpy(sTDECL, "            ");

      if (TID > 9999)
        TID = 9999; // Größenbegrenzungen wg. Ausdruck

      if (NTP > 12)
        NTP = 12;

      snprintf(sTDECL, sizeof sTDECL, "%02u%02u%02u%02u%02u%02u",
               TDECL.day, TDECL.month , TDECL.year % 100,
               TDECL.hour, TDECL.minute, TDECL.second);

      // If no FDT field was received from the logger (from FW 161 upwards)
      // we have to create one
      if (!(FDT[0] | FDT[1] | FDT[2])) {
        // TDECL is used as a base
        T_FDT = TDECL;
        // Add the timezone
        T_FDT = T_FDT + zz_min*60; // add timezone in seconds
        FDT[0] = T_FDT.day;
        FDT[1] = T_FDT.month;
        FDT[2] = T_FDT.year % 100;
      }

      if (version >= 422) {
        FDT[0] = 0;
        FDT[1] = 0;
        FDT[2] = 0;
      }

      if (FDT[0] > 31)
        FDT[0] = 31;
      if (FDT[1] > 12)
        FDT[1] = 12;
      if (FDT[2] > 99)
        FDT[2] = 99;

      fprintf(aus, "C%s%02u%02u%02u%04u%02u\n", sTDECL, FDT[0], FDT[1], FDT[2],
          TID, NTP); // print C1-Record

      TKF.print(version, aus, "Takeoff"); // print Takeoff-Point
      STA.print(version, aus, "Start  "); // print Start-Point

      // print Turnpoints
      for (unsigned i = 0; i < NTP; i++) {
        sprintf(is, "TP%02u   ", i + 1);
        TP[i].print(version, aus, is);
      }

      FIN.print(version, aus, "Finish "); // print Finish-Point
      LDG.print(version, aus, "Landing"); // print Landing-Point
    }
  }

  C_RECORD()
  {
    NTP = 0;
    TID = 0;
    memset(&T_FDT, 0, sizeof T_FDT);
    zz_min = 0;
    TDECL = BrokenDateTime::Invalid();
    strcpy(sTDECL, "            ");
    memset(FDT, 0, sizeof FDT);
  }
};

/**
 * Structure of the IGC header
 */
struct IGCHEADER
{
  char A[10], DTE[10], FXA[10], PLT[80], GTY[50], GID[50], RFW[10], RHW[10],
       FTY[50], DTM[10], CID[50], CCL[50], TZN[20];

  /** Constructor */
  IGCHEADER(void)
  {
    DTE[0] = 0;
    FXA[0] = 0;
    PLT[0] = 0;
    GTY[0] = 0;
    GID[0] = 0;

    RFW[0] = 0;
    RHW[0] = 0;
    FTY[0] = 0;
    DTM[0] = 0;
    CID[0] = 0;
    CCL[0] = 0;
    TZN[0] = 0;
  }

  /**
   * Output of the IGC header fields without the unused fields.
   * Unused field will be prepared as HO fields.
   */
  void
  output(FILE *ausgabe, int version, bool oo_fillin)
  {
    igc_filter(PLT);
    igc_filter(GTY);

    igc_filter(GID);
    igc_filter(CCL);
    igc_filter(CID);

    igc_filter(A);
    if ((version < 413) || (version >= 416)) { // aus Kompatibilität zu alten Versionen
      fprintf(ausgabe, "A%s%s\n", MFR_ID, A);
    } else { // ab Ver.1: Anzeige der Konverterversion im A-Record
      fprintf(ausgabe,
          "A%s%s  :%01d.%02d created by DATA-GCS version:%01d.%02d\n", MFR_ID,
          A, version / 100, version % 100, version / 100, version % 100);
    }

    igc_filter(DTE);
    fprintf(ausgabe, "HFDTE%s\n", DTE);

    igc_filter(FXA);
    fprintf(ausgabe, "HFFXA%s\n", FXA);

    if (PLT[0])
      fprintf(ausgabe, "HFPLTPILOT:%s\n", PLT);
    else if (oo_fillin)
      fprintf(ausgabe, "HOPLTPILOT:\n");

    if (GTY[0])
      fprintf(ausgabe, "HFGTYGLIDERTYPE:%s\n", GTY);
    else if (oo_fillin)
      fprintf(ausgabe, "HOGTYGLIDERTYPE:\n");

    if (GID[0])
      fprintf(ausgabe, "HFGIDGLIDERID:%s\n", GID);
    else if (oo_fillin)
      fprintf(ausgabe, "HOGIDGLIDERID:\n");

    fprintf(ausgabe, "HFDTM%sGPSDATUM:%s\n", DTM, "WGS84");

    fprintf(ausgabe, "HFRFWFIRMWAREVERSION:%s\n", RFW);
    fprintf(ausgabe, "HFRHWHARDWAREVERSION:%s\n", RHW);

    if ((version >= 421) && (FTY[0]))
      fprintf(ausgabe, "HFFTYFR TYPE:%s\n", FTY);

    if (CID[0])
      fprintf(ausgabe, "HFCIDCOMPETITIONID:%s\n", CID);

    if (CCL[0])
      fprintf(ausgabe, "HFCCLCOMPETITIONCLASS:%s\n", CCL);

    if (TZN[0])
      fprintf(ausgabe, "HFTZNTIMEZONE:%s\n", TZN);

    // For old files
    if (((version < 413) || (version >= 416)) && (strcmp(RHW, "3.3") < 0))
      fprintf(ausgabe, "I013638FXA\n");
    else {
      // only activate, if RHW > certain value (with microfone)
      fprintf(ausgabe, "I023638FXA3941ENL\n");
    }

    // LCONV-VER-Erzeugung ab Version 4.16
    if (version >= 416 && oo_fillin)
      fprintf(ausgabe, "LCONV-VER:%01d.%02d\n", version / 100, version % 100);
  }
};

/******************************************************************
   Helper functions
******************************************************************/

/**
 * more or less inaccurate conversion from HDOP to fix accuracy.
 * doesn't have to be right. might be changed sometime.
 */
static unsigned
hdop2fxa(uint8_t hdop)
{
  // return (float) hdop * 100.0/3);
  return unsigned((float(hdop) * 100.01 / 3));
}

/** non-linear conversion of the ENL values */
static int
enlflt(int enl)
{
  if (enl < 500)
    enl /= 2;
  else if (enl < 750)
    enl = 250 + 2 * (enl - 500);
  else

    enl = int(750 + (enl - 750) * 1.5);
  return enl;
}

/** ENL limitation */
static int
enllim(int enl)
{
  if (enl > 999)
    enl = 999;
  return enl;
}

/*
Binären Datenblock *bin_puffer in das IGC-Format umwandeln und in der
Datei *Ausgabedatei speichern


Konvertierung erfolgt in 2 Phasen:
1) Auffüllen der HFxxx-Records mit den Daten aus GCS-Datei
   Auffüllen der C-Records mit den Daten aus der GCS-Datei
2) Ausdrucken der Hxxxx-Records in der vorgeschriebenen Reihenfolge,
   leere, aber vorgeschriebene H-Records werden als HO-Records ausgedruckt

   Ausdrucken des I-Records (I013638FXA)
   Ausdrucken des ersten C-Records in der vorgeschriebenen Reihenfolge
   Konvertierung und Ausdrucken der B- und E-Datensätze

Binärdatensatztypen (Achtung: aktuelles Datenformat siehe Firmwarelisting):
  T mit rel. Zeit  / ohne rel. Zeit
  V variable Länge / feste Länge
     TV  L
sep        1  Flugtrennzeichen
pos  x    11  Positionsdatensatz
vrb   x  >=2  Variabel, ohne rel .Zeit
vrt  xx  >=3  Variabel, mit rel. Zeit
sec       41  Signatur
tnd  x     8  Zeit und Datum
*/


/*
Parameter
  igcfile_version:
    Version von DATA-GCS, die der Konverter simulieren soll
    dies ist aus Kompatibilitätsgründen erforderlich
  Ausgabedatei:
    Datei, in die das Ergebnis der Konvertierung (IGC-Datei) geschrieben
    wird
  bin_puffer:
    Zeiger auf den Speicherbereich, in dem die binäre formatierte IGC-
    Datei, so wie sie vom Logger kommt, liegt
  oo_fillin:
    ???
  *serno:
    Seriennummer des Loggers, aus dem die Datei stammt
  *sp:
    Position, an der die Signatur in der Binärdatei liegt
*/

static constexpr int igcfile_version = 424;

size_t
convert_gcs(FILE *Ausgabedatei,
            const uint8_t *const bin_puffer, size_t length,
            bool oo_fillin)
{
  const uint8_t *const end = bin_puffer + length;

  IGCHEADER igcheader;
  C_RECORD task;
  struct
  {
    // All value directly from fix, before conversion
    char time[10];
    char valid;
    long lat;
    long lon;
  } igcfix;

  int l = 0;
  int ende;

  // relative time from the beginning of the logging
  long time_relative = 0;
  long temptime;
  long decl_time;
  BrokenDateTime firsttime;

  BrokenDateTime realtime;
  uint8_t Haupttyp;
  uint8_t Untertyp;
  const uint8_t *p;
  const uint8_t *p2;
  long pl;
  char PILOT[40];
  int tzh, tzm;
  // word           keysn;
  // binary data format version
  int bfv = 0;
  long delta_lat, delta_lon;
  // Timezone in minutes from FDT field
  int tzn = 4000;
  // Ini value for determination, whether the field existed
  float ftz = 0;
  // Flag, whether ftz is received from a valid position
  int tzset = 0;

  // long ggtz = timezone;

  igcfix.lat = 0;
  igcfix.lon = 0;

  decl_time = -1;

  ende = 0;
  p = bin_puffer;

  do {
    if (p >= end)
      return 0;

    Haupttyp = p[0] & rectyp_msk;
    switch (Haupttyp) {
    case rectyp_tnd:
      if (p + 8 > end)
        return 0;

      // calculates backwards the time of the first fix
      time_relative += p[1];
      temptime = 65536L * p[2] + 256L * p[3] + p[4];
      firsttime = BrokenDateTime(BrokenDate::Invalid(),
                                 BrokenTime::FromSecondOfDay(temptime));
      firsttime.day = 10 * (p[7] >> 4) + (p[7] & 0x0f);
      firsttime.month = 10 * (p[6] >> 4) + (p[6] & 0x0f);
      firsttime.year = 10 * (p[5] >> 4) + (p[5] & 0x0f) + 1900;
      // Y2K-handling
      if (igcfile_version >= 424)
        if (firsttime.year < 1980)
          firsttime.year += 100;

      firsttime = firsttime - time_relative;
      l = 8;
      break;
      /*
       case rectyp_pos : time_relative += p[2];
       l = pos_ds_size[bfv][0];
       break;
       case rectyp_poc :	if(p[2] & 0x80) { // Endebedingung
       ende = 1;
       l = 0;
       break;
       }
       time_relative += p[2];
       l = pos_ds_size[bfv][1];
       break;
       */
    case rectyp_pos:
    case rectyp_poc:
      if (p + 2 >= end)
        return 0;

      if (p[2] & 0x80) { // Endebedingung
        ende = 1;
        l = 0;
        break;
      }

      if (p + 8 > end)
        return 0;

      time_relative += p[2];
      igcfix.valid = ((p[0] & 0x10) >> 4) ? 'A' : 'V';
      if (Haupttyp == rectyp_pos) {
        l = pos_ds_size[bfv][0];
        igcfix.lon = ((unsigned long)p[6]) << 16 | ((unsigned long)p[7]) << 8
            | p[8];
        if (p[9] & 0x80)
          igcfix.lon = -igcfix.lon;
      } else {
        l = pos_ds_size[bfv][1];
        delta_lon = (((unsigned long)p[3] & 0x78) << 5) | p[5];
        if (p[6] & 0x80)
          delta_lon = -delta_lon;
        igcfix.lon += delta_lon;
      }
      // ftz mit Längengrad füllen
      // der erste gültige ist der letzte,
      // der in ftz gespeichert wird
      if (!tzset) {
        ftz = float(igcfix.lon);
        if (igcfix.valid == 'A')
          tzset = 1;
      }
      break;
    case rectyp_sep:
      time_relative = 0;
      bfv = p[0] & ~rectyp_msk;
      if (bfv > max_bfv) {
        // unsupported binary file version
        return 0;
      }
      l = 1;
      break;
    case 0xC0:
      l = 1; // Füllzeichen
      break;
    case rectyp_end:
      /* sp = (p - bin_puffer) + 1 */
      l = 41;
      ende = 1;
      break;
    case rectyp_vrb:
    case rectyp_vrt:
      if (p + 2 >= end)
        return 0;

      l = p[1];
      switch (Haupttyp) {
      case rectyp_vrb:
        p2 = p + 2;
        break;
      case rectyp_vrt:
        time_relative += p[2];
        p2 = p + 3;
        break;
      default:
        p2 = p;
        break;
      }

      if (p2 >= end)
        return 0;

      Untertyp = (p2[0]);
      switch (Untertyp) {

      case FLDNTP:
        if (p2 + 1 >= end)
          return 0;

        task.NTP = p2[1];
        decl_time = time_relative;
        break;
      case FLDTID:
        if (p2 + 2 >= end)
          return 0;

        task.TID = 256 * p2[1] + p2[2];
        if (igcfile_version >= 422)
          decl_time = time_relative;
        break;
      case FLDFDT:
        if (p2 + 1 + sizeof(task.FDT) > end)
          return 0;

        //_fmemcpy(&task.FDT,&p2[1],sizeof task.FDT);
        memcpy(&task.FDT, &p2[1], sizeof task.FDT);
        break;
      case FLDTZN: // Reading timezone offset
        if (p2 + 1 >= end)
          return 0;

        if (p2[1] < 128)
          tzn = 15 * p2[1];
        else
          tzn = 15 * (p2[1] - 256);
        break;

      case FLDTKF:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TKF.packed2unpacked(&p2[1]);
        break;
      case FLDSTA:
        if (p2 + 1 + 12 > end)
          return 0;

        task.STA.packed2unpacked(&p2[1]);
        break;
      case FLDFIN:
        if (p2 + 1 + 12 > end)
          return 0;

        task.FIN.packed2unpacked(&p2[1]);
        break;
      case FLDLDG:
        if (p2 + 1 + 12 > end)
          return 0;

        task.LDG.packed2unpacked(&p2[1]);
        break;
      case FLDTP1:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TP[0].packed2unpacked(&p2[1]);
        break;
      case FLDTP2:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TP[1].packed2unpacked(&p2[1]);
        break;
      case FLDTP3:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TP[2].packed2unpacked(&p2[1]);
        break;
			  case FLDTP4:
        task.TP[3].packed2unpacked(&p2[1]);
        break;
      case FLDTP5:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TP[4].packed2unpacked(&p2[1]);
        break;
      case FLDTP6:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TP[5].packed2unpacked(&p2[1]);
        break;
      case FLDTP7:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TP[6].packed2unpacked(&p2[1]);
        break;
      case FLDTP8:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TP[7].packed2unpacked(&p2[1]);
        break;
      case FLDTP9:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TP[8].packed2unpacked(&p2[1]);
        break;
      case FLDTP10:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TP[9].packed2unpacked(&p2[1]);
        break;
      case FLDTP11:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TP[10].packed2unpacked(&p2[1]);
        break;
      case FLDTP12:
        if (p2 + 1 + 12 > end)
          return 0;

        task.TP[11].packed2unpacked(&p2[1]);
        break;

      case FLDPLT1: // Reading pilotname
      case FLDPLT2: // Reading pilotname
      case FLDPLT3: // Reading pilotname
      case FLDPLT4: // Reading pilotname
        if (p2 + 1 + sizeof(PILOT) > end)
          return 0;

        // _fmemcpy(igcheader.PLT, &p2[1], (sizeof igcheader.PLT));
        // igcheader.PLT[(sizeof igcheader.PLT)-1] = 0;
        //_fmemcpy(PILOT, &p2[1], (sizeof PILOT));
        memcpy(PILOT, &p2[1], (sizeof PILOT));
        PILOT[(sizeof PILOT) - 1] = 0;
        strcat(igcheader.PLT, PILOT);
        if (igcfile_version < 413)
          strcat(igcheader.PLT, " ");
        break;
      case FLDGTY: // Reading plane type
        if (p2 + 1 + sizeof(igcheader.GTY) > end)
          return 0;

        //_fmemcpy(igcheader.GTY, &p2[1], (sizeof igcheader.GTY));
        memcpy(igcheader.GTY, &p2[1], (sizeof igcheader.GTY));
        igcheader.GTY[(sizeof igcheader.GTY) - 1] = 0;
        break;
      case FLDGID: // Reading plane reg
        if (p2 + 1 + sizeof(igcheader.GID) > end)
          return 0;

        //_fmemcpy(igcheader.GID, &p2[1], (sizeof igcheader.GID));
        memcpy(igcheader.GID, &p2[1], (sizeof igcheader.GID));
        igcheader.GID[(sizeof igcheader.GID) - 1] = 0;
        break;
      case FLDCCL: // Reading plane class
        if (p2 + 1 + sizeof(igcheader.CCL) > end)
          return 0;

        //_fmemcpy(igcheader.CCL, &p2[1], (sizeof igcheader.CCL));
        memcpy(igcheader.CCL, &p2[1], (sizeof igcheader.CCL));
        igcheader.CCL[(sizeof igcheader.CCL) - 1] = 0;
        break;
      case FLDCID: // Reading plane competition sign
        if (p2 + 1 + sizeof(igcheader.CID) > end)
          return 0;

        //_fmemcpy(igcheader.CID, &p2[1], (sizeof igcheader.CID));
        memcpy(igcheader.CID, &p2[1], (sizeof igcheader.CID));
        igcheader.CID[(sizeof igcheader.CID) - 1] = 0;
        break;
      case FLDHDR: // Reading serial number and other stuff
        // Public-Key erst mal löschen
        // 19.10.99 weggemacht, weil schon in main vorhanden
        //dsa_y_b[0] = 2; dsa_y_b[1] = 0;
        //memset(&dsa_y_b[2],0,(sizeof dsa_y_b)-2);

        if (p2 + 7 >= end)
          return 0;

        // sonstiges einlesen
        wordtoserno(igcheader.A, 256 * p2[1] + p2[2]);

        sprintf(igcheader.DTM, "%03u", p2[3]);
        sprintf(igcheader.RHW, "%0X.%0X", p2[4] >> 4, (p2[4] & 0xf));
        sprintf(igcheader.RFW, "%0X.%0X", p2[5] >> 4, (p2[5] & 0xf));
        sprintf(igcheader.FXA, "%03u", p2[7]);

        // neuer obligatorischer H-Record
        if (igcfile_version >= 421)
          sprintf(igcheader.FTY,
              "GARRECHT INGENIEURGESELLSCHAFT,VOLKSLOGGER 1.0");
        break;
      }
      ;
      break;

    default:
      ende = 1;
      break;
    }
    p += l;
  } while (!ende);

  pl = p - bin_puffer;

  // Timezone/Hours = floor(LON + 7.5°) / 15° of the first fix
  ftz = ftz + 450000L;
  ftz = ftz / 900000L;
  task.zz_min = int(60 * floor(ftz));
  //  printf("theoretische Zeitzone = UTC %-d min\n",task.zz_min);
  //  getch();

  // for new files
  if ((igcfile_version >= 420) && (igcfile_version < 422))
    // if TZN field does not exist
    if (tzn == 4000)
      // emulate it with the calculated
      tzn = task.zz_min;

  // for all files
  // show TZN, if set
  if (tzn != 4000) {
    tzh = abs(tzn) / 60;
    tzm = abs(tzn) % 60;
    sprintf(igcheader.TZN, "UTC%c%02d:%02d", (tzn < 0 ? '-' : '+'), tzh, tzm);
  }

  snprintf(igcheader.DTE, sizeof(igcheader.DTE), "%02u%02u%02u",
           firsttime.day, firsttime.month, firsttime.year % 100);
  igcheader.output(Ausgabedatei, igcfile_version, oo_fillin);

  if (igcfile_version >= 414 || (task.STA.koord.lat != 0)
      || (task.STA.koord.lon != 0)) {
    if (decl_time >= 0) {
      task.hasdeclaration = 1;
      task.TDECL = firsttime + decl_time;
      task.print(igcfile_version, Ausgabedatei);
    }
  }

  igcfix.lat = 0;
  igcfix.lon = 0;

  realtime = firsttime;
  ende = 0;
  p = bin_puffer;
  do {
    Haupttyp = p[0] & rectyp_msk;
    switch (Haupttyp) {
    case rectyp_sep:
      l = 1;
      if (bfv > max_bfv) {
        ende = 1;
        l = 0;
        break;
      }
      break;
    case 0xC0:
      l = 1;
      break;
    case rectyp_pos:
    case rectyp_poc: {
      if (p[2] & 0x80) { // Endebedingung
        ende = 1;
        l = 0;
        break;
      }
      time_relative += p[2];
      realtime = realtime + p[2];
      igcfix.valid = ((p[0] & 0x10) >> 4) ? 'A' : 'V';
      const unsigned press = unsigned(p[0] & 0x0f) << 8 | p[1];
      unsigned gpalt, fxa, enl;
      if (Haupttyp == rectyp_pos) {
        l = pos_ds_size[bfv][0];
        igcfix.lat = ((unsigned long)p[3] & 0x7f) << 16 | ((unsigned long)p[4])
            << 8 | p[5];
        if (p[3] & 0x80)
          igcfix.lat = -igcfix.lat;

        igcfix.lon = ((unsigned long)p[6]) << 16 | ((unsigned long)p[7]) << 8
            | p[8];
        if (p[9] & 0x80)
          igcfix.lon = -igcfix.lon;

        gpalt = unsigned(p[9] & 0x70) << 4 | p[10];
        fxa = hdop2fxa(p[9] & 0x0f);
        enl = 4 * p[11];
      }

      else {
        l = pos_ds_size[bfv][1];
        delta_lat = (((unsigned long)p[3] & 0x07) << 8) | p[4];
        if (p[3] & 0x80)
          delta_lat = -delta_lat;
        delta_lon = (((unsigned long)p[3] & 0x78) << 5) | p[5];
        if (p[6] & 0x80)
          delta_lon = -delta_lon;

        igcfix.lat += delta_lat;
        igcfix.lon += delta_lon;
        gpalt = unsigned(p[6] & 0x70) << 4 | p[7];
        fxa = hdop2fxa(p[6] & 0x0f);
        enl = 4 * p[8];
      }

      const unsigned latdeg = labs(igcfix.lat) / 60000;
      const unsigned latmin = labs(igcfix.lat) % 60000;
      const unsigned londeg = labs(igcfix.lon) / 60000;
      const unsigned lonmin = labs(igcfix.lon) % 60000;

      long gps_alt = 10L * gpalt - 1000L;

      if (igcfile_version >= 423)
        enl = enlflt(enl);
      enl = enllim(enl);

      // Bei allen neuen Dateien auf Wunsch von IAN
      // aber dank neuer Regeln ab
      // Konverter Nr. 4.20 nicht mehr !!
      if ((igcfile_version >= 413) && (igcfile_version < 420))
        if (igcfix.valid == 'V')
          gps_alt = 0;

      const long pressure_alt = pressure2altitude(press);

      snprintf(igcfix.time,sizeof(igcfix.time), "%02u%02u%02u",
               realtime.hour, realtime.minute, realtime.second);
      fprintf(Ausgabedatei, "B%6s%02u%05u%c%03u%05u%c%c%05ld%05ld%03u",
              igcfix.time,
              latdeg, latmin, igcfix.lat < 0 ? 'S' : 'N',
              londeg, lonmin, igcfix.lon < 0 ? 'W' : 'E',
              igcfix.valid, pressure_alt, gps_alt,
              fxa);

      // activate on ENL in I record
      if ((igcfile_version >= 413) && (igcfile_version < 416))
        fprintf(Ausgabedatei, "999");

      // have to be active, if sensor exists
      if (strcmp(igcheader.RHW, "3.3") >= 0)
        fprintf(Ausgabedatei, "%03u", enl);

      fprintf(Ausgabedatei, "\n");
    }
      break;

    case rectyp_vrb:
    case rectyp_vrt:
      l = p[1];
      switch (Haupttyp) {
      case rectyp_vrb:
        p2 = p + 2;
        break;
      case rectyp_vrt:
        realtime = realtime + p[2];
        p2 = p + 3;
        break;
      default:
        p2 = p;
        break;
      }
      Untertyp = (p2[0]);
      switch (Untertyp) {
      case FLDEPEV:
        snprintf(igcfix.time,sizeof(igcfix.time), "%02u%02u%02u",
                 realtime.hour, realtime.minute, realtime.second);
        fprintf(Ausgabedatei, "E%6sPEVEVENTBUTTON PRESSED\n", igcfix.time);
        break;
      case FLDETKF:
        snprintf(igcfix.time,sizeof(igcfix.time), "%02u%02u%02u",
                         realtime.hour, realtime.minute, realtime.second);
        fprintf(Ausgabedatei, "LGCSTKF%6sTAKEOFF DETECTED\n", igcfix.time);
        break;
      }
      ;
      break;
    case rectyp_tnd:
      realtime = realtime + p[1];
      l = 8;
      break;

    default:
      ende = 1;
      l = 0;
      break;
    }
    p += l;
  } while (!ende);
  return pl;
}

// Members of class DIR
bool
conv_dir(std::vector<DIRENTRY> &flights, const uint8_t *p, const size_t length)
{
  const uint8_t *const end = p + length;

  assert(flights.empty());
  DIRENTRY de; // directory entry
  uint8_t Haupttyp, Untertyp;
  uint8_t l; // length of DS
  const uint8_t *p2; // Pointer to the beginning of the content of a vrb or vrt
  BrokenDateTime olddate;

  int olddate_flg = 0;
  int flight_of_day = 0;
  long temptime;
  BrokenDateTime datetime1;

  int bfv = 0;
  char pilot1[17];
  char pilot2[17];
  char pilot3[17];
  char pilot4[17];
  memset(&de, 0, sizeof(de));

  size_t nbytes = 0;

  while (p < end) {
    Haupttyp = (p[0] & rectyp_msk);
    switch (Haupttyp) {
    case rectyp_sep: // Initialize Dir-Entry
      de.serno = 0;
      de.pilot[0] = 0;
      pilot1[0] = 0;
      pilot2[0] = 0;
      pilot3[0] = 0;
      pilot4[0] = 0;
      de.takeoff = 0;
      bfv = p[0] & ~rectyp_msk;
      if (bfv > max_bfv)
      {
        //abort function
        return false;
      }
      l = 1;
      break;
    case rectyp_vrt: // getim'ter variabler DS oder
    case rectyp_vrb: // ungetim'ter variabler DS
      if (p + 1 >= end)
        return false;

      l = p[1];
      switch (Haupttyp) {
      case rectyp_vrb:
        p2 = p + 2;
        break;
      case rectyp_vrt:
        p2 = p + 3;
        break;
      default:
        p2 = p;
        break;
      }

      if (p2 >= end)
        return false;

      Untertyp = (p2[0]);
      switch (Untertyp) {
      case FLDCID: // Read pilotname
        if (p2 + 1 + sizeof(de.competitionid) > end)
          return false;

        memcpy(de.competitionid, &p2[1], sizeof(de.competitionid));
        de.competitionid[sizeof(de.competitionid) - 1] = 0;
        break;
      case FLDGID: // Read pilotname
        if (p2 + 1 + sizeof(de.gliderid) > end)
          return false;

        memcpy(de.gliderid, &p2[1], sizeof(de.gliderid));
        de.gliderid[sizeof(de.gliderid) - 1] = 0;
        break;
      case FLDPLT1: // Read pilotname
        if (p2 + 1 + sizeof(pilot1) > end)
          return false;

        memcpy(pilot1, &p2[1], sizeof(pilot1));
        pilot1[sizeof(pilot1) - 1] = 0;
        break;
      case FLDPLT2: // Read pilotname
        if (p2 + 1 + sizeof(pilot2) > end)
          return false;

        memcpy(pilot2, &p2[1], sizeof(pilot2));
        pilot2[sizeof(pilot2) - 1] = 0;
        break;
      case FLDPLT3: // Read pilotname
        if (p2 + 1 + sizeof(pilot3) > end)
          return false;

        memcpy(pilot3, &p2[1], sizeof(pilot3));
        pilot3[sizeof(pilot3) - 1] = 0;
        break;
      case FLDPLT4: // Read pilotname
        if (p2 + 1 + sizeof(pilot4) > end)
          return false;

        memcpy(pilot4, &p2[1], sizeof(pilot4));
        pilot4[sizeof(pilot4) - 1] = 0;
        break;
      case FLDHDR: // Read serial number
        if (p2 + 2 >= end)
          return false;

        de.serno = 256 * p2[1] + p2[2];
        break;
      case FLDETKF: // set takeoff flag
        de.takeoff = 1;
        break;
      }
      break;
    case 0xC0:

      l = 1;
      break;
    case rectyp_pos:
      l = pos_ds_size[bfv][0];
      break;


    /*
     * This End Condition Statement does not seem to be
     * valid. At least not for the tested Volkslogger.
     * To have a valid condition the length is
     * now known in conv_dir() and the loop will finish when the
     * end of the data is reached.
     * Maybe this end condition here is valid in special
     * cases or on other hardware though.
     */
    case rectyp_poc:
      if (p + 2 >= end)
        return false;

      if (p[2] & 0x80) { // Endebedingung
        return true;
      }
      l = pos_ds_size[bfv][1];
      break;
    case rectyp_tnd:
      if (p + 7 >= end)
        return false;

      // speichert in timetm1 den aktuellen tnd-DS ab
      temptime = 65536L * p[2] + 256L * p[3] + p[4];
      datetime1 = BrokenDateTime(BrokenDate::Invalid(),
                                 BrokenTime::FromSecondOfDay(temptime));
      datetime1.day = 10 * (p[7] >> 4) + (p[7] & 0x0f);
      datetime1.month = 10 * (p[6] >> 4) + (p[6] & 0x0f);
      datetime1.year = 10 * (p[5] >> 4) + (p[5] & 0x0f) + 1900;
      // Y2K-handling
      if (datetime1.year < 1980)
        datetime1.year += 100;
      l = 8;
      break;
    case rectyp_end:
      if (p + 6 >= end)
        return false;

      // setzt firsttime und lasttime aufgrund der Werte im sta-DS
      temptime = 65536L * p[4] + 256L * p[5] + p[6]; // Aufzeichnungsbeginn
      de.firsttime = datetime1 - temptime;

      temptime = 65536L * p[1] + 256L * p[2] + p[3]; // Aufzeichnungsdauer
      de.recordingtime = temptime;
      de.lasttime=de.firsttime + temptime;

      if (!olddate_flg) {
        olddate = de.firsttime;
        flight_of_day = 0;
        olddate_flg = 1;
      }
      if ((olddate.day == de.firsttime.day) && (olddate.month
          == de.firsttime.month)
          && (olddate.year == de.firsttime.year))
        flight_of_day++;
      else {
        olddate = de.firsttime;
        flight_of_day = 1;
        olddate_flg = 1;
      }
      strcat(de.pilot, pilot1);
      strcat(de.pilot, pilot2);
      strcat(de.pilot, pilot3);
      strcat(de.pilot, pilot4);

      flights.push_back(de);
      l = 7;
      break;
    default:
      //abort function
      return false;
    };
    p += l;
    nbytes += l;
  }
  return true;
}
