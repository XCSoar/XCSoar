@echo on 

REM This is an example how the manuals get generated on Windows with MikTeX (2.9)

REM Set the full path to executable pdflatex, or put its installation path to the %PATH% env instead.
set PDFLATEXBIN="pdflatex"

cd %~dp0
mkdir output

REM Comment out or in whatever your focus is on.

REM  English XCSoar-manual generation
%PDFLATEXBIN% --halt-on-error --interaction=nonstopmode --include-directory=%~dp0en --include-directory=%~dp0 --output-directory=%~dp0output  %~dp0en\XCSoar-manual.tex

REM  English XCSoar-developer-manual generation
%PDFLATEXBIN% --halt-on-error --interaction=nonstopmode --include-directory=%~dp0en  --include-directory=%~dp0 --output-directory=%~dp0output  %~dp0en\XCSoar-developer-manual.tex

REM  German XCSoar-Blitzeinstieg generation
%PDFLATEXBIN% --halt-on-error --interaction=nonstopmode --include-directory=%~dp0de\Blitz --include-directory=%~dp0de\Blitz\Bilder --include-directory=%~dp0 --output-directory=%~dp0output  %~dp0de\Blitz\XCSoar-Blitzeinstieg.tex

REM  German XCSoar-Handbuch generation
%PDFLATEXBIN% --halt-on-error --interaction=nonstopmode --include-directory=%~dp0de --include-directory=%~dp0 --include-directory=%~dp0en --output-directory=%~dp0output  %~dp0de\XCSoar-manual-de.tex

REM  french XCSoar-Prise-en-main generation
%PDFLATEXBIN% --halt-on-error --interaction=nonstopmode --include-directory=%~dp0fr  --include-directory=%~dp0 --output-directory=%~dp0output  %~dp0fr\XCSoar-Prise-en-main.tex 

REM  french XCSoar-manual-fr generation
%PDFLATEXBIN% --halt-on-error --interaction=nonstopmode --include-directory=%~dp0fr --include-directory=%~dp0 --include-directory=%~dp0en --output-directory=%~dp0output  %~dp0fr\XCSoar-manual-fr.tex

