/*
** This code is entirely based on the previous work of Frank Warmerdam. It is
** essentially shapelib 1.1.5. However, there were enough changes that it was
** incorporated into the MapServer source to avoid confusion. See the README
** for licence details.
*/

#include "map.h"

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <stdbool.h>

#ifdef ANDROID
#include <sys/endian.h>
#endif

#if UINT_MAX == 65535
typedef long          int32;
#else
typedef int           int32;
#endif

static const bool bBigEndian = BYTE_ORDER == BIG_ENDIAN;

/************************************************************************/
/*                              SwapWord()                              */
/*                                                                      */
/*      Swap a 2, 4 or 8 byte word.                                     */
/************************************************************************/
static void SwapWord( int length, void * wordP )
{
  int i;
  uchar	temp;

  for( i=0; i < length/2; i++ )
    {
      temp = ((uchar *) wordP)[i];
      ((uchar *)wordP)[i] = ((uchar *) wordP)[length-i-1];
      ((uchar *) wordP)[length-i-1] = temp;
    }
}

/************************************************************************/
/*                             SfRealloc()                              */
/*                                                                      */
/*      A realloc cover function that will access a NULL pointer as     */
/*      a valid input.                                                  */
/************************************************************************/
static void *
SfRealloc(void *pMem, size_t nNewSize)
{
  return realloc(pMem, nNewSize);
}

