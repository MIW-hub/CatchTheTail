# 실제 게임 흐름의 EOS 방 접속 2인 시험: 방장이 방을 열면(로비 진입) 참가자가 방 코드로 입장해 로비 맵에서 만남
# 사용법: powershell -File Tools\CTEosFlowTest.ps1   (에디터를 끄고 실행)
$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root "docs\baseline"
New-Item -ItemType Directory -Force $out | Out-Null
$hl = Join-Path $out "eosflow_host.log"; $cl = Join-Path $out "eosflow_client.log"
Remove-Item $hl, $cl -Force -ErrorAction SilentlyContinue
if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { Write-Error "에디터가 켜져 있습니다. 먼저 끄세요."; exit 1 }

$exe = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
$base = "`"$root\CatchTheTail.uproject`" /Game/Levels/L_Mainmenu -game -windowed -ResX=640 -ResY=360 -nosound -nosplash -nop4 -log -NOSTEAM"
# 기기 ID는 같은 컴퓨터에서 하나라서, 두 인스턴스가 서로 다른 사람이 되려면 한쪽만 새 ID를 쓰게 해야 함 -> 방장은 새로 만든 ID, 참가자는 아래에서 지우고 새로 만든 ID
$h = Start-Process $exe -ArgumentList "$base -WinX=0 -WinY=0 -ini:Engine:[SystemSettings]:ct.AutoHostAfterLogin=1 -ABSLOG=`"$hl`"" -PassThru

# 방 코드가 로그에 나올 때까지 기다림
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
Start-Sleep -Seconds 50
Get-Process UnrealEditor*, CrashReportClient* -ErrorAction SilentlyContinue | Stop-Process -Force

$hostLog = if (Test-Path $hl) { Select-String -Path $hl -Pattern "\[CT\] (온라인|방 열기|방 코드)|LogNet: Join succeeded" -Encoding UTF8 | ForEach-Object { ($_.Line -split "LogTemp: ")[-1] } } else { @() }
$clientLog = if (Test-Path $cl) { Select-String -Path $cl -Pattern "\[CT\] (온라인|방 입장|방 입장 실패|네트워크 오류)|Welcomed by server" -Encoding UTF8 | ForEach-Object { ($_.Line -split "LogTemp: ")[-1] } } else { @() }
"-- 방장"; $hostLog; "-- 참가자"; $clientLog
$fail = @()
if (-not ($hostLog -match "방 열기 성공")) { $fail += "방장: 방 열기 실패" }
if (-not ($hostLog -match "Join succeeded")) { $fail += "방장: 참가자가 게임에 입장하지 못함" }
if (-not ($clientLog -match "Welcomed by server.*L_Lobby")) { $fail += "참가자: 방장의 로비에 들어가지 못함" }
if ($fail.Count -eq 0) { "EOS 실제 흐름 시험: PASS" } else { "EOS 실제 흐름 시험: FAIL"; $fail | ForEach-Object { "  - $_" }; exit 3 }
