@echo off
setlocal

set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEVCMD%" (
    echo Visual Studio C build tools were not found.
    exit /b 2
)

call "%VSDEVCMD%" -arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%

if not exist "build\nlx_host" mkdir "build\nlx_host"

cl /nologo /W4 /WX /std:c11 ^
  /Fo:build\nlx_host\ ^
  /I applications_user\nlx_credential_suite\core ^
  /I applications_user\nlx_credential_suite\handoff ^
  applications_user\nlx_credential_suite\tests\host\test_nlx_core.c ^
  applications_user\nlx_credential_suite\core\nlx_credential_result.c ^
  applications_user\nlx_credential_suite\core\nlx_format_decoder.c ^
  applications_user\nlx_credential_suite\core\nlx_report.c ^
  applications_user\nlx_credential_suite\core\nlx_risk_assessment.c ^
  applications_user\nlx_credential_suite\core\nlx_scan_schedule.c ^
  applications_user\nlx_credential_suite\handoff\nlx_tool_registry.c ^
  /Fe:build\nlx_host\nlx_credential_suite_tests.exe
if errorlevel 1 exit /b %errorlevel%

build\nlx_host\nlx_credential_suite_tests.exe
exit /b %errorlevel%
