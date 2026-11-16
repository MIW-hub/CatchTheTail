# EOS로 모인 두 사람이 로비에서 게임을 시작해 게임 맵으로 넘어가는 것까지 확인하는 시험
# 사용법: powershell -File Tools\CTEosStartTest.ps1   (에디터를 끄고 실행)
$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root "docs\baseline"
New-Item -ItemType Directory -Force $out | Out-Null
$hl = Join-Path $out "eosstart_host.log"; $cl = Join-Path $out "eosstart_client.log"
Remove-Item $hl, $cl -Force -ErrorAction SilentlyContinue
if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { Write-Error "에디터가 켜져 있습니다. 먼저 끄세요."; exit 1 }

$exe = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
$base = "`"$root\CatchTheTail.uproject`" /Game/Levels/L_Mainmenu -game -windowed -ResX=640 -ResY=360 -nosound -nosplash -nop4 -log -NOSTEAM -ini:Engine:[SystemSettings]:ct.AutoLobbyStart=1"
$h = Start-Process $exe -ArgumentList "$base -WinX=0 -WinY=0 -ini:Engine:[SystemSettings]:ct.AutoHostAfterLogin=1 -ABSLOG=`"$hl`"" -PassThru
$code = $null
$deadline = (Get-Date).AddSeconds(90)
while ((Get-Date) -lt $deadline -and -not $code) {
	Start-Sleep -Seconds 3
	if (Test-Path $hl) {
		$m = Select-String -Path $hl -Pattern "\[CT\] 방 코드: (\S+)" -Encoding UTF8 | Select-Object -First 1
		if ($m) { $code = $m.Matches[0].Groups[1].Value }
	}
}
if (-not $code) { Get-Process UnrealEditor* -ErrorAction SilentlyContinue | Stop-Process -Force; "방장이 방 코드를 만들지 못함"; exit 3 }
"방 코드: $code"
Start-Sleep -Seconds 8
$c = Start-Process $exe -ArgumentList "$base -WinX=660 -WinY=0 -ini:Engine:[SystemSettings]:ct.AutoJoinCode=$code -ini:Engine:[SystemSettings]:ct.EosFreshDeviceId=1 -ABSLOG=`"$cl`"" -PassThru

# 게임이 시작될 때까지 기다림
$deadline = (Get-Date).AddSeconds(120)
$started = $false
while ((Get-Date) -lt $deadline) {
	Start-Sleep -Seconds 4
	if ((Test-Path $hl) -and (Select-String -Path $hl -Pattern "\[CT\] 게임 시작: 전체" -Quiet -Encoding UTF8)) { $started = $true; Start-Sleep -Seconds 25; break }
}
Get-Process UnrealEditor*, CrashReportClient* -ErrorAction SilentlyContinue | Stop-Process -Force

$hostLog = if (Test-Path $hl) { Select-String -Path $hl -Pattern "\[CT-Test\]|\[CT\] (게임 시작|게임 모드 규칙|AI 플레이어)|LogNet: Join succeeded|네트워크 오류" -Encoding UTF8 | ForEach-Object { ($_.Line -split "LogTemp: ")[-1] } } else { @() }
$clientLog = if (Test-Path $cl) { Select-String -Path $cl -Pattern "\[CT-Test\]|Welcomed by server|SeamlessTravel to: |SeamlessTravel finished|네트워크 오류|방장과의 연결 끊김|메인 메뉴로 이동" -Encoding UTF8 | ForEach-Object { ($_.Line -split "LogTemp: ")[-1] } } else { @() }
"-- 방장"; $hostLog; "-- 참가자"; $clientLog
$fail = @()
if (-not $started) { $fail += "방장: 게임이 시작되지 않음" }
if (-not ($clientLog -match "SeamlessTravel to: /Game/Levels/L_(Box|Country|Building)")) { $fail += "참가자: 게임 맵으로 넘어가지 못함" }
if (-not ($clientLog -match "SeamlessTravel finished")) { $fail += "참가자: 맵 이동이 끝나지 않음" }
if (($hostLog + $clientLog) -match "네트워크 오류|연결 끊김") { $fail += "접속 오류 발생" }
if ($fail.Count -eq 0) { "EOS 게임 시작 시험: PASS" } else { "EOS 게임 시작 시험: FAIL"; $fail | ForEach-Object { "  - $_" }; exit 3 }
