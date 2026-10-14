@echo off
del ..\Release\output\*.mva,*.img,*.mva.bin,*.img.bin
del ..\Release\output\new_nosdkdata.bin
del ..\Release\output\*.mva ..\Release\output\*.mva.bin
grep '^#define CFG_EFFECT_PARAM_IN_FLASH_EN' ..\app_src\system_config\app_config.h | grep '//' && set config1=0 || set config1=1
grep '^#define CFG_FUNC_REMIND_SOUND_EN' ..\app_src\system_config\app_config.h | grep '//' && set config2=0 || set config2=1
 ..\tools\merge_script\merge.exe %config1% [effect_data] %config2% [remind]
::grep '^#define CFG_DOUBLE_KEY_EN' ..\app_src\system_config\app_config.h | grep '//'&& set config3 = -k 0 || set config3 = -k 1
..\tools\merge_script\Andes_MVAGenerate.exe %config3 % 
if not exist ..\Release\output\B6_Audio_SDK.bin (del ..\Release\output\*.mva,*.img,*.mva.bin,*.img.bin)
del ..\Release\output\noSDKData.bin ..\Release\output\bin_name.bin  ..\Release\zs.bin