@echo off
setlocal enabledelayedexpansion

:: Check if Doxygen is installed
where doxygen >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Doxygen is not installed. Please install it first.
    exit /b 1
)

:: Create docs/doxygen directory if it doesn't exist
if not exist docs\doxygen mkdir docs\doxygen

:: Generate Doxyfile if it doesn't exist
if not exist docs\Doxyfile (
    echo Generating Doxyfile...
    doxygen -g docs\Doxyfile
    
    :: Update Doxyfile with project settings
    powershell -Command "(Get-Content docs\Doxyfile) -replace 'PROJECT_NAME           = \"My Project\"', 'PROJECT_NAME           = \"LuminLogger\"' | Set-Content docs\Doxyfile"
    powershell -Command "(Get-Content docs\Doxyfile) -replace 'PROJECT_BRIEF          =', 'PROJECT_BRIEF          = \"A modern C++ logging library\"' | Set-Content docs\Doxyfile"
    powershell -Command "(Get-Content docs\Doxyfile) -replace 'OUTPUT_DIRECTORY       =', 'OUTPUT_DIRECTORY       = docs/doxygen' | Set-Content docs\Doxyfile"
    powershell -Command "(Get-Content docs\Doxyfile) -replace 'EXTRACT_ALL            = NO', 'EXTRACT_ALL            = YES' | Set-Content docs\Doxyfile"
    powershell -Command "(Get-Content docs\Doxyfile) -replace 'EXTRACT_PRIVATE        = NO', 'EXTRACT_PRIVATE        = YES' | Set-Content docs\Doxyfile"
    powershell -Command "(Get-Content docs\Doxyfile) -replace 'EXTRACT_STATIC         = NO', 'EXTRACT_STATIC         = YES' | Set-Content docs\Doxyfile"
    powershell -Command "(Get-Content docs\Doxyfile) -replace 'INPUT                  =', 'INPUT                  = include src' | Set-Content docs\Doxyfile"
    powershell -Command "(Get-Content docs\Doxyfile) -replace 'RECURSIVE              = NO', 'RECURSIVE              = YES' | Set-Content docs\Doxyfile"
    powershell -Command "(Get-Content docs\Doxyfile) -replace 'GENERATE_HTML          = NO', 'GENERATE_HTML          = YES' | Set-Content docs\Doxyfile"
    powershell -Command "(Get-Content docs\Doxyfile) -replace 'GENERATE_LATEX         = YES', 'GENERATE_LATEX         = NO' | Set-Content docs\Doxyfile"
    powershell -Command "(Get-Content docs\Doxyfile) -replace 'USE_MDFILE_AS_MAINPAGE =', 'USE_MDFILE_AS_MAINPAGE = README.md' | Set-Content docs\Doxyfile"
)

:: Run Doxygen
echo Generating documentation...
doxygen docs\Doxyfile

echo.
echo Documentation generated successfully in docs\doxygen\html\
echo. 