/************************************************************************/
/*                              msSHPOpen()                             */
/*                                                                      */
/*      Open the .shp and .shx files based on the basename of the       */
/*      files or either file name.                                      */
/************************************************************************/
SHPHandle msSHPOpen( const char * pszLayer, const char * pszAccess )
{
  char		*pszFullname, *pszBasename;
  SHPHandle	psSHP;

  uchar		*pabyBuf;
  int		i;
  double	dValue;

  /* -------------------------------------------------------------------- */
  /*      Ensure the access string is one of the legal ones.  We          */
  /*      ensure the result string indicates binary to avoid common       */
  /*      problems on Windows.                                            */
  /* -------------------------------------------------------------------- */
  if( strcmp(pszAccess,"rb+") == 0
      || strcmp(pszAccess,"r+b") == 0
      || strcmp(pszAccess,"r+") == 0 )
    pszAccess = "r+b";
  else
    pszAccess = "rb";

  /* -------------------------------------------------------------------- */
  /*	Initialize the info structure.					    */
  /* -------------------------------------------------------------------- */
  psSHP = (SHPHandle) malloc(sizeof(SHPInfo));

  psSHP->pabyRec = NULL;
  psSHP->panParts = NULL;
  psSHP->nBufSize = psSHP->nPartMax = 0;

  /* -------------------------------------------------------------------- */
  /*	Compute the base (layer) name.  If there is any extension	    */
  /*	on the passed in filename we will strip it off.			    */
  /* -------------------------------------------------------------------- */
  pszBasename = (char *) malloc(strlen(pszLayer)+5);
  strcpy( pszBasename, pszLayer );
  for( i = strlen(pszBasename)-1;
       i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'
	 && pszBasename[i] != '\\';
       i-- ) {}

  if( pszBasename[i] == '.' )
    pszBasename[i] = '\0';

  /* -------------------------------------------------------------------- */
  /*	Open the .shp and .shx files.  Note that files pulled from	    */
  /*	a PC to Unix with upper case filenames won't work!		    */
  /* -------------------------------------------------------------------- */
  pszFullname = (char *) malloc(strlen(pszBasename) + 5);
  sprintf( pszFullname, "%s.shp", pszBasename );
  psSHP->zfpSHP = zzip_fopen(pszFullname, pszAccess );
  psSHP->fpSHP = NULL;
  if( psSHP->zfpSHP == NULL ) {
    free(psSHP);
    free(pszFullname);
    return( NULL );
  }

  sprintf( pszFullname, "%s.shx", pszBasename );
  psSHP->zfpSHX = zzip_fopen(pszFullname, pszAccess );
  psSHP->fpSHX = NULL;
  if( psSHP->zfpSHX == NULL ) {
    free(psSHP);
    free(pszFullname);
    return( NULL );
  }

  free( pszFullname );
  free( pszBasename );


  /* -------------------------------------------------------------------- */
  /*   Read the file size from the SHP file.				    */
  /* -------------------------------------------------------------------- */
  pabyBuf = (uchar *) malloc(100);
  if (pabyBuf == 0) {
    free(psSHP);
    return NULL;
  }
  if (zzip_fread( pabyBuf, 1, 100, psSHP->zfpSHP )<100) {
    free(psSHP);
    return NULL;
  }

  psSHP->nFileSize = (pabyBuf[24] * 256 * 256 * 256
		      + pabyBuf[25] * 256 * 256
		      + pabyBuf[26] * 256
		      + pabyBuf[27]) * 2;

  /* -------------------------------------------------------------------- */
  /*  Read SHX file Header info                                           */
  /* -------------------------------------------------------------------- */
  zzip_fread( pabyBuf, 100, 1, psSHP->zfpSHX );

  if( pabyBuf[0] != 0
      || pabyBuf[1] != 0
      || pabyBuf[2] != 0x27
      || (pabyBuf[3] != 0x0a && pabyBuf[3] != 0x0d) )
    {
      zzip_fclose( psSHP->zfpSHP );
      zzip_fclose( psSHP->zfpSHX );
      free( psSHP );

      return( NULL );
    }

  psSHP->nRecords = pabyBuf[27] + pabyBuf[26] * 256
    + pabyBuf[25] * 256 * 256 + pabyBuf[24] * 256 * 256 * 256;
  psSHP->nRecords = (psSHP->nRecords*2 - 100) / 8;

  psSHP->nShapeType = pabyBuf[32];

  if( bBigEndian ) SwapWord( 8, pabyBuf+36 );
  memcpy( &dValue, pabyBuf+36, 8 );
  psSHP->adBoundsMin[0] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+44 );
  memcpy( &dValue, pabyBuf+44, 8 );
  psSHP->adBoundsMin[1] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+52 );
  memcpy( &dValue, pabyBuf+52, 8 );
  psSHP->adBoundsMax[0] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+60 );
  memcpy( &dValue, pabyBuf+60, 8 );
  psSHP->adBoundsMax[1] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+84 );		/* m */
  memcpy( &dValue, pabyBuf+84, 8 );
  psSHP->adBoundsMin[3] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+92 );
  memcpy( &dValue, pabyBuf+92, 8 );
  psSHP->adBoundsMax[3] = dValue;
  free( pabyBuf );

  /* -------------------------------------------------------------------- */
  /*	Read the .shx file to get the offsets to each record in 	    */
  /*	the .shp file.							    */
  /* -------------------------------------------------------------------- */
  psSHP->nMaxRecords = psSHP->nRecords;

  psSHP->panRecOffset = (int *) malloc(sizeof(int) * (psSHP->nMaxRecords) );
  psSHP->panRecSize = (int *) malloc(sizeof(int) * (psSHP->nMaxRecords) );

  pabyBuf = (uchar *) malloc(8 * psSHP->nRecords );
  zzip_fread( pabyBuf, 8, psSHP->nRecords, psSHP->zfpSHX );

  for( i = 0; i < psSHP->nRecords; i++ ) {
    int32 nOffset, nLength;

    memcpy( &nOffset, pabyBuf + i * 8, 4 );
    if( !bBigEndian ) SwapWord( 4, &nOffset );

    memcpy( &nLength, pabyBuf + i * 8 + 4, 4 );
    if( !bBigEndian ) SwapWord( 4, &nLength );

    psSHP->panRecOffset[i] = nOffset*2;
    psSHP->panRecSize[i] = nLength*2;
  }
  free( pabyBuf );

  return( psSHP );
}

/************************************************************************/
/*                              msSHPClose()                            */
/*								       	*/
/*	Close the .shp and .shx files.					*/
/************************************************************************/
void msSHPClose(SHPHandle psSHP )
{
  /* -------------------------------------------------------------------- */
  /*      Free all resources, and close files.                            */
  /* -------------------------------------------------------------------- */
  free( psSHP->panRecOffset );
  free( psSHP->panRecSize );

  free(psSHP->pabyRec);
  free(psSHP->panParts);

  if (psSHP->zfpSHX)
    zzip_fclose( psSHP->zfpSHX );
  if (psSHP->zfpSHP)
    zzip_fclose( psSHP->zfpSHP );
  if (psSHP->fpSHX)
    fclose( psSHP->fpSHX );
  if (psSHP->fpSHP)
    fclose( psSHP->fpSHP );

  free( psSHP );
}

