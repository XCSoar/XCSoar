/***********************************************************************
**
**   dbbconv.h
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

#ifndef DBBCONV_H
#define DBBCONV_H

#include "vlapityp.h"
#include "Database.hpp"

#include <stdint.h>
#include <stddef.h>

class DBB {
public:
  enum {
    DBBBeg  = 0x0000,
    DBBEnd  = 0x3000,
    FrmBeg  = 0x3000,
    FrmEnd  = 0x4000
  };
  size_t dbcursor;
  size_t fdfcursor;
  struct HEADER {
    unsigned dsanzahl;
    unsigned dslaenge, keylaenge;
    unsigned dsfirst, dslast;
  };
  HEADER header[8];
public:
  uint8_t block[DBBEnd-DBBBeg];
  uint8_t fdf[FrmEnd-FrmBeg];
  DBB();

protected:
  Volkslogger::TableHeader *GetHeader(unsigned i) {
    Volkslogger::TableHeader *h = (Volkslogger::TableHeader *)block;
    return &h[i];
  }

  const Volkslogger::TableHeader *GetHeader(unsigned i) const {
    const Volkslogger::TableHeader *h =
      (const Volkslogger::TableHeader *)block;
    return &h[i];
  }

public:
  void open_dbb();
  void close_db(int kennung);
  void add_ds(int kennung, const void *quelle);

  /**
   * Add a FDF entry.
   *
   * @return a writeable pointer of the specified size or nullptr if
   * the buffer is full
   */
  void *AddFDF(uint8_t id, size_t size);

  void add_fdf(int feldkennung, size_t feldlaenge, const void *quelle);

  /**
   * Add a FDF entry with the specified string value (convert to upper
   * case).
   */
  void AddFDFStringUpper(uint8_t id, const char *value);

  int16 fdf_findfield(uint8_t id) const;
};

#endif
