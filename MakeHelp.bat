@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by MAGICSCROLL.HPJ. >"hlp\MagicScroll.hm"
echo. >>"hlp\MagicScroll.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\MagicScroll.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\MagicScroll.hm"
echo. >>"hlp\MagicScroll.hm"
echo // Prompts (IDP_*) >>"hlp\MagicScroll.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\MagicScroll.hm"
echo. >>"hlp\MagicScroll.hm"
echo // Resources (IDR_*) >>"hlp\MagicScroll.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\MagicScroll.hm"
echo. >>"hlp\MagicScroll.hm"
echo // Dialogs (IDD_*) >>"hlp\MagicScroll.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\MagicScroll.hm"
echo. >>"hlp\MagicScroll.hm"
echo // Frame Controls (IDW_*) >>"hlp\MagicScroll.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\MagicScroll.hm"
REM -- Make help for Project MAGICSCROLL


echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\MagicScroll.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\MagicScroll.hlp" goto :Error
if not exist "hlp\MagicScroll.cnt" goto :Error
echo.
if exist Install\nul copy "hlp\MagicScroll.hlp" Install
if exist Debug\nul copy "hlp\MagicScroll.hlp" Debug
if exist Release\nul copy "hlp\MagicScroll.hlp" Release
echo.
goto :done

:Error
echo hlp\MagicScroll.hpj(1) : error: Problem encountered creating help file

:done
echo.