/************************************************************************/
/*                             msSHPGetInfo()                           */
/*                                                                      */
/*      Fetch general information about the shape file.                 */
/************************************************************************/
void msSHPGetInfo(SHPHandle psSHP, int * pnEntities, int * pnShapeType )
{
  if( pnEntities )
    *pnEntities = psSHP->nRecords;

  if( pnShapeType )
    *pnShapeType = psSHP->nShapeType;
}

/*
** msSHPReadShape() - Reads the vertices for one shape from a shape file.
*/
void msSHPReadShape( SHPHandle psSHP, int hEntity, shapeObj *shape )
{
    int	       		i, j, k;
    int nOffset = 0;


    msInitShape(shape); /* initialize the shape */

    /* -------------------------------------------------------------------- */
    /*      Validate the record/entity number.                              */
    /* -------------------------------------------------------------------- */
    if( hEntity < 0 || hEntity >= psSHP->nRecords )
      return;

    if( psSHP->panRecSize[hEntity] == 4 ) {
      shape->type = MS_SHAPE_NULL;
      return;
    }

    /* -------------------------------------------------------------------- */
    /*      Ensure our record buffer is large enough.                       */
    /* -------------------------------------------------------------------- */
    if( psSHP->panRecSize[hEntity]+8 > psSHP->nBufSize )
    {
	psSHP->nBufSize = psSHP->panRecSize[hEntity]+8;
	psSHP->pabyRec = (uchar *) SfRealloc(psSHP->pabyRec,psSHP->nBufSize);
    }

    /* -------------------------------------------------------------------- */
    /*      Read the record.                                                */
    /* -------------------------------------------------------------------- */
    zzip_seek( psSHP->zfpSHP, psSHP->panRecOffset[hEntity], 0 );
    zzip_fread( psSHP->pabyRec, psSHP->panRecSize[hEntity]+8, 1, psSHP->zfpSHP );

    /* -------------------------------------------------------------------- */
    /*  Extract vertices for a Polygon or Arc.				    */
    /* -------------------------------------------------------------------- */
    if( psSHP->nShapeType == SHP_POLYGON || psSHP->nShapeType == SHP_ARC ||
        psSHP->nShapeType == SHP_POLYGONM || psSHP->nShapeType == SHP_ARCM)
    {
      int32		nPoints, nParts;

      // copy the bounding box
      memcpy( &shape->bounds.minx, psSHP->pabyRec + 8 + 4, 8 );
      memcpy( &shape->bounds.miny, psSHP->pabyRec + 8 + 12, 8 );
      memcpy( &shape->bounds.maxx, psSHP->pabyRec + 8 + 20, 8 );
      memcpy( &shape->bounds.maxy, psSHP->pabyRec + 8 + 28, 8 );

      if( bBigEndian ) {
	SwapWord( 8, &shape->bounds.minx);
	SwapWord( 8, &shape->bounds.miny);
	SwapWord( 8, &shape->bounds.maxx);
	SwapWord( 8, &shape->bounds.maxy);
      }

      memcpy( &nPoints, psSHP->pabyRec + 40 + 8, 4 );
      memcpy( &nParts, psSHP->pabyRec + 36 + 8, 4 );

      if( bBigEndian ) {
	SwapWord( 4, &nPoints );
        SwapWord( 4, &nParts );
      }

      /* -------------------------------------------------------------------- */
      /*      Copy out the part array from the record.                        */
      /* -------------------------------------------------------------------- */
      if( psSHP->nPartMax < nParts )
      {
	psSHP->nPartMax = nParts;
	psSHP->panParts = (int *) SfRealloc(psSHP->panParts, psSHP->nPartMax * sizeof(int) );
      }

      memcpy( psSHP->panParts, psSHP->pabyRec + 44 + 8, 4 * nParts );
      for( i = 0; i < nParts; i++ )
	if( bBigEndian ) SwapWord( 4, psSHP->panParts+i );

      /* -------------------------------------------------------------------- */
      /*      Fill the shape structure.                                       */
      /* -------------------------------------------------------------------- */
      if( (shape->line = (lineObj *)malloc(sizeof(lineObj)*nParts)) == NULL ) {
	msSetError(MS_MEMERR, NULL, "SHPReadShape()");
	return;
      }

      shape->numlines = nParts;

      k = 0; /* overall point counter */
      for( i = 0; i < nParts; i++)
      {
	if( i == nParts-1)
	  shape->line[i].numpoints = nPoints - psSHP->panParts[i];
	else
	  shape->line[i].numpoints = psSHP->panParts[i+1] - psSHP->panParts[i];

	if( (shape->line[i].point = (pointObj *)malloc(sizeof(pointObj)*shape->line[i].numpoints)) == NULL ) {
	  free(shape->line);
	  shape->numlines = 0;
	  return;
	}

        //nOffset = 44 + 8 + 4*nParts;
	for( j = 0; j < shape->line[i].numpoints; j++ )
	{
	  memcpy(&(shape->line[i].point[j].x), psSHP->pabyRec + 44 + 4*nParts + 8 + k * 16, 8 );
	  memcpy(&(shape->line[i].point[j].y), psSHP->pabyRec + 44 + 4*nParts + 8 + k * 16 + 8, 8 );

	  if( bBigEndian ) {
	    SwapWord( 8, &(shape->line[i].point[j].x) );
	    SwapWord( 8, &(shape->line[i].point[j].y) );
	  }

          shape->line[i].point[j].m = 0; // initialize
/* -------------------------------------------------------------------- */
/*      Measured arc and polygon support.                               */
/* -------------------------------------------------------------------- */
          if (psSHP->nShapeType == SHP_POLYGONM || psSHP->nShapeType == SHP_ARCM)
          {
              nOffset = 44 + 8 + (4*nParts) + (16*nPoints) ;
              if( psSHP->panRecSize[hEntity]+8 >= nOffset + 16 + 8*nPoints )
              {
                   memcpy(&(shape->line[i].point[j].m),
                          psSHP->pabyRec + nOffset + 16 + k*8, 8 );
                   if( bBigEndian )
                       SwapWord( 8, &(shape->line[i].point[j].m) );
              }
          }
	  k++;
	}
      }

      if(psSHP->nShapeType == SHP_POLYGON || psSHP->nShapeType == SHP_POLYGONM)
        shape->type = MS_SHAPE_POLYGON;
      else
        shape->type = MS_SHAPE_LINE;

    }

    /* -------------------------------------------------------------------- */
    /*  Extract a MultiPoint.   			                    */
    /* -------------------------------------------------------------------- */
    else if( psSHP->nShapeType == SHP_MULTIPOINT || psSHP->nShapeType == SHP_MULTIPOINTM)
    {
      int32		nPoints;

      // copy the bounding box
      memcpy( &shape->bounds.minx, psSHP->pabyRec + 8 + 4, 8 );
      memcpy( &shape->bounds.miny, psSHP->pabyRec + 8 + 12, 8 );
      memcpy( &shape->bounds.maxx, psSHP->pabyRec + 8 + 20, 8 );
      memcpy( &shape->bounds.maxy, psSHP->pabyRec + 8 + 28, 8 );

      if( bBigEndian ) {
	SwapWord( 8, &shape->bounds.minx);
	SwapWord( 8, &shape->bounds.miny);
	SwapWord( 8, &shape->bounds.maxx);
	SwapWord( 8, &shape->bounds.maxy);
      }

      memcpy( &nPoints, psSHP->pabyRec + 44, 4 );
      if( bBigEndian ) SwapWord( 4, &nPoints );

      /* -------------------------------------------------------------------- */
      /*      Fill the shape structure.                                       */
      /* -------------------------------------------------------------------- */
      if( (shape->line = (lineObj *)malloc(sizeof(lineObj))) == NULL ) {
	msSetError(MS_MEMERR, NULL, "SHPReadShape()");
	return;
      }

      shape->numlines = 1;
      shape->line[0].numpoints = nPoints;
      shape->line[0].point = (pointObj *) malloc( nPoints * sizeof(pointObj) );

      for( i = 0; i < nPoints; i++ ) {
	memcpy(&(shape->line[0].point[i].x), psSHP->pabyRec + 48 + 16 * i, 8 );
	memcpy(&(shape->line[0].point[i].y), psSHP->pabyRec + 48 + 16 * i + 8, 8 );

	if( bBigEndian ) {
	  SwapWord( 8, &(shape->line[0].point[i].x) );
	  SwapWord( 8, &(shape->line[0].point[i].y) );
	}

        shape->line[0].point[i].m = 0; //initialize
/* -------------------------------------------------------------------- */
/*      Measured shape : multipont.                                     */
/* -------------------------------------------------------------------- */
        if (psSHP->nShapeType == SHP_MULTIPOINTM)
        {
            nOffset = 48 + 16*nPoints;
            memcpy(&(shape->line[0].point[i].m), psSHP->pabyRec + nOffset + 16 + i*8, 8 );
            if( bBigEndian )
              SwapWord( 8, &(shape->line[0].point[i].m));
        }
      }

      shape->type = MS_SHAPE_POINT;
    }

    /* -------------------------------------------------------------------- */
    /*  Extract a Point.   			                    */
    /* -------------------------------------------------------------------- */
    else if( psSHP->nShapeType == SHP_POINT ||  psSHP->nShapeType == SHP_POINTM)
    {

      /* -------------------------------------------------------------------- */
      /*      Fill the shape structure.                                       */
      /* -------------------------------------------------------------------- */
      if( (shape->line = (lineObj *)malloc(sizeof(lineObj))) == NULL ) {
	msSetError(MS_MEMERR, NULL, "SHPReadShape()");
	return;
      }

      shape->numlines = 1;
      shape->line[0].numpoints = 1;
      shape->line[0].point = (pointObj *) malloc(sizeof(pointObj));

      memcpy( &(shape->line[0].point[0].x), psSHP->pabyRec + 12, 8 );
      memcpy( &(shape->line[0].point[0].y), psSHP->pabyRec + 20, 8 );

      if( bBigEndian ) {
	SwapWord( 8, &(shape->line[0].point[0].x));
	SwapWord( 8, &(shape->line[0].point[0].y));
      }

      shape->line[0].point[0].m = 0; //initialize
/* -------------------------------------------------------------------- */
/*      Measured support : point.                                       */
/* -------------------------------------------------------------------- */
      if (psSHP->nShapeType == SHP_POINTM)
      {
          nOffset = 20 + 8;
          if( psSHP->panRecSize[hEntity]+8 >= nOffset + 8 )
          {
              memcpy(&(shape->line[0].point[0].m), psSHP->pabyRec + nOffset, 8 );

              if( bBigEndian )
                SwapWord( 8, &(shape->line[0].point[0].m));
          }
      }
      // set the bounding box to the point
      shape->bounds.minx = shape->bounds.maxx = shape->line[0].point[0].x;
      shape->bounds.miny = shape->bounds.maxy = shape->line[0].point[0].y;

      shape->type = MS_SHAPE_POINT;
    }

    shape->index = hEntity;
    return;
}

