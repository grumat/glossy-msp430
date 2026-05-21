@echo off
REM Run this file to build the project outside of the IDE.
REM WARNING: if using a different machine, copy the .rsp files together with this script.
echo LoadRegs.s
C:\ti\msp430-gcc\bin\msp430-elf-g++.exe @"VisualGDB/Release/LoadRegs.gcc.rsp" || exit 1
echo Linking ../../VisualGDB/Release/LoadRegs.elf...
C:\ti\msp430-gcc\bin\msp430-elf-g++.exe @../../VisualGDB/Release/LoadRegs.link.rsp || exit 1
C:\ti\msp430-gcc\bin\msp430-elf-objcopy.exe @../../VisualGDB/Release/LoadRegs.mkbin.rsp || exit 1
