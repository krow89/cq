@echo off
REM Installation script for cq on Windows
REM Requires MSVC (Visual Studio Build Tools)

echo Installing cq for Windows...

REM Create directories
if not exist obj mkdir obj
if not exist build mkdir build

REM Compile source files
echo Compiling csv_reader.c...
cl.exe /W3 /O2 /Iinclude /c src\csv_reader.c /Foobj\csv_reader.obj || goto :error

echo Compiling evaluator.c...
cl.exe /W3 /O2 /Iinclude /c src\evaluator.c /Foobj\evaluator.obj || goto :error

echo Compiling main.c...
cl.exe /W3 /O2 /Iinclude /c src\main.c /Foobj\main.obj || goto :error

echo Compiling parser.c...
cl.exe /W3 /O2 /Iinclude /c src\parser.c /Foobj\parser.obj || goto :error

echo Compiling tokenizer.c...
cl.exe /W3 /O2 /Iinclude /c src\tokenizer.c /Foobj\tokenizer.obj || goto :error

echo Compiling utils.c...
cl.exe /W3 /O2 /Iinclude /c src\utils.c /Foobj\utils.obj || goto :error

REM Link executable
echo Linking cq.exe...
link.exe obj\*.obj /OUT:build\cq.exe || goto :error

echo.
echo Build successful! Binary: build\cq.exe
echo.
echo You can now run: build\cq.exe -h
goto :end

:error
echo.
echo Build failed!
exit /b 1

:end