int msSHPReadBounds( SHPHandle psSHP, int hEntity, rectObj *padBounds)
{
  /* -------------------------------------------------------------------- */
  /*      Validate the record/entity number.                              */
  /* -------------------------------------------------------------------- */
  if( psSHP->nRecords <= 0 || hEntity < -1 || hEntity >= psSHP->nRecords ) {
    padBounds->minx = padBounds->miny = padBounds->maxx = padBounds->maxy = 0.0;
    return(-1);
  }

  /* -------------------------------------------------------------------- */
  /*	If the entity is -1 we fetch the bounds for the whole file.	  */
  /* -------------------------------------------------------------------- */
  if( hEntity == -1 ) {
    padBounds->minx = psSHP->adBoundsMin[0];
    padBounds->miny = psSHP->adBoundsMin[1];
    padBounds->maxx = psSHP->adBoundsMax[0];
    padBounds->maxy = psSHP->adBoundsMax[1];
  } else {
    if( psSHP->panRecSize[hEntity] == 4 ) { // NULL shape
      padBounds->minx = padBounds->miny = padBounds->maxx = padBounds->maxy = 0.0;
      return(-1);
    }

    if( psSHP->nShapeType != SHP_POINT ) {
      zzip_seek( psSHP->zfpSHP, psSHP->panRecOffset[hEntity]+12, 0 );
      zzip_fread( padBounds, sizeof(double)*4, 1, psSHP->zfpSHP );

      if( bBigEndian ) {
	SwapWord( 8, &(padBounds->minx) );
	SwapWord( 8, &(padBounds->miny) );
	SwapWord( 8, &(padBounds->maxx) );
	SwapWord( 8, &(padBounds->maxy) );
      }
    } else {
      /* -------------------------------------------------------------------- */
      /*      For points we fetch the point, and duplicate it as the          */
      /*      minimum and maximum bound.                                      */
      /* -------------------------------------------------------------------- */

      zzip_seek( psSHP->zfpSHP, psSHP->panRecOffset[hEntity]+12, 0 );
      zzip_fread( padBounds, sizeof(double)*2, 1, psSHP->zfpSHP );

      if( bBigEndian ) {
	SwapWord( 8, &(padBounds->minx) );
	SwapWord( 8, &(padBounds->miny) );
      }

      padBounds->maxx = padBounds->minx;
      padBounds->maxy = padBounds->miny;
    }
  }

  return(0);
}

