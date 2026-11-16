# EOS 방 + 실제 게임 접속(P2P) 2인 시험 (에픽 계정 없이 기기 ID 로그인)
# 사용법: powershell -File Tools\CTEosLobbyTest.ps1   (에디터를 끄고 실행)
#   Config\Windows\WindowsEngine.ini 에 EOS 접속 값이 들어 있어야 합니다.
$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root "docs\baseline"
New-Item -ItemType Directory -Force $out | Out-Null
$hl = Join-Path $out "eos_host.log"; $cl = Join-Path $out "eos_client.log"
Remove-Item $hl, $cl -Force -ErrorAction SilentlyContinue
if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { Write-Error "에디터가 켜져 있습니다. 먼저 끄세요."; exit 1 }

$exe = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
$common = "`"$root\CatchTheTail.uproject`" /Game/Levels/L_Mainmenu -game -windowed -ResX=640 -ResY=360 -nosound -nosplash -nop4 -log -NOSTEAM"
$code = "HC" + (Get-Random -Minimum 1000 -Maximum 9999)
$h = Start-Process $exe -ArgumentList "$common -WinX=0 -WinY=0 -ExecCmds=`"ct.EosGameHost $code`" -ABSLOG=`"$hl`"" -PassThru
Start-Sleep -Seconds 40
$c = Start-Process $exe -ArgumentList "$common -WinX=660 -WinY=0 -ExecCmds=`"ct.EosGameJoin $code`" -ABSLOG=`"$cl`"" -PassThru
Start-Sleep -Seconds 45
Get-Process UnrealEditor*, CrashReportClient* -ErrorAction SilentlyContinue | Stop-Process -Force

"방 코드: $code"
$host_ = if (Test-Path $hl) { Select-String -Path $hl -Pattern "\[CT-EOS\]" -Encoding UTF8 | ForEach-Object { ($_.Line -split "LogTemp: ")[-1] } } else { @() }
$client_ = if (Test-Path $cl) { Select-String -Path $cl -Pattern "\[CT-EOS\]" -Encoding UTF8 | ForEach-Object { ($_.Line -split "LogTemp: ")[-1] } } else { @() }
"-- 방장"; $host_; "-- 참가자"; $client_

$fail = @()
if (-not ($host_ -match "방 만들기 성공")) { $fail += "방장: 방 만들기 실패" }
if (-not ($client_ -match "방 들어가기 성공")) { $fail += "참가자: 방에 들어가지 못함" }
$hostNet = if (Test-Path $hl) { Select-String -Path $hl -Pattern "LogNet: Join succeeded" -Encoding UTF8 } else { @() }
$clientNet = if (Test-Path $cl) { Select-String -Path $cl -Pattern "LogNet: Welcomed by server .*L_Lobby" -Encoding UTF8 } else { @() }
"-- 접속 로그: 방장 $($hostNet.Count)줄 / 참가자 $($clientNet.Count)줄"
if ($hostNet.Count -eq 0) { $fail += "방장: 참가자의 게임 입장(Join succeeded)을 받지 못함" }
if ($clientNet.Count -eq 0) { $fail += "참가자: 방장 게임의 로비 맵에 들어가지 못함(Welcomed by server)" }
if ($fail.Count -eq 0) { "EOS 게임 접속 시험: PASS" } else { "EOS 게임 접속 시험: FAIL"; $fail | ForEach-Object { "  - $_" }; exit 3 }
