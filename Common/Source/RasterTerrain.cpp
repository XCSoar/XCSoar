
#include "RasterTerrain.h"
#include "XCSoar.h"

// JMW added cacheing of results of terrain lookup to reduce file IO


void RasterTerrain::SetCacheTime() {
  terraincachehits = 1;
  terraincachemisses = 1;
  cachetime++;
}


void RasterTerrain::ClearTerrainCache() {
  int i;
  for (i=0; i<MAXTERRAINCACHE; i++) {
    TerrainCache[i].index= -1;
    TerrainCache[i].recency= 0;
    TerrainCache[i].h= 0;
  }
}


short RasterTerrain::LookupTerrainCache(long SeekPos) {
  int ifound= -1;
  unsigned int recencymin = 0;
  int i;
  _TERRAIN_CACHE* tcp, *tcpmin;

  terraincacheefficiency = (100*terraincachehits)/(terraincachehits+terraincachemisses);

  // search to see if it is found in the cache

  tcp = TerrainCache;
  for (i=0; i<MAXTERRAINCACHE; i++) {
    if (tcp->index == SeekPos) {
      tcp->recency = cachetime;
      i=MAXTERRAINCACHE;
      terraincachehits++;
      return tcp->h;
    }
    tcp++;
  }

  // if not found..
  terraincachemisses++;

  tcpmin = TerrainCache; recencymin = tcpmin->recency;
  for (tcp=TerrainCache; tcp<TerrainCache+MAXTERRAINCACHE; tcp++) {
    if (tcp->recency < recencymin) {
      tcpmin = tcp;
      recencymin = tcp->recency;
    }
  }

  // put new value in slot tcpmin

  __int16 NewAlt = 0;
  DWORD dwBytesRead;
  DWORD SeekRes, dwError;
  short Alt;

  SeekRes = SetFilePointer(hTerrain,SeekPos,NULL,FILE_BEGIN);
  if(SeekRes == 0xFFFFFFFF && (dwError = GetLastError()) != NO_ERROR ) {
    // error, not found!
    Alt = -1;
  } else {
    ReadFile(hTerrain,&NewAlt,sizeof(__int16),&dwBytesRead,NULL);
    Alt = NewAlt;
    if(Alt<0) Alt = 0;
  }
  tcpmin->recency = cachetime;
  tcpmin->h = Alt;
  tcpmin->index = SeekPos;

  return Alt;
}

float RasterTerrain::GetTerrainSlopeStep() {
    float kpixel = (float)256.0/(
			  GetTerrainStepSize()
                          * (float)rounding
                          * (float)2.0);
    return kpixel;
}



float RasterTerrain::GetTerrainStepSize() {
  // this is approximate of course..
  return (float)(250.0/0.0025*TerrainInfo.StepSize);
}


void RasterTerrain::SetTerrainRounding(double dist) {
  rounding = iround(dist/(GetTerrainStepSize()/1000.0));
  if (rounding<1) {
    rounding = 1;
  }
}


// JMW rounding further reduces data as required to speed up terrain display on
// low zoom levels


short RasterTerrain::GetTerrainHeight(double Lattitude,
				      double Longditude)
{
  long SeekPos;
  double X,Y;
  long lx, ly;

  if(hTerrain == NULL)
    return 0;

  if ((Lattitude > TerrainInfo.Top )||
      (Lattitude < TerrainInfo.Bottom )||
      (Longditude < TerrainInfo.Left )||
      (Longditude > TerrainInfo.Right )) {
    return -1;
  }

  X =  Longditude -TerrainInfo.Left;
  X = X / TerrainInfo.StepSize ;

  lx = lround(X/rounding)*rounding;

  Y = TerrainInfo.Top  - Lattitude ;
  Y = Y / TerrainInfo.StepSize ;

  if ((Y<0)||(X<0)) {
    return 0;
  }

  ly = lround(Y/rounding)*rounding;

  ly *= TerrainInfo.Columns;
  ly +=  lx;

  SeekPos = ly;
  SeekPos *= 2;
  SeekPos += sizeof(TERRAIN_INFO);

  ////// JMW added terrain cache lookup
  short h = LookupTerrainCache(SeekPos);
  return h;
}




extern TCHAR szRegistryTerrainFile[];

void RasterTerrain::OpenTerrain(void)
{
  DWORD dwBytesRead;
  static TCHAR  szFile[MAX_PATH] = TEXT("\0");

  GetRegistryString(szRegistryTerrainFile, szFile, MAX_PATH);

  hTerrain = NULL;
  hTerrain = CreateFile(szFile,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if( hTerrain == NULL)
    {
      return;
    }
  ReadFile(hTerrain,&TerrainInfo,sizeof(TERRAIN_INFO),&dwBytesRead,NULL);

  ClearTerrainCache();
}



void RasterTerrain::CloseTerrain(void)
{
  if( hTerrain == NULL)
    {
      return;
    }
  else
    {
      CloseHandle(hTerrain);
    }
}
