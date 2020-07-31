@echo off

set Wildcard=*.h *.cpp

echo Statics found:
findstr -s -n -i -r "static" %Wildcard%

echo Globals found:
findstr -s -n -i -r "global_variable" %Wildcard%
