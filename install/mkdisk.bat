@echo off
rem AR-Optionen
rem ===========
rem A - Add files to archive
rem S - Save files to archive without compression
rem X - Extract
rem R - Replace
rem D - Delete
rem P - Print
rem L - List contents
echo Make Disk DFCGEN V1.00
echo ----------------------

copy setup.cfg disk\setup.cfg
copy uninstal.cfg disk\uninstal.cfg
copy c:\yawpi\install.exe disk\setup.exe
copy c:\yawpi\uninstal.exe disk\uninstal.exe
copy ..\src\dfcgen.exe disk\dfcgen.exe
copy ..\doc\dfcgen.hlp disk\dfcgen.hlp
copy german.txt disk\german.txt
copy readme.txt disk\readme.txt
copy order.txt disk\order.txt
copy ..\src\bwcc.dll disk\bwcc.dll
copy dfcgen.ini disk\dfcgen.ini

cd disk

c:\yawpi\disk\ar.exe a dfcgen.ar dfcgen.exe dfcgen.hlp bwcc.dll
c:\yawpi\disk\ar.exe a dfcgen.ar readme.txt order.txt
c:\yawpi\disk\ar.exe a dfcgen.ar uninstal.exe
c:\yawpi\disk\ar.exe a dfcwin.ar bwcc.dll dfcgen.ini

del uninstal.exe
del dfcgen.exe
del dfcgen.hlp
del readme.txt
del order.txt
del bwcc.dll
del dfcgen.ini

echo Inhalt von DFCGEN.AR
echo --------------------
c:\yawpi\disk\ar.exe l dfcgen.ar
echo Inhalt von DFCWIN.AR
echo --------------------
c:\yawpi\disk\ar.exe l dfcwin.ar

echo Seal Editor nicht vergessen ???
echo -------------------------------
echo FERTIG.
cd ..
pause
