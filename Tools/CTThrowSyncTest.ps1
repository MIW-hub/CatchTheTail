# 던지기 동기화 2인 접속 자동 시험
# 사용법: powershell -File Tools\CTThrowSyncTest.ps1   (에디터를 끄고 실행)
#   방장(서버)과 클라이언트를 한 컴퓨터에서 띄워 보물찾기 한 판을 열고, 방장이 오브젝트를 집어 클라이언트를 향해 던질 때
#   서버 로그(들기, 던짐, 맞음)와 클라이언트 로그(상대가 든 오브젝트가 보임, 날아가는 오브젝트가 보임, 내 에너지 감소)를 확인합니다.
$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root "docs\baseline"
New-Item -ItemType Directory -Force $out | Out-Null
$hostLog = Join-Path $out "throwsync_host.log"
$clientLog = Join-Path $out "throwsync_client.log"
Remove-Item $hostLog, $clientLog -Force -ErrorAction SilentlyContinue

if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { Write-Error "에디터가 켜져 있습니다. 먼저 끄세요."; exit 1 }

$exe = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
$proj = "`"$root\CatchTheTail.uproject`""
$common = "-ini:Engine:[SystemSettings]:ct.ThrowSyncTest=1 -NOSTEAM -game -windowed -ResX=640 -ResY=360 -nosound -nosplash -nop4 -log"
# 두 사람이 모인 뒤 경기가 시작되도록 시작 카운트다운을 길게 잡음 (접속할 때마다 다시 시작됨)
$hostCmds = "set CTGameMode DebugAIPlayerCount 0,set CTGameMode GameDurationSeconds 120,set CTGameMode RoundDurationSeconds 40"

$h = Start-Process $exe -ArgumentList "$proj /Game/Levels/L_Building?listen -CTMode=1 $common -WinX=0 -WinY=0 -ExecCmds=`"$hostCmds`" -ABSLOG=`"$hostLog`"" -PassThru
Start-Sleep -Seconds 12
$c = Start-Process $exe -ArgumentList "$proj 127.0.0.1 $common -WinX=660 -WinY=0 -ABSLOG=`"$clientLog`"" -PassThru

$deadline = (Get-Date).AddSeconds(110)
$done = $false
while ((Get-Date) -lt $deadline) {
	Start-Sleep -Seconds 3
	if ((Test-Path $hostLog) -and (Select-String -Path $hostLog -Pattern "서버: 시험 끝|못 들었음|오브젝트가 없음|자리가 없음" -Quiet -Encoding UTF8)) { $done = $true; Start-Sleep -Seconds 3; break }
}
foreach ($p in @($h, $c)) { if ($p -and -not $p.HasExited) { Stop-Process $p -Force } }
Get-Process UnrealEditor* -ErrorAction SilentlyContinue | Stop-Process -Force

$host_ = if (Test-Path $hostLog) { Select-String -Path $hostLog -Pattern "\[CT-Test\]|오브젝트 들기|오브젝트 던짐|보물찾기 맞음|Fatal error" -Encoding UTF8 | ForEach-Object { ($_.Line -split "LogTemp: ")[-1] } } else { @() }
$client_ = if (Test-Path $clientLog) { Select-String -Path $clientLog -Pattern "\[CT-Test\]|Fatal error" -Encoding UTF8 | ForEach-Object { ($_.Line -split "LogTemp: ")[-1] } } else { @() }
"-- 서버"; $host_; "-- 클라이언트"; $client_

$fail = @()
if (-not $done) { $fail += "시험이 끝까지 진행되지 않음" }
if (-not ($host_ -match "방장이 오브젝트를 들었음")) { $fail += "서버: 방장이 오브젝트를 못 듦" }
if (-not ($host_ -match "오브젝트 던짐")) { $fail += "서버: 던지지 못함" }
if (-not ($host_ -match "보물찾기 맞음")) { $fail += "서버: 클라이언트를 맞히지 못함" }
if (-not ($client_ -match "든 것이 보임")) { $fail += "클라: 상대가 든 오브젝트가 안 보임" }
if (-not ($client_ -match "날아가는 오브젝트가 보임")) { $fail += "클라: 날아가는 오브젝트가 안 보임" }
if (-not ($client_ -match "에너지 값 5")) { $fail += "클라: 내 에너지가 5로 줄지 않음" }
if (($host_ + $client_) -match "Fatal error") { $fail += "치명적 오류 로그" }
if ($fail.Count -eq 0) { Write-Output "던지기 동기화 시험: PASS" } else { Write-Output "던지기 동기화 시험: FAIL"; $fail | ForEach-Object { "  - $_" }; exit 3 }
