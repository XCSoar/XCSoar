//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("XCSoarPC.res");
USEFORM("Unit1.cpp", Form1);
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\XCSoar.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Airspace.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\AirspaceColourDlg.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Calculations.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\devCAI302.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\devEW.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\device.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Dialogs.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Logger.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\mapbits.c");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\maperror.c");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\mapprimitive.c");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\mapsearch.c");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\mapshape.c");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\maptree.c");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\MapWindow.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\mapxbase.c");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\McReady.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Parser.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Port.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Port2.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Process.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\RasterTerrain.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\rscalc.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\StdAfx.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\STScreenBuffer.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Terrain.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Topology.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Utils.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\VarioSound.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\VOIMAGE.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\WaveThread.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\Waypointparser.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\windanalyser.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\windmeasurementlist.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\windstore.cpp");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\AirfieldDetails.cpp");
USERC("C:\Project\xcsoarMy\source\Common\Source\XCSoar.rc");
USEUNIT("C:\Project\xcsoarMy\source\Common\Source\units.cpp");
USEUNIT("modDummies.cpp");
//---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
int WINAPI xcsoarWinMain(     HINSTANCE hInstance,
                        HINSTANCE hPrevInstance,
                        LPTSTR    lpCmdLine,
                        int       nCmdShow);

#ifdef __cplusplus
}
#endif


WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{

  xcsoarWinMain(hInstance, hPrevInstance, NULL, nShowCmd);

  return (0);

  try
  {
   Application->Initialize();
   Application->CreateForm(__classid(TForm1), &Form1);
   Application->Run();
  }
  catch (Exception &exception)
  {
   Application->ShowException(&exception);
  }
  return 0;
}



//---------------------------------------------------------------------------
