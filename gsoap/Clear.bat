@echo off
for /f "delims=" %%i in ('dir /a-d /b') do (
	if /i "%%i" neq "xml-rpc.h" if /i "%%i" neq "xml-rpc.c" if /i "%%i" neq "json.h" if /i "%%i" neq "json.c" if /i "%%i" neq "stdsoap2.h" if /i "%%i" neq "stdsoap2.c" if /i "%%i" neq "xml-rpc-iters.h" if /i "%%i" neq "Server.bat"  if /i "%%i" neq "Clear.bat" (
		del %%i
	)
)