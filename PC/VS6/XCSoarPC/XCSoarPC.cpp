// XCSoarPC.cpp : Defines the entry point for the application.
//

#include "stdafx.h"


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


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nShowCmd)
{
 	// TODO: Place code here.
  (void)lpCmdLine;
  xcsoarWinMain(hInstance, hPrevInstance, NULL, nShowCmd);

  /*
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
  */
  return 0;

	return 0;
}



