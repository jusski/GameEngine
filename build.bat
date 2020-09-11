@echo off
REM if not exist e:\build mkdir e:\build
cd /D %~dp0
pushd build

set CFLAGS=-nologo  -FC -WX -W4 -wd4201 -wd4514 -wd4100 -wd4505 -wd4189^
           -Oi -Od -GR- -Gm- -EHa- -Zi -MDd /std:c++17 
set LFLAGS=/PDB:%random%.pdb /DEBUG 
set EXPORTS=
SET IMPORTS=user32.lib gdi32.lib dsound.lib winmm.lib opengl32.lib

del *.pdb
cl %CFLAGS% /LD ..\code\game.cpp  /link %LFLAGS% /INCREMENTAL:NO /EXPORT:GameEngine
REM cl %CFLAGS% ..\code\win32_main.cpp  /link /DEBUG /INCREMENTAL:NO %IMPORTS%
popd
