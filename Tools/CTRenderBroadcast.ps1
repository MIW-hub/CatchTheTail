# AI 8명이 실제 경기를 하는 모습을 중계 카메라로 촬영합니다. 에디터를 끄고 실행. 결과: Saved\CineRender\<Name>.avi
# 예) powershell -File Tools\CTRenderBroadcast.ps1 -Map L_Box -Mode 0 -Style 1 -Seconds 60 -Name aerial_tailchase
#   -Mode  0 꼬리잡기, 1 보물찾기
#   -Style 1 플레이 컷 편집, 2 공중 비행 촬영
#   -Seconds 60 / 90 / 120 (시퀀스 길이)
#   -Hud   경기 HUD 를 보이려면 지정 (관전자 기준 정보라 기본은 숨김)
param([string]$Map = "L_Box", [int]$Mode = 0, [int]$Style = 1, [int]$Seconds = 60, [string]$Name = "broadcast", [int]$Width = 1920, [int]$Height = 1080, [string]$Format = "Video", [double]$Scale = 1, [switch]$Hud)
$root = Split-Path -Parent $PSScriptRoot
$out = "$root\Saved\CineRender"
if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { Write-Error "에디터가 켜져 있습니다."; exit 1 }
New-Item -ItemType Directory $out -Force | Out-Null
Get-ChildItem $out -File | ForEach-Object { $_.Delete() }
$log = "$env:TEMP\cine_broadcast.log"
$hudArg = ""
if ($Hud) { $hudArg = "-CTBroadcastHud" }
$a = "$root\CatchTheTail.uproject /Game/Levels/$Map -game -windowed -ResX=$Width -ResY=$Height -ForceRes -CTMode=$Mode -CTAI=8 -CTBroadcast=$Style -CTBroadcastScale=$Scale $hudArg -MovieSceneCaptureType=/Script/MovieSceneCapture.AutomatedLevelSequenceCapture -LevelSequence=/Game/Cinematic/LS_Broadcast$Seconds.LS_Broadcast$Seconds -MovieFormat=$Format -MovieFolder=`"$out`" -MovieName=$Name -MovieFrameRate=30 -NoTextureStreaming -NoLoadingScreen -MovieCinematicMode=yes -NoScreenMessages -nosound -nosplash -nop4 -log -ABSLOG=`"$log`""
$p = Start-Process -FilePath "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList $a -PassThru
$p.WaitForExit(1800000) | Out-Null
if (-not $p.HasExited) { $p.Kill() }
"결과 -> {0} ({1}개 파일), 로그 {2}" -f $out, (Get-ChildItem $out -File).Count, $log
