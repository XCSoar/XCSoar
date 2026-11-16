/***********************************************************************
**
**   grecord.h
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

#pragma once

#include <stdint.h>
#include <stdio.h>

class GRECORD {
 private:
  char grecord[80];
  int  tricnt;
  int  gcnt;
  uint8_t ba[3];
  FILE *ausgabe;
  void init(void); /* Initialisieren der Werte */

 public:
  GRECORD(FILE *ausgabedatei);
  void update(uint8_t b);
  void finish(void);

};


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
print_g_record(FILE *datei, const uint8_t *puffer, size_t puflen);
