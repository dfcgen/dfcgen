@echo off

rem Shows the contents of dfcgen.ar and dfcwin.ar
rem =============================================

rem AR-Optionen
rem -----------
rem A - Add files to archive
rem S - Save files to archive without compression
rem X - Extract
rem R - Replace
rem D - Delete
rem P - Print
rem L - List contents

cd disk
echo Inhalt von DFCGEN.AR
echo --------------------
c:\yawpi\disk\ar.exe l dfcgen.ar
echo Inhalt von DFCWIN.AR
echo --------------------
c:\yawpi\disk\ar.exe l dfcwin.ar

cd ..
pause
