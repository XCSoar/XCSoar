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
#include <memory.h>
#include <string.h>


DBB::DBB() { // Konstruktor: leeren Datenbank-Block erzeugen
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


// update header of specified table (kennung) of the database
// and close the table (it can't be extended anymore)
void DBB::close_db(int kennung) {
 HEADER *h = &header[kennung];
 byte *b = block + kennung*6;
	// calculate position of last record
	h->dslast = h->dsfirst + (h->dsanzahl-1) * h->dslaenge;
	// build database header
	b[0] = h->dsfirst / 256;
	b[1] = h->dsfirst % 256;
	b[2] = h->dslast  / 256;
	b[3] = h->dslast  % 256;
	b[4] = h->dslaenge;
	b[5] = h->keylaenge;
}


// generate Header-Structure from DBB-File
void DBB::open_dbb() {
 int i;
	// determine the beginning and length of the database parts
  for(i=0; i<8; i++) {
    if ( (block[6*i] == 0xff) && (block[6*i+1] == 0xff) )
      continue;
    header[i].dsfirst   = 256*block[6*i+0] + block[6*i+1];
    header[i].dslast    = 256*block[6*i+2] + block[6*i+3];
    header[i].dslaenge  = block[6*i+4];
    header[i].keylaenge = block[6*i+5];
  }
}


void DBB::add_ds(int kennung,void *quelle) {
 HEADER *h = &header[kennung];
	// append record if there is space for it
	if ((dbcursor + h->dslaenge) < DBBEnd) {
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


void DBB::add_fdf(int feldkennung,int feldlaenge, void *quelle) {
	if ((fdfcursor+feldlaenge+2) < FrmEnd) {
		fdf[fdfcursor] = feldlaenge + 2;
		fdf[fdfcursor+1] = feldkennung;
		memcpy(&fdf[fdfcursor+2],quelle, feldlaenge);
		fdfcursor += (feldlaenge + 2);
	}
}

// find an actual record of specified type(id) in the declaration memory
// and return it's position in the memory array
//
int16 DBB::fdf_findfield(byte id) {
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

