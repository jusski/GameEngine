@echo off
pushd
cd %~d0\code\
"C:\Program Files (x86)\emacs-26.3-x86_64\bin\runemacs.exe" -q -l %~d0\misc\.emacs
popd
