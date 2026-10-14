del ..\nds32-ae210p.ld

REM Update nds32-ae210p.sag based on sys_em.cfg
echo Updating nds32-ae210p.sag with dynamic values from sys_em.cfg...
powershell -ExecutionPolicy Bypass -File ..\tools\update_sag.ps1 -ConfigFile "sys_em.cfg" -SagFile "..\nds32-ae210p.sag"
if %errorlevel% neq 0 (
    echo Error: Failed to update nds32-ae210p.sag
    exit /b %errorlevel%
)

nds_ldsag -t  ../nds32_template.txt  ../nds32-ae210p.sag -o ../nds32-ae210p.ld

cd ..
if exist mp2 (rd /s /q mp2)
md mp2
cd remind_res
for %%i in (16K//*.mp3) do (..\tools\remind_script\ffmpeg.exe -i 16K//%%i -ab 24000 -ac 1 -ar 16000 ..\mp2//%%i.mp2 >> info.txt 2>&1)
del info.txt
cd ..
for %%i in (remind_res//*.mp3) do (tools\remind_script\ffmpeg.exe -i remind_res//%%i -ab 48000 -ac 1 mp2//%%i.mp2 >> info.txt 2>&1)
del info.txt
cd mp2
ren *.mp2 *.
cd ..
tools\remind_script\MergeAudio2BinNew.exe -a 0x0 -i mp2 -o tools\remind_script\
rd /s /q mp2
fc tools\remind_script\sound_remind_item.h app_framework\audio_engine\remind_sound_item.h
if %errorlevel%==1 (copy tools\remind_script\sound_remind_item.h app_framework\audio_engine\remind_sound_item.h)
tools\libs_copy_script\copyLibs.exe -s tools\script_copy.ini
grep '^#define CFG_DOUBLE_KEY_EN' app_src\system_config\app_config.h
if %errorlevel%==0 (goto found) else (goto not_found)
:found
grep '^#define CFG_DOUBLE_KEY_EN' app_src\system_config\app_config.h | grep '//'
if %errorlevel%==1 (nds_ldsag.exe -t tools\nds32_template.txt tools\BP16x_Dkey.sag -o BP16x.ld) else (nds_ldsag.exe -t tools\nds32_template.txt tools\BP16x.sag -o BP16x.ld)
exit
:not_found
nds_ldsag.exe -t tools\nds32_template.txt tools\BP16x.sag -o BP16x.ld
exit