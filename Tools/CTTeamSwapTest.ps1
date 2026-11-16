# 로비 팀 변경 요청/수락 2인 접속 자동 시험
# 사용법: powershell -File Tools\CTTeamSwapTest.ps1
#   - 에디터를 끄고 실행하세요. 호스트(방장)와 클라이언트를 한 컴퓨터에서 띄워, 팀 정원을 1명으로 줄인 로비에서
#     방장이 팀 변경을 요청하고 클라이언트가 수락해 두 사람의 팀이 맞바뀌는지 로그로 확인합니다.
$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root "docs\baseline"
New-Item -ItemType Directory -Force $out | Out-Null
$hostLog = Join-Path $out "teamswap_host.log"
$clientLog = Join-Path $out "teamswap_client.log"
Remove-Item $hostLog, $clientLog -Force -ErrorAction SilentlyContinue

if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { Write-Error "에디터가 켜져 있습니다. 먼저 끄세요."; exit 1 }

$exe = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
$proj = "`"$root\CatchTheTail.uproject`""
$common = "-NOSTEAM -game -windowed -ResX=640 -ResY=360 -nosound -nosplash -nop4 -log"
$hostCmds = "ct.TeamCapacity 1,ct.TeamSwapTest 1,CTLobbyPreview"

$h = Start-Process $exe -ArgumentList "$proj /Game/Levels/L_Lobby?listen $common -WinX=0 -WinY=0 -ExecCmds=`"$hostCmds`" -ABSLOG=`"$hostLog`"" -PassThru
Start-Sleep -Seconds 12
$c = Start-Process $exe -ArgumentList "$proj 127.0.0.1 $common -WinX=660 -WinY=0 -ExecCmds=`"ct.TeamSwapTest 1`" -ABSLOG=`"$clientLog`"" -PassThru

# 요청과 수락이 끝날 때까지 기다림
$deadline = (Get-Date).AddSeconds(70)
$done = $false
while ((Get-Date) -lt $deadline) {
	Start-Sleep -Seconds 3
	if ((Test-Path $hostLog) -and (Select-String -Path $hostLog -Pattern "로비 팀 맞바꿈" -Quiet -Encoding UTF8)) { $done = $true; Start-Sleep -Seconds 2; break }
}
foreach ($p in @($h, $c)) { if ($p -and -not $p.HasExited) { Stop-Process $p -Force } }
Get-Process UnrealEditor* -ErrorAction SilentlyContinue | Stop-Process -Force

$lines = @()
foreach ($log in @($hostLog, $clientLog)) { if (Test-Path $log) { $lines += Select-String -Path $log -Pattern "\[CT-Test\]|로비 팀|Fatal error" -Encoding UTF8 | ForEach-Object { ($_.Line -split "LogTemp: ")[-1] } } }
$lines
if ($done) { Write-Output "팀 변경 시험: PASS (요청 -> 수락 -> 맞바꿈)" } else { Write-Output "팀 변경 시험: FAIL"; exit 3 }
