//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USERES("XCSoarPC.res");
USEFORM("Unit1.cpp", Form1);
USEUNIT("..\..\Common\Source\XCSoar.cpp");
USEUNIT("..\..\Common\Source\Airspace.cpp");
USEUNIT("..\..\Common\Source\AirspaceColourDlg.cpp");
USEUNIT("..\..\Common\Source\Calculations.cpp");
USEUNIT("..\..\Common\Source\devCAI302.cpp");
USEUNIT("..\..\Common\Source\devEW.cpp");
USEUNIT("..\..\Common\Source\device.cpp");
USEUNIT("..\..\Common\Source\Dialogs.cpp");
USEUNIT("..\..\Common\Source\Logger.cpp");
USEUNIT("..\..\Common\Source\mapbits.c");
USEUNIT("..\..\Common\Source\maperror.c");
USEUNIT("..\..\Common\Source\mapprimitive.c");
USEUNIT("..\..\Common\Source\mapsearch.c");
USEUNIT("..\..\Common\Source\mapshape.c");
USEUNIT("..\..\Common\Source\maptree.c");
USEUNIT("..\..\Common\Source\MapWindow.cpp");
USEUNIT("..\..\Common\Source\mapxbase.c");
USEUNIT("..\..\Common\Source\McReady.cpp");
USEUNIT("..\..\Common\Source\Parser.cpp");
USEUNIT("..\..\Common\Source\Port.cpp");
USEUNIT("..\..\Common\Source\Port2.cpp");
USEUNIT("..\..\Common\Source\Process.cpp");
USEUNIT("..\..\Common\Source\RasterTerrain.cpp");
USEUNIT("..\..\Common\Source\rscalc.cpp");
USEUNIT("..\..\Common\Source\StdAfx.cpp");
USEUNIT("..\..\Common\Source\STScreenBuffer.cpp");
USEUNIT("..\..\Common\Source\Terrain.cpp");
USEUNIT("..\..\Common\Source\Topology.cpp");
USEUNIT("..\..\Common\Source\Utils.cpp");
USEUNIT("..\..\Common\Source\VarioSound.cpp");
USEUNIT("..\..\Common\Source\VOIMAGE.cpp");
USEUNIT("..\..\Common\Source\WaveThread.cpp");
USEUNIT("..\..\Common\Source\Waypointparser.cpp");
USEUNIT("..\..\Common\Source\windanalyser.cpp");
USEUNIT("..\..\Common\Source\windmeasurementlist.cpp");
USEUNIT("..\..\Common\Source\windstore.cpp");
USEUNIT("..\..\Common\Source\AirfieldDetails.cpp");
USERC("..\..\Common\Source\XCSoar.rc");
USEUNIT("..\..\Common\Source\units.cpp");
USEUNIT("modDummies.cpp");
USEUNIT("..\..\Common\Source\Statistics.cpp");
USEUNIT("..\..\Common\Source\GaugeVarioAltA.cpp");
USEUNIT("..\..\Common\Source\InfoBoxLayout.cpp");
USEUNIT("..\..\Common\Source\InputEvents.cpp");
USEUNIT("..\..\Common\Source\leastsqs.cpp");
USEUNIT("..\..\Common\Source\GaugeCDI.cpp");
USEUNIT("..\..\Common\Source\Task.cpp");
USEUNIT("..\..\Common\Source\Message.cpp");
USEUNIT("..\..\Common\Source\InfoBox.cpp");
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
