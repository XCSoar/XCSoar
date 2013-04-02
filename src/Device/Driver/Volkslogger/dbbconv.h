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

#include "Database.hpp"

#include <stdint.h>
#include <stddef.h>

class DBB {
public:
  static constexpr size_t DBBBeg = 0x0000;
  static constexpr size_t DBB_SIZE = 0x3000;
  static constexpr size_t DBBEnd = DBBBeg + DBB_SIZE;
  static constexpr size_t FrmBeg = 0x3000;
  static constexpr size_t FRM_SIZE = 0x1000;
  static constexpr size_t FrmEnd = FrmBeg + FRM_SIZE;
  static constexpr size_t SIZE = DBB_SIZE + FRM_SIZE;

  size_t dbcursor;
  size_t fdfcursor;
  struct HEADER {
    unsigned dsanzahl;
    unsigned dslaenge, keylaenge;
    unsigned dsfirst, dslast;
  };
  HEADER header[8];
public:
  uint8_t buffer[SIZE];

  /**
   * Konstruktor: leeren Datenbank-Block erzeugen.
   */
  DBB();

  void *GetBlock(size_t offset=0) {
    return buffer + DBBBeg + offset;
  }

  const void *GetBlock(size_t offset=0) const {
    return buffer + DBBBeg + offset;
  }

  void *GetFDF(size_t offset=0) {
    return buffer + FrmBeg + offset;
  }

  const void *GetFDF(size_t offset=0) const {
    return buffer + FrmBeg + offset;
  }

protected:
  Volkslogger::TableHeader *GetHeader(unsigned i) {
    Volkslogger::TableHeader *h = (Volkslogger::TableHeader *)GetBlock();
    return &h[i];
  }

  const Volkslogger::TableHeader *GetHeader(unsigned i) const {
    const Volkslogger::TableHeader *h =
      (const Volkslogger::TableHeader *)GetBlock();
    return &h[i];
  }

public:
  /**
   * Generate Header-Structure from DBB-File.
   */
  void open_dbb();

  /**
   * Update header of specified table (kennung) of the database and
   * close the table (it can't be extended anymore).
   */
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

  /**
   * Find an actual record of specified type(id) in the declaration
   * memory and return it's position in the memory array.
   */
  int fdf_findfield(uint8_t id) const;
};

#endif
