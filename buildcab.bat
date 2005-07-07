
"C:\Program Files\Windows CE Tools\wce420\POCKET PC 2003\Tools\Cabwiz.exe" XCSoarData.inf

ezsetup -l english -i XCSoarData.ini -r installmsgdata.txt -e gpl.txt -o InstallXCSoar-Data.exe 


pause


"C:\Program Files\Windows CE Tools\wce420\POCKET PC 2003\Tools\Cabwiz.exe" XCSoar.inf /cpu armv4

ezsetup -l english -i XCSoar.ini -r installmsg.txt -e gpl.txt -o InstallXCSoar-ARM.exe

pause