int msSHPOpenFile(shapefileObj *shpfile, const char *mode,
                  const char *filename)
{
  int i;
  char *dbfFilename;

  if(!filename) {
    msSetError(MS_IOERR, "No (NULL) filename provided.", "msSHPOpenFile()");
    return(-1);
  }

  /* initialize a few things */
  shpfile->status = NULL;
  shpfile->lastshape = -1;

  /* open the shapefile file (appending ok) and get basic info */
  if(!mode)
    shpfile->hSHP = msSHPOpen( filename, "rb");
  else
    shpfile->hSHP = msSHPOpen( filename, mode);

  if(!shpfile->hSHP) {
    msSetError(MS_IOERR, "(%s)", "msSHPOpenFile()", filename);
    return(-1);
  }

  strcpy(shpfile->source, filename);

  /* load some information about this shapefile */
  msSHPGetInfo( shpfile->hSHP, &shpfile->numshapes, &shpfile->type);
  msSHPReadBounds( shpfile->hSHP, -1, &(shpfile->bounds));

  dbfFilename = (char *)malloc(strlen(filename)+5);
  strcpy(dbfFilename, filename);

  /* clean off any extention the filename might have */
  for (i = strlen(dbfFilename) - 1;
       i > 0 && dbfFilename[i] != '.' && dbfFilename[i] != '/' && dbfFilename[i] != '\\';
       i-- ) {}

  if( dbfFilename[i] == '.' )
    dbfFilename[i] = '\0';

  strcat(dbfFilename, ".dbf");

  shpfile->hDBF = msDBFOpen(dbfFilename, "rb");

  if(!shpfile->hDBF) {
    msSetError(MS_IOERR, "(%s)", "msSHPOpenFile()", dbfFilename);
    free(dbfFilename);
    return(-1);
  }
  free(dbfFilename);

  return(0); /* all o.k. */
}

