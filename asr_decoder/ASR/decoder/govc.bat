@echo off
del /Q *.exe
REM cl /c /O2 /Oi /Ot /EHsc /GA /arch:SSE2 *.cpp
REM cl /c /O2 /Oi /Ot /EHsc /GA /arch:SSE2 /DASR_SSE2 *.cpp
cl /c /O2 /Oi /Ot /EHsc /GA /DASR_GENERIC *.cpp

link /OUT:recog.exe *.obj

del /Q *.obj

echo === decoder float to fixed ===
codeconvert . ..\decoderFixed
