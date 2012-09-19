@echo off
cd ..

echo Creating winsrc dir
mkdir winsrc > nul:

echo Copying Source files to winsrc dir
copy /Y .\src\*.c .\winsrc\*.cpp > nul:
copy /Y .\win32\*.cpp .\winsrc\*.cpp > nul:
copy /Y .\include\*.h .\winsrc\*.h > nul:
copy /Y .\win32\*.h .\winsrc\*.h > nul:

echo Copying makefile
copy /Y .\win32\makefile.win .\makefile > nul:

echo Making compile directory...
mkdir compile > nul:

echo Remember to setup your VCVARS before typing 'nmake'
rem C:/PROGRA~1/DEVSTU~1/VC/BIN/VCVARS32.BAT
