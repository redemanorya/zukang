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
tools\remind_script\MergeAudio2BinNew.exe -a 0x0 -i tools\remind_script\
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