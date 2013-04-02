/***********************************************************************
**
**   dbbconv.cpp
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

// Low-Level Funktionen für DBB Behandlung (Schreiben)

#include "dbbconv.h"
#include "OS/ByteOrder.hpp"
#include "Util/CharUtil.hpp"

#include <string.h>
#include <assert.h>

DBB::DBB() {
  memset(this,0xff,sizeof *this);
  dbcursor = 8 * 6; // dbcursor direkt hinter den Header-Bereich setzen
  fdfcursor = 0;
  for (int i=0; i<8; i++) {
    header[i].dsanzahl = 0;
  }
  header[0].dslaenge  = 13;
  header[0].keylaenge = 6;
  header[1].dslaenge  = 16;
  header[1].keylaenge = 16;
  header[2].dslaenge  = 7;
  header[2].keylaenge = 7;
  header[3].dslaenge  = 144;
  header[3].keylaenge = 14;
}

void DBB::close_db(int kennung) {
  HEADER *h = &header[kennung];
  // calculate position of last record
  h->dslast = h->dsfirst + (h->dsanzahl-1) * h->dslaenge;
  // build database header

  Volkslogger::TableHeader *dest = GetHeader(kennung);
  dest->start_offset = ToBE16(h->dsfirst);
  dest->end_offset = ToBE16(h->dslaenge);
  dest->dslaenge = h->dslaenge;
  dest->keylaenge = h->keylaenge;
}

void DBB::open_dbb() {
  int i;
  // determine the beginning and length of the database parts
  const Volkslogger::TableHeader *src = GetHeader(0);
  for(i=0; i<8; i++) {
    if (src->start_offset == 0xffff)
      continue;

    header[i].dsfirst = FromBE16(src->start_offset);
    header[i].dslast = FromBE16(src->end_offset);
    header[i].dslaenge = src->dslaenge;
    header[i].keylaenge = src->keylaenge;

    ++src;
  }
}

void
DBB::add_ds(int kennung, const void *quelle)
{
  HEADER *h = &header[kennung];
  // append record if there is space for it
  if ((dbcursor + h->dslaenge) < sizeof(block)) {
    // and only if the database is still open
    if (h->dslast == 0xffff) {
      // save the position of the first record
      if (h->dsanzahl == 0)
        h->dsfirst = dbcursor;
      // save record in memory
      memcpy(&block[dbcursor],quelle,h->dslaenge);
      dbcursor += h->dslaenge;
      h->dsanzahl++;
    }
  }
}

void *
DBB::AddFDF(uint8_t id, size_t size)
{
  assert(size + 2 <= 0xff);

  if (fdfcursor + size + 2 > sizeof(fdf))
    return nullptr;

  fdf[fdfcursor++] = size + 2;
  fdf[fdfcursor++] = id;

  void *result = &fdf[fdfcursor];
  fdfcursor += size;
  return result;
}

void
DBB::add_fdf(int feldkennung, size_t feldlaenge, const void *quelle)
{
  void *dest = AddFDF(feldkennung, feldlaenge);
  if (dest != nullptr)
    memcpy(dest, quelle, feldlaenge);
}

void
DBB::AddFDFStringUpper(uint8_t id, const char *src)
{
  char *dest = (char *)AddFDF(id, strlen(src) + 1);
  if (dest == nullptr)
    return;

  do {
    *dest++ = ToUpperASCII(*src++);
  } while (*src != '\0');
}

int16
DBB::fdf_findfield(uint8_t id) const
{
  int16 ii;
  ii = -1;
  for (unsigned i = 0; i < sizeof(fdf);) {
    if (fdf[i+1] == id) {
      // Feld gefunden
      ii = i;
      break;
    }
    if (fdf[i] == 0)
      // Zyklus verhindern
      return -1;
    i = i + fdf[i];
  }
  return ii;
}
