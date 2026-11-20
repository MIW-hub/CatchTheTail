# 인트로 시네마틱을 PNG 프레임(30fps)으로 뽑습니다. 에디터를 끄고 실행. 결과: Saved\CineRender\intro.0000.png ...
# 사용법: powershell -File Tools\CTRenderIntroCinematic.ps1 [-Width 1920 -Height 1080]
param([int]$Width = 1920, [int]$Height = 1080, [string]$Format = "PNG")
$root = Split-Path -Parent $PSScriptRoot
$out = "$root\Saved\CineRender"
if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { Write-Error "에디터가 켜져 있습니다."; exit 1 }
New-Item -ItemType Directory $out -Force | Out-Null
Get-ChildItem $out -File | ForEach-Object { $_.Delete() }
$log = "$env:TEMP\cine_render.log"
$a = "$root\CatchTheTail.uproject /Game/Levels/L_Cinematic -game -windowed -ResX=$Width -ResY=$Height -ForceRes -MovieSceneCaptureType=/Script/MovieSceneCapture.AutomatedLevelSequenceCapture -LevelSequence=/Game/Cinematic/LS_IntroCinematic.LS_IntroCinematic -MovieFormat=$Format -MovieFolder=`"$out`" -MovieName=intro -MovieFrameRate=30 -NoTextureStreaming -NoLoadingScreen -MovieCinematicMode=yes -NoScreenMessages -nosound -nosplash -nop4 -log -ABSLOG=`"$log`""
$p = Start-Process -FilePath "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList $a -PassThru
$p.WaitForExit(900000) | Out-Null
if (-not $p.HasExited) { $p.Kill() }
"프레임 {0}장 -> {1}" -f (Get-ChildItem $out -File).Count, $out
