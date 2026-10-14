@echo off
cd ..
if exist mp2 (rd /s /q mp2)
md mp2
cd remind_res
for %%i in (16K//*.mp3) do (..\tools\remind_script\ffmpeg.exe -i 16K//%%i -ab 24000 -ac 1 -ar 16000 ..\mp2//%%i.mp2)
cd ..
for %%i in (remind_res//*.mp3) do (tools\remind_script\ffmpeg.exe -i remind_res//%%i -ab 48000 -ac 1 mp2//%%i.mp2)
cd mp2
ren *.mp2 *.
cd ..
tools\remind_script\MergeAudio2BinNew.exe -a 0x0 -i ..\..\mp2
rd /s /q mp2
del ..\Release\output\*.img
del ..\Release\output\main_merge.mva
grep '^#define CFG_EFFECT_PARAM_IN_FLASH_EN' ..\app_src\system_config\app_config.h | grep '//' && set config1=0 || set config1=1
grep '^#define CFG_FUNC_REMIND_SOUND_EN' ..\app_src\system_config\app_config.h | grep '//' && set config2=0 || set config2=1
 ..\tools\merge_script\merge.exe %config1% [effect_data] %config2% [remind]
grep '^#define CFG_DOUBLE_KEY_EN' ..\app_src\system_config\app_config.h | grep '//'&& set config3 = -k 0 || set config3 = -k 1
..\tools\merge_script\Andes_MVAGenerate.exe %config3 % 
if not exist ..\Release\output\BT_Audio_APP.bin (del ..\Release\output\main_merge.mva)
del ..\Release\output\noSDKData.bin ..\Release\output\bin_name.bin  ..\Release\zs.bin
pause
