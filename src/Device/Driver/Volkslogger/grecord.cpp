/***********************************************************************
**
**   grecord.cpp
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

#include "grecord.h"

#include <stdio.h>
#include <string.h>

/* base-64 functions */
static const char *
byte_bas64(const uint8_t *b)
{
 static constexpr char base64tab[] =
   "0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ`abcdefghijklmnopqrstuvwxyz";
 static char bas64ar[5];
  bas64ar[0] = base64tab[   b[0] >> 2                           ];
  bas64ar[1] = base64tab[ ((b[0] & 0x03) << 4) | (b[1] >> 4)    ];
  bas64ar[2] = base64tab[ ((b[1] & 0x0f) << 2) | (b[2] >> 6)    ];
  bas64ar[3] = base64tab[   b[2] & 0x3f                         ];
  bas64ar[4] = 0;
  return bas64ar;
}

/* g-record functions */
GRECORD::GRECORD(FILE *ausgabedatei) {
  strcpy(grecord, "");
  tricnt = 0;
  gcnt   = 0;
  memset(ba,0xff,3);
  ausgabe = ausgabedatei;
}

void
GRECORD::update(uint8_t b)
{
  ba[tricnt++] = b;
  if (tricnt == 3) {
  tricnt = 0;
  strcat(grecord,byte_bas64(ba));
  memset(ba,0xff,3);
  gcnt++;
    if (gcnt == 18) {
      gcnt = 0;
      fprintf(ausgabe,"G%s\n",grecord);
      strcpy(grecord, "");
    }
  }
}

void
GRECORD::finish()
{
  /* ba[] is 0xff after a complete triplet.  Those bytes are padding
     for a short tail, so encode them only when a byte is left over. */
  if (tricnt != 0)
    strcat(grecord, byte_bas64(ba));

  if (tricnt != 0 || gcnt != 0)
    fprintf(ausgabe, "G%s\n", grecord);
}

/*
 * DATA-GCS:
 *   - Binärblock beim Logger anfordern und im Speicher ablegen
 *   * Binärblock ins IGC-Format konvertieren
 *   * IGC-Datei abspeichern
 *   - Binärblock im radix-64-Format als G-Records an die IGC-Datei
 *     anhängen
 *
 * VALI-GCS:
 *   - IGC-Datei laden und ohne die nicht vom Logger stammenden
 *     Datensätze und Whitespaces in temp1.igc abspeichern
 *   - G-Records aus der IGC-Datei laden und von radix-64 in einen
 *     Binärblock umwandeln
 *   * Binärblock ins IGC-Format konvertieren und in temp2.igc speichern
 *   - Sicherheitscheck: temp1 und temp2 vergleichen, Signatur prüfen
 *
 * Ein * markiert einen Schritt für das DATA- und das VALI-Programm.
 *
 * Benötigte Funktionen (D = DATA, V = VALI, P = schon programmiert):
 *
 *   D V P
 *   x   x  Verzeichnis der Flüge auslesen
 *   x   x  Binärblock (Flug) vom Logger lesen
 *   x x    Binärblock ins IGC-Format konvertieren und die IGC-Datei
 *          speichern
 *   x      Dateiname nach IGC-Vorschrift generieren
 *   x x    Datei kopieren
 *     x    Signatur im Binärblock überprüfen
 *   x   x  Binärblock in GR64 konvertieren und anhängen
 *     x    GR64 laden, in einen Binärblock umwandeln und ablegen
 *          IGC-Datei laden, loggerfremde Datensätze ausfiltern und als
 *          temporäre Datei speichern
 */

void
print_g_record(FILE *datei, const uint8_t *puffer, size_t puflen)
{
  GRECORD g1(datei);
  for (size_t i = 0; i < puflen; ++i)
    g1.update(puffer[i]);
  g1.finish();
}
