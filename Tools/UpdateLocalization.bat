@echo off
rem 번역 갱신: 문구 수집 -> 번역표 반영 -> 컴파일
rem 새 문구가 생기면 Config\Localization\Game_Translations.tsv 에 "원문<TAB>영어<TAB>일본어" 줄을 추가한 뒤 이 파일을 실행하세요.
rem 에디터가 켜져 있으면 먼저 끄세요.
set PROJECT=%~dp0..\CatchTheTail.uproject
set ENGINE=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe

"%ENGINE%" "%PROJECT%" -run=GatherText -config="Config/Localization/Game_Gather.ini" -unattended -nop4 -nosplash
perl "%~dp0ApplyTranslations.pl" "%~dp0.."
"%ENGINE%" "%PROJECT%" -run=GatherText -config="Config/Localization/Game_Compile.ini" -unattended -nop4 -nosplash
pause