void msSHPCloseFile(shapefileObj *shpfile)
{
  if (shpfile) { // Silently return if called with NULL shpfile by freeLayer()
    if(shpfile->hSHP) msSHPClose(shpfile->hSHP);
    if(shpfile->hDBF) msDBFClose(shpfile->hDBF);
    free(shpfile->status);
  }
}

// status array lives in the shpfile, can return MS_SUCCESS/MS_FAILURE/MS_DONE
int msSHPWhichShapes(shapefileObj *shpfile, rectObj rect, int debug)
{
  int i;
  rectObj shaperect;
  char *filename;

  free(shpfile->status);
  shpfile->status = NULL;

  shpfile->statusbounds = rect; // save the search extent

  // rect and shapefile DON'T overlap...
  if(msRectOverlap(&shpfile->bounds, &rect) != MS_TRUE)
    return(MS_DONE);

  if(msRectContained(&shpfile->bounds, &rect) == MS_TRUE) {
    shpfile->status = msAllocBitArray(shpfile->numshapes);
    if(!shpfile->status) {
      msSetError(MS_MEMERR, NULL, "msSHPWhichShapes()");
      return(MS_FAILURE);
    }
    for(i=0;i<shpfile->numshapes;i++)
      msSetBit(shpfile->status, i, 1);
  } else {
    if((filename = (char *)malloc(strlen(shpfile->source)+strlen(MS_INDEX_EXTENSION)+1)) == NULL) {
      msSetError(MS_MEMERR, NULL, "msSHPWhichShapes()");
      return(MS_FAILURE);
    }
    sprintf(filename, "%s%s", shpfile->source, MS_INDEX_EXTENSION);

    shpfile->status = msSearchDiskTree(filename, rect, debug);
    free(filename);

    if(shpfile->status) // index
      msFilterTreeSearch(shpfile, shpfile->status, rect);
    else { // no index
      shpfile->status = msAllocBitArray(shpfile->numshapes);
      if(!shpfile->status) {
	msSetError(MS_MEMERR, NULL, "msSHPWhichShapes()");
	return(MS_FAILURE);
      }

      for(i=0;i<shpfile->numshapes;i++) {
	if(!msSHPReadBounds(shpfile->hSHP, i, &shaperect))
	  if(msRectOverlap(&shaperect, &rect) == MS_TRUE)
	    msSetBit(shpfile->status, i, 1);
      }
    }
  }

  shpfile->lastshape = -1;

  return(MS_SUCCESS); /* success */
}

