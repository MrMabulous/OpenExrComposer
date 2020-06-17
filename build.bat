cd "%~dp0"
@echo off
@echo updating git submodules
git submodule update --init --recursive
git submodule update --recursive

@echo finding visual studio installation
if defined VARS_LOADED (goto :breakLoop)
for /f "usebackq delims=" %%i in (`.\buildtools\vswhere.exe -prerelease -latest -property installationPath`) do (
  if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" (
    call "%%i\VC\Auxiliary\Build\vcvars64.bat"
    SET VARS_LOADED=y
    goto :breakLoop
  )
)
@echo Error: Could not locate Visual Studio installation
exit /B 1
:breakLoop

@echo building openExrComposer with bazel
.\buildtools\bazel build --verbose_failures --cxxopt=/std:c++latest -c opt :openExrComposer