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

class DBB {
public:
  enum {
    DBBBeg  = 0x0000,
    DBBEnd  = 0x3000,
    FrmBeg  = 0x3000,
    FrmEnd  = 0x4000
  };
  int dbcursor;
  int fdfcursor;
  struct HEADER {
    int dsanzahl;
    int dslaenge, keylaenge;
    unsigned short int dsfirst, dslast;
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
  void add_ds(int kennung,void *quelle);
  void add_fdf(int feldkennung,int feldlaenge, void *quelle);
  int16 fdf_findfield(uint8_t id) const;
};

#endif
