@echo off
set BOOST_ROOT=C:\Libraries\boost_1_69_0
if not exist %BOOST_ROOT% (
  PowerShell -file "install_boost_1.69.0_vs2015.ps1"
)
del CMakeCache.txt > nul 2>&1
cmake -G"Visual Studio 14 2015" -A Win32 .
msbuild bcDec.sln /verbosity:minimal -p:Configuration=Release