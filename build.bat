@REM @echo off
@REM powershell -NoProfile -Command "$s = Get-Date; cmake --build build; $e = Get-Date; Write-Host ('{0:N2}s' -f ($e - $s).TotalSeconds) -ForegroundColor Green"

set NINJA_STATUS=[%%e s] 
cmake --build